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
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/skbuff.h>

#include "qm35.h"
#include "qm35_transport.h"

/**
 * qm35_transport_start() - Calls the start callback.
 * @qm35: QM35 instance to start.
 *
 * This function calls the start() callback from the transport driver to
 * power-on the QM35 device and ensure it stays powered.
 *
 * The transport module **MUST** use a counting semaphore or an atomic counter
 * to handle multiple call by different module correctly.
 *
 * Returns: Zero or a negative error from start callback.
 */
int qm35_transport_start(struct qm35 *qm35)
{
	if (!qm35->transport_ops->start)
		return 0;
	return qm35->transport_ops->start(qm35);
}
EXPORT_SYMBOL(qm35_transport_start);

/**
 * qm35_transport_stop() - Calls the stop callback.
 * @qm35: QM35 instance to stop.
 *
 * This function calls the stop() callback from the transport driver to
 * power-off the QM35 device. The transport module should power-down the
 * device immediately or after a short delay.
 */
void qm35_transport_stop(struct qm35 *qm35)
{
	if (qm35->transport_ops->stop)
		qm35->transport_ops->stop(qm35);
}
EXPORT_SYMBOL(qm35_transport_stop);

/**
 * qm35_transport_reset() - Calls the reset callback.
 * @qm35: QM35 instance to reset.
 * @bootrom: Reset to bootrom command mode.
 *
 * This function is used by the ``qm35_bypass_control()`` function to request
 * the transport module to reset the device. This allow the UCI char dev to
 * reset the QM35 using a simple IOCTL.
 *
 * Returns: Zero or a negative error from reset callback.
 */
int qm35_transport_reset(struct qm35 *qm35, bool bootrom)
{
	int rc = 0;

	if (qm35->transport_ops->reset)
		rc = qm35->transport_ops->reset(qm35, bootrom);
	return rc;
}

/**
 * qm35_transport_fw_update() - Calls the firmware update callback.
 * @qm35: QM35 instance to update.
 * @current_ver: Firmware version currently running on the QM35.
 * @device_id: QM35 device_id.
 * @fw_name: Name of the firmware file to be loaded, replacing the default one.
 *
 * This function is used by the ``qm35_register_device()`` function to update the
 * firmware of the QM35 chip if needed after the probing.
 *
 * Returns: Zero or a negative error from firmware update callback.
 */
int qm35_transport_fw_update(struct qm35 *qm35,
			     struct qm35_fw_version *current_ver, u16 device_id,
			     const char *fw_name)
{
	if (!qm35->transport_ops->fw_update)
		return 0;
	return qm35->transport_ops->fw_update(qm35, current_ver, device_id,
					      fw_name);
}

/**
 * qm35_transport_power() - Calls the power callback.
 * @qm35: QM35 instance to power manage.
 * @on: Int that define if we power on or off.
 *
 * This function allows UCI char device to manage power manually while keeping
 * the char device opened (ie. transport driver started).
 *
 * Returns: Zero or a negative error from power callback.
 */
int qm35_transport_power(struct qm35 *qm35, int on)
{
	int rc = 0;
	if (qm35->transport_ops->power) {
		rc = qm35->transport_ops->power(qm35, on);
	}
	return rc;
}

/**
 * qm35_transport_has_high_prio() - Check high-prio handler registered.
 * @qm35: The QM35 device.
 * @type: The type of message.
 *
 * Return: 1 if an high-prio handler is registered else 0.
 */
static int qm35_transport_has_high_prio(struct qm35 *qm35,
					enum qm35_transport_msg_type type)
{
	int idx, ret = 0;
	if (type >= QM35_TRANSPORT_MSG_MAX)
		return -EINVAL;

	/* Start lookup with high prio handler. */
	idx = type * QM35_TRANSPORT_PRIO_COUNT + QM35_TRANSPORT_PRIO_HIGH;

	/* To avoid race condition in qm35_transport_event(), we need to
	 * ensure no concurrent access on this table. */
	spin_lock(&qm35->lock);
	ret = !!qm35->rx_handlers[idx].cb;
	spin_unlock(&qm35->lock);
	return ret;
}

/**
 * qm35_transport_send() - Send a packet to the qm35 transport layer.
 * @qm35: The qm35 device.
 * @type: The type of packet.
 * @data: Pointer to the start of the packet.
 * @length: Size of the packet in bytes.
 *
 * This function is used by the UCI probing and other modules to send messages
 * to QM35 using the transport driver.
 *
 * If an high-priority handler is registered for this message type, the function
 * returns ``-EBUSY`` because the bypass API has priority over everything else
 * for the selected message type.
 *
 * On its lower layer, it uses the functions exported by the low-level hardware
 * transport drivers in its ``struct qm35_transport_ops`` structure.
 *
 * This function may be called BEFORE the ``qm35_transport_start()`` was called,
 * so the underlying transport implementation MUST allow temporary power-on the
 * device.
 *
 * Returns: if the bypass is opened for the given type, then return -EBUSY,
 * else the return code from the transport layer.
 */
int qm35_transport_send(struct qm35 *qm35, enum qm35_transport_msg_type type,
			const void *data, size_t length)
{
	int ret = qm35_transport_has_high_prio(qm35, type);
	if (ret)
		return ret < 0 ? ret : -EBUSY;
	return qm35->transport_ops->send(qm35, type, data, length);
}
EXPORT_SYMBOL(qm35_transport_send);

/**
 * qm35_transport_get_registered() - Get registered callback for a type.
 * @qm35: The QM35 device.
 * @type: The type of message.
 * @out: Address where to store callback & associated data.
 *
 * Return highest priority handler for a given message type.
 *
 * Return: 0 on success else a negative error.
 */
static int
qm35_transport_get_registered(struct qm35 *qm35,
			      enum qm35_transport_msg_type type,
			      struct qm35_transport_recv_handler *out)
{
	int ret = 0;
	unsigned idx;

	if (type >= QM35_TRANSPORT_MSG_MAX)
		return -EINVAL;
	/* Start lookup with high prio handler. */
	idx = type * QM35_TRANSPORT_PRIO_COUNT + QM35_TRANSPORT_PRIO_HIGH;

	/* To avoid race condition in qm35_transport_event(), we need to
	 * ensure no concurrent access on this table. */
	spin_lock(&qm35->lock);
	if (!qm35->rx_handlers[idx].cb)
		idx++;
	if (qm35->rx_handlers[idx].cb) {
		*out = qm35->rx_handlers[idx];
	} else {
		ret = -ENOENT;
	}
	spin_unlock(&qm35->lock);
	return ret;
}

/**
 * qm35_transport_probe() - Probe the device.
 * @qm35: The QM35 device.
 * @infobuf: The device info buffer where to store the obtained info.
 * @len: Size of the device info buffer.
 *
 * This function allows to probe a device and fills a struct with the device
 * info if successful.
 *
 * Returns: The device info size on success else a negative error.
 */
int qm35_transport_probe(struct qm35 *qm35, char *infobuf, size_t len)
{
	int rc = 0;

	if (qm35->transport_ops->probe)
		rc = qm35->transport_ops->probe(qm35, infobuf, len);
	return rc;
}

/**
 * qm35_transport_event() - Receive an event from the transport layer.
 * @qm35: The qm35 device.
 * @transport_event: The type of transport event received.
 *
 * Called by low-level hardware driver module to inform the core that an IRQ is
 * received.
 *
 * It allocates a new ``struct sk_buff`` and receives into it the pending QM35
 * message using the ``recv()`` transport callback.
 *
 * Then, it calls the registered handler for the received packet type.
 *
 * Returns: 0 on success else a negative error.
 */
int qm35_transport_event(struct qm35 *qm35,
			 enum qm35_transport_events transport_event)
{
	struct qm35_transport_recv_handler handler;
	enum qm35_transport_msg_type type;
	struct sk_buff *skb;
	int flags, rc;

	if (!qm35)
		return -EINVAL;

	/* Allocate buffer to receive frame */
	skb = alloc_skb(QM35_MAX_PACKET_SIZE, GFP_KERNEL);
	if (!skb)
		return -ENOMEM;

	/* Receive frame from transport driver implementation. */
	rc = qm35->transport_ops->recv(qm35, skb->data, skb_availroom(skb),
				       &type, &flags);
	if (rc < 0)
		goto error;
	/* Update packet with received payload length. */
	skb_put(skb, rc);
	/* Store additional information. */
	skb->cb[0] = type;
	skb->cb[1] = flags;
	skb->cb[2] = transport_event;

	/* Check if a handler is registered */
	if (qm35_transport_get_registered(qm35, type, &handler) < 0) {
		rc = -EOPNOTSUPP;
		goto error;
	}
	/* Handle received packet with the desired handler */
	handler.cb(handler.data, skb);
	return 0;

error:
	kfree_skb(skb);
	return rc;
}
EXPORT_SYMBOL(qm35_transport_event);

/**
 * qm35_transport_register() - Register a callback for a message type.
 * @qm35: The QM35 device.
 * @type: The type of msg received.
 * @prio: The callback priority.
 * @callback: The callback you want to register.
 * @data: User data pointer that will be sent back on callback calls.
 *
 * This function is used by the UCI client and other auxiliary modules to
 * register a message reception handler for a specific type of message.
 *
 * Registered callbacks are called by ``qm35_transport_event()`` according to
 * the received message type.
 *
 * Callback takes ownership of the received packet so it must always free the
 * packet.
 *
 * Return: 0 on success else a negative error.
 */
int qm35_transport_register(struct qm35 *qm35,
			    enum qm35_transport_msg_type type,
			    enum qm35_transport_priority prio,
			    qm35_transport_recv_cb callback, void *data)
{
	int ret = 0;
	unsigned idx;

	if (!qm35 || !callback || type >= QM35_TRANSPORT_MSG_MAX ||
	    prio >= QM35_TRANSPORT_PRIO_COUNT)
		return -EINVAL;
	/* No test for idx since type & prio already tested. */
	idx = type * QM35_TRANSPORT_PRIO_COUNT + prio;

	/* To avoid race condition in qm35_transport_event(), we need to
	 * ensure no concurrent access on this table. */
	spin_lock(&qm35->lock);
	if (!qm35->rx_handlers[idx].cb) {
		qm35->rx_handlers[idx].cb = callback;
		qm35->rx_handlers[idx].data = data;
	} else {
		ret = -EEXIST;
	}
	spin_unlock(&qm35->lock);
	return ret;
}
EXPORT_SYMBOL(qm35_transport_register);

/**
 * qm35_transport_unregister() - Unregister the callback for a message type.
 * @qm35: The QM35 device.
 * @type: The type of msg received.
 * @prio: The priority of callback to remove.
 * @callback: The callback you want to unregister.
 *
 * Check the given callback is actually the registered callback and remove it
 * in that case. This message type can be registered again.
 *
 * Return: 0 on success else a negative error.
 */
int qm35_transport_unregister(struct qm35 *qm35,
			      enum qm35_transport_msg_type type,
			      enum qm35_transport_priority prio,
			      qm35_transport_recv_cb callback)
{
	int ret = 0;
	unsigned idx;

	if (!qm35 || !callback || type >= QM35_TRANSPORT_MSG_MAX ||
	    prio >= QM35_TRANSPORT_PRIO_COUNT)
		return -EINVAL;
	idx = type * QM35_TRANSPORT_PRIO_COUNT + prio;

	/* To avoid race condition in qm35_transport_event(), we need to
	 * ensure no concurrent access on this table. */
	spin_lock(&qm35->lock);
	if (qm35->rx_handlers[idx].cb == callback) {
		qm35->rx_handlers[idx].cb = NULL;
		qm35->rx_handlers[idx].data = NULL;
	} else {
		ret = -EBADF;
	}
	spin_unlock(&qm35->lock);
	return ret;
}
EXPORT_SYMBOL(qm35_transport_unregister);
