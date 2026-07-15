/*
 * This file is part of the UWB stack for linux.
 *
 * Copyright (c) 2021-2021 Qorvo US, Inc.
 *
 * This software is provided under the GNU General Public License, version 2
 * (GPLv2), as well as under a Qorvo commercial license.
 *
 * You may choose to use this software under the terms of the GPLv2 License,
 * version 2 ("GPLv2"), as published by the Free Software Foundation.
 * You should have received a copy of the GPLv2 along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * This program is distributed under the GPLv2 in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GPLv2 for more
 * details.
 *
 * If you cannot meet the requirements of the GPLv2, you may not use this
 * software for any purpose without first obtaining a commercial license from
 * Qorvo. Please contact Qorvo to inquire about licensing terms.
 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/skbuff.h>
#include <linux/version.h>

#include "qm35.h"
#include "qm35_spi.h"
#include "qm35_core.h"
#include "qm35_transport.h"
#include "qm35_uci_probe.h"
#include "uci/uci_spec_fira.h"

/* To avoid dependency to uci.h, and so to whole qosal, define here required
 * symbols. */
#define UCI_PACKET_HEADER_SIZE 4
#define UCI_MT_GID_OID(mt, gid, oid) ((mt) << 13 | (gid) << 8 | (oid))

/**
 * uci_status_to_err() - Convert UCI status value to negative error
 * @status: The positive UCI error to convert.
 *
 * This function convert status code from UCI status to a negative error
 * value. This allow UCI command functions to return a negative value like
 * most kernel functions in case of error.
 *
 * Note: Be careful, the UCI error code don't match the linux/errno.h ones.
 * Return: Zero on success, else a negative error.
 */
static inline int uci_status_to_err(enum uci_status_code status)
{
	/* Ensure returned errors are negative */
	return -(int)status;
}

/**
 * uci_generic_status() - Handle UCI response message.
 * @qmup: The QM35 UCI probing instance.
 * @skb: The UCI frame received.
 * @mt_gid_oid: The message id from UCI packet header.
 * @payload_len: The payload length from UCI packet header.
 *
 * Do analysis of payload data for a status response message. The status byte
 * is saved in status field of struct qm35_uci_client for use by UCI command
 * currently waiting for completion.
 *
 * Context: Called from inside the qm35_transport_event() which receive the
 *          frame and feed it to the UCI core for parsing.
 * Return: Always 0.
 */
static int uci_generic_status(struct qm35_uci_probing *qmup,
			      struct sk_buff *skb, uint16_t mt_gid_oid,
			      uint16_t payload_len)
{
	/* Check if internal client sent a command */
	if (!atomic_read(&qmup->cmd_sent))
		return 0;

	/* Retrieve status */
	if (skb->len < 1) {
		qmup->status = UCI_STATUS_SYNTAX_ERROR;
	} else {
		qmup->status = skb->data[0];
	}

	/* Signal caller */
	complete(&qmup->completion);
	return 0;
}

/**
 * uci_device_state_notification() - Handle UCI notification message.
 * @qmup: The QM35 UCI probing instance.
 * @skb: The UCI frame received.
 * @mt_gid_oid: The message id from UCI packet header.
 * @payload_len: The payload length from UCI packet header.
 *
 * Context: Called from inside the qm35_transport_event() which receive the
 *          notification.
 * Return: Always 0.
 */
static int uci_device_state_notification(struct qm35_uci_probing *qmup,
					 struct sk_buff *skb,
					 uint16_t mt_gid_oid,
					 uint16_t payload_len)
{
	struct qm35_spi *qmspi =
		container_of(qmup, struct qm35_spi, probing_data);
	enum qm35_state new_state = QM35_STATE_UNKNOWN;
	uint8_t device_state = UCI_DEVICE_STATE_ERROR;

	/* Retrieve device state */
	if (skb->len) {
		device_state = skb->data[0];
	}
	switch (device_state) {
	case UCI_DEVICE_STATE_READY:
		new_state = QM35_STATE_READY;
		break;
	case UCI_DEVICE_STATE_ACTIVE:
		new_state = QM35_STATE_ACTIVE;
		break;
	case UCI_DEVICE_STATE_ERROR:
		new_state = QM35_STATE_ERROR;
		break;
	}
	qm35_state_set(&qmspi->base, new_state);
	return 0;
}

/**
 * uci_device_info_status() - Handle UCI response for GET_DEVICE_INFO command.
 * @qmup: The QM35 UCI probing instance.
 * @skb: The UCI frame received.
 * @mt_gid_oid: The message id from UCI packet header.
 * @payload_len: The payload length from UCI packet header.
 *
 * Do analysis of payload data for a GET_DEVICE_INFO response message.
 *
 * The status byte is saved in status field of struct qm35_uci_client and the
 * device information is saved in the struct qm35_uci_device_info given by the
 * user_data field.
 *
 * The user_data parameter ISN'T used since it is common to all handler, so
 * unusable for our purpose (on-stack per command specific data).
 *
 * Context: Called from inside the qm35_transport_event() which receive the
 *          frame and feed it to the UCI core for parsing.
 * Return: Always 0.
 */
static int uci_device_info_status(struct qm35_uci_probing *qmup,
				  struct sk_buff *skb, uint16_t mt_gid_oid,
				  uint16_t payload_len)
{
	struct qm35_uci_device_info *info = qmup->cmd_data;
	/* Minimum length is vendor_length = 0, no data, +1 for status. */
	size_t minlen = offsetof(struct qm35_uci_device_info, vendor_data) + 1;

	/* Check if internal client sent a command. */
	if (!atomic_read(&qmup->cmd_sent))
		return 0;

	/* Retrieve status & data. */
	if (skb->len < minlen || qmup->cmd_data_sz < minlen) {
		qmup->status = UCI_STATUS_SYNTAX_ERROR;
	} else {
		/* Read status. */
		qmup->status = skb->data[0];
		minlen--;
		/* Read info. */
		memcpy(info, skb->data + 1, minlen);
		if (info->vendor_length) {
			size_t len = min((size_t)info->vendor_length,
					 qmup->cmd_data_sz - minlen);
			len = min(len, (size_t)skb->len - (1 + minlen));
			/* Read vendor data. */
			memcpy(info->vendor_data, skb->data + (1 + minlen),
			       len);
			if (len < info->vendor_length)
				/* Truncated vendor data! May be enough. */
				info->vendor_length = len;
		}
	}

	/* Signal caller */
	complete(&qmup->completion);
	return 0;
}

/**
 * qm35_uci_probe_handle() - QM35 transport handler for UCI responses.
 * @data: Pointer to qm35_uci_probing structure.
 * @skb: UCI packet received.
 *
 * Context: Always called from qm35_transport_event().
 */
static void qm35_uci_probe_handle(void *data, struct sk_buff *skb)
{
	struct qm35_uci_probing *qmup = data;
	struct qm35_spi *qmspi =
		container_of(qmup, struct qm35_spi, probing_data);
	int rc;

	while (skb->len >= UCI_PACKET_HEADER_SIZE) {
		uint16_t mt_gid_oid;
		/* Only handle response or notif. */
		uint16_t payload_len = skb->data[3];
		/* UCI message is truncated! */
		if (skb->len < (UCI_PACKET_HEADER_SIZE + payload_len))
			break;
		/* Get header mt_gid_oid and switch on it */
		mt_gid_oid = ((uint16_t)skb->data[0]) << 8 | skb->data[1];
		skb_pull(skb, UCI_PACKET_HEADER_SIZE);

		/* Call handler for specific mt_gid_oid. */
		switch (mt_gid_oid) {
		case UCI_MT_GID_OID(UCI_MESSAGE_TYPE_RESPONSE, UCI_GID_CORE,
				    UCI_OID_CORE_DEVICE_RESET):
			rc = uci_generic_status(qmup, skb, mt_gid_oid,
						payload_len);
			break;
		case UCI_MT_GID_OID(UCI_MESSAGE_TYPE_RESPONSE, UCI_GID_CORE,
				    UCI_OID_CORE_GET_DEVICE_INFO):
			rc = uci_device_info_status(qmup, skb, mt_gid_oid,
						    payload_len);
			break;
		case UCI_MT_GID_OID(UCI_MESSAGE_TYPE_NOTIFICATION, UCI_GID_CORE,
				    UCI_OID_CORE_DEVICE_STATUS):
			rc = uci_device_state_notification(
				qmup, skb, mt_gid_oid, payload_len);
			break;
		default:
			dev_info(qmspi->base.dev,
				 "Probe: Unknown UCI packet "
				 "(mt_gid_oid=0x%x)\n",
				 mt_gid_oid);
			rc = 0;
			break;
		}
		skb_pull(skb, payload_len);
		if (rc) {
			dev_warn(qmspi->base.dev,
				 "Probe: Error handling UCI packet "
				 "(rc=%d, mt_gid_oid=0x%x)\n",
				 rc, mt_gid_oid);
		}
	}
	if (skb->len) {
		dev_warn(qmspi->base.dev,
			 "Probe: Truncated UCI packet "
			 "(%d bytes remain)\n",
			 skb->len);
	}
	consume_skb(skb);
}

/**
 * qm35_uci_probe_setup() - Setup UCI probing.
 * @qmspi: QM35 SPI device instance.
 *
 * Install UCI packet handler for probing. Probing use the HIGH priority handler.
 */
void qm35_uci_probe_setup(struct qm35_spi *qmspi)
{
	struct qm35_uci_probing *qmup = &qmspi->probing_data;

	memset(qmup, 0, sizeof(*qmup));
	mutex_init(&qmup->lock);
	init_completion(&qmup->completion);
	atomic_set(&qmup->cmd_sent, 0);

	qm35_transport_register(&qmspi->base, QM35_TRANSPORT_MSG_UCI,
				QM35_TRANSPORT_PRIO_HIGH, qm35_uci_probe_handle,
				qmup);
}

/**
 * qm35_uci_probe_cleanup() - Clean UCI probing.
 * @qmspi: QM35 SPI device instance.
 *
 * Remove UCI packet handler used for probing.
 */
void qm35_uci_probe_cleanup(struct qm35_spi *qmspi)
{
	qm35_transport_unregister(&qmspi->base, QM35_TRANSPORT_MSG_UCI,
				  QM35_TRANSPORT_PRIO_HIGH,
				  qm35_uci_probe_handle);
}

/**
 * qm35_uci_probe_send_cmd() - Synchronous UCI command execution helper.
 * @qmspi: QM35 SPI device instance.
 * @cmd: The command to send.
 * @cmd_sz: The command size.
 * @user_data: The user data passed to response message handler.
 * @udata_sz: The user data size.
 *
 * This function is used to send a command, already prepared by the caller, and
 * wait for response.
 *
 * The provided user_data (which can be on caller stack) is stored in the
 * struct qm35_uci_client which is inside struct qm35 instance so the response
 * message handler can decode and store information for use by the caller.
 *
 * Call to this function is protected by a mutex, so two commands cannot be
 * send at the same time.
 *
 * Context: This function wait for completion, so it cannot be called from the
 *          same context than the qm35_transport_event() function which will do
 *          response message analysis.
 * Return: Zero on success, else a negative UCI error.
 */
static int qm35_uci_probe_send_cmd(struct qm35_spi *qmspi, const uint8_t *cmd,
				   size_t cmd_sz, void *user_data,
				   size_t udata_sz)
{
	struct qm35_uci_probing *qmup = &qmspi->probing_data;
	enum uci_status_code status;
	int ret;

	mutex_lock(&qmup->lock);

	/* Store local user_data in our structure to be command specific. */
	qmup->cmd_data = user_data;
	qmup->cmd_data_sz = udata_sz;
	/* Indicate that internal client sent a command */
	atomic_inc(&qmup->cmd_sent);
	/* Send message */
	ret = qm35_transport_send_direct(&qmspi->base, QM35_TRANSPORT_MSG_UCI,
					 cmd, cmd_sz);
	/* Wait for response */
	ret = wait_for_completion_timeout(&qmup->completion, HZ);
	atomic_dec(&qmup->cmd_sent);
	/* Retrieve status */
	status = qmup->status;

	mutex_unlock(&qmup->lock);

	return uci_status_to_err(ret ? status : UCI_STATUS_UCI_MESSAGE_RETRY);
}

/**
 * qm35_uci_probe_device_reset() - Sends a CORE_DEVICE_RESET message.
 * @qmspi: QM35 SPI device instance.
 *
 * Prepare and send the DEVICE_RESET command message to the QM35.
 *
 * Context: See qm35_uci_probe_send_cmd().
 * Return: Zero on success, else a negative UCI error.
 */
int qm35_uci_probe_device_reset(struct qm35_spi *qmspi)
{
	uint16_t mt_gid_oid = UCI_MT_GID_OID(UCI_MESSAGE_TYPE_COMMAND,
					     UCI_GID_CORE,
					     UCI_OID_CORE_DEVICE_RESET);
	uint8_t cmd[UCI_PACKET_HEADER_SIZE + 1] = { mt_gid_oid >> 8, mt_gid_oid,
						    0, 1, 0 };
	int ret = qm35_uci_probe_send_cmd(qmspi, cmd, sizeof(cmd), NULL, 0);
	if (ret == uci_status_to_err(UCI_STATUS_REJECTED))
		ret = 0; /* Ignore if unsupported by FW. */
	else
		qm35_state_set(&qmspi->base, QM35_STATE_UNKNOWN);
	return ret;
}

/**
 * qm35_uci_probe_device_info() - Sends a CORE_GET_DEVICE_INFO_CMD message.
 * @qmspi: QM35 SPI device instance.
 * @info: Pointer to device information structure to fill.
 * @info_sz: Size of device information structure.
 *
 * Prepare and send the DEVICE_RESET command message to the QM35.
 *
 * Context: See qm35_uci_probe_send_cmd().
 * Return: Zero on success, else a negative UCI error.
 */
int qm35_uci_probe_device_info(struct qm35_spi *qmspi,
			       struct qm35_uci_device_info *info,
			       size_t info_sz)
{
	uint16_t mt_gid_oid = UCI_MT_GID_OID(UCI_MESSAGE_TYPE_COMMAND,
					     UCI_GID_CORE,
					     UCI_OID_CORE_GET_DEVICE_INFO);
	uint8_t cmd[UCI_PACKET_HEADER_SIZE] = { mt_gid_oid >> 8, mt_gid_oid, 0,
						0 };
	return qm35_uci_probe_send_cmd(qmspi, cmd, sizeof(cmd), info, info_sz);
}
