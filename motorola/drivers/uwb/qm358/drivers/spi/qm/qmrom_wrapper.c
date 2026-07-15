/*
 * This file is part of the UWB stack for linux.
 *
 * Copyright (c) 2020-2021 Qorvo US, Inc.
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
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/gpio/consumer.h>

#include "qmrom_spi.h"
#include "qmrom_log.h"
#include "qm35_spi.h"
#include "qm35_spi_trc.h"
#include "spi_rom_protocol.h"

static unsigned int qmrom_spi_speed_hz;

/**
 * qmrom_spi_set_freq() - SPI set freq wrapper function for the libqmrom.
 * @freq: SPI bus frequency in Hz.
 */
void qmrom_spi_set_freq(unsigned int freq)
{
	trace_qmrom_spi_set_freq(freq);

	qmrom_spi_speed_hz = freq;
}

/**
 * qmrom_spi_transfer() - SPI transfer wrapper function for the libqmrom.
 * @handle: spi_device for this transfer.
 * @rbuf: Read buffer.
 * @wbuf: Write buffer.
 * @size: Size of the transfer.
 *
 * Return: zero on success, else a negative error code.
 */
int qmrom_spi_transfer(void *handle, char *rbuf, const char *wbuf, size_t size)
{
	struct spi_device *spi = *(struct spi_device **)handle;
	struct qm35_spi *qmspi = container_of(handle, struct qm35_spi, spi);
	int rc;

	struct spi_transfer xfer = {
		.tx_buf = wbuf,
		.rx_buf = rbuf,
		.len = size,
		.speed_hz = qmrom_spi_speed_hz,
	};

	trace_qmrom_spi_transfer(qmspi);

	/* Limit to 64 bytes to avoid dumping firmware data chunks. */
	if (((struct qm35_hsspi_header *)wbuf)->flags & HSSPI_HOST_WR &&
	    size < 64)
		trace_qm35_hsspi_data(qmspi, wbuf, size);
	trace_qm35_hsspi_host_header(qmspi, (struct qm35_hsspi_header *)wbuf);

	rc = spi_sync_transfer(spi, &xfer, 1);

	trace_qm35_hsspi_soc_header(qmspi, (struct qm35_hsspi_header *)rbuf);
	if (((struct qm35_hsspi_header *)rbuf)->flags & HSSPI_SOC_OA)
		trace_qm35_hsspi_data(qmspi, rbuf, size);

	trace_qmrom_spi_transfer_return(qmspi, rc);
	return rc;
}

/**
 * qmrom_spi_set_cs_level() - SPI CS level wrapper function for the libqmrom.
 * @handle: spi_device for this transfer.
 * @level: Level of the SPI CS line.
 *
 * Return: zero on success, else a negative error code.
 */
#if 0
int qmrom_spi_set_cs_level(void *handle, int level)
{
	struct spi_device *spi = *(struct spi_device **)handle;
	struct qm35_spi *qmspi = container_of(handle, struct qm35_spi, spi);
	int rc;

	struct spi_transfer xfer = {
		.cs_change = !level,
		.speed_hz = qmrom_spi_speed_hz,
	};

	trace_qmrom_spi_set_cs_level(qmspi, level);

	rc = spi_sync_transfer(spi, &xfer, 1);

	trace_qmrom_spi_set_cs_level_return(qmspi, rc);
	return rc;
}
#else
//add by motorola, begin
extern void qm35_set_csn_level(int level);
int qmrom_spi_set_cs_level(void *handle, int level)
{
	struct qm35_spi *qmspi = container_of(handle, struct qm35_spi, spi);
	trace_qmrom_spi_set_cs_level(qmspi, level);
	qm35_set_csn_level(level);
	trace_qmrom_spi_set_cs_level_return(qmspi, 0);
	return 0;
}
//add by motorola, end
#endif

/**
 * qmrom_spi_reset_device() - Hard reset wrapper function for the libqmrom.
 * @handle: gpio_desc used to hard reset the device.
 *
 * Return: 0.
 */
int qmrom_spi_reset_device(void *handle)
{
	struct gpio_desc *reset_gpio = *(struct gpio_desc **)handle;
	struct qm35_spi *qmspi =
		container_of(handle, struct qm35_spi, reset_gpio);

	trace_qmrom_spi_reset_device(qmspi);

	gpiod_set_value_cansleep(reset_gpio, 1);
	usleep_range(SPI_RST_LOW_DELAY_MS * USEC_PER_MSEC,
		     2 * SPI_RST_LOW_DELAY_MS * USEC_PER_MSEC);
	gpiod_set_value_cansleep(reset_gpio, 0);

	return 0;
}

/**
 * qmrom_spi_wait_for_irq_line() - SPI wait for IRQ line wrapper function for the libqmrom.
 * @handle: gpio_desc to wait on.
 * @timeout_ms: Timeout in ms.
 *
 * Return: zero on success, else a negative error code.
 */
int qmrom_spi_wait_for_irq_line(void *handle, unsigned int timeout_ms)
{
	struct gpio_desc *irq_gpio = *(struct gpio_desc **)handle;
	struct qm35_spi *qmspi =
		container_of(handle, struct qm35_spi, irq_gpio);
	bool irq;
	int rc;
	const int delay_us = 10;
	ktime_t start_time = ktime_get();
	ktime_t timeout = ktime_add_ms(start_time, timeout_ms);

	trace_qmrom_spi_wait_for_irq_line(qmspi);

	while (!(irq = gpiod_get_value(irq_gpio) > 0) &&
	       ktime_before(ktime_get(), timeout))
		usleep_range(delay_us, 2 * delay_us);

	rc = irq ? 0 : SPI_ERR_IRQ_LINE_TIMEOUT;

	trace_qmrom_spi_wait_for_irq_line_return(qmspi, rc);
	return rc;
}

/**
 * qmrom_spi_read_irq_line() - SPI read IRQ line wrapper function for the libqmrom.
 * @handle: gpio_desc to read from.
 *
 * Return: irq_gpio value if available, else 0.
 */
int qmrom_spi_read_irq_line(void *handle)
{
	struct gpio_desc *irq_gpio = *(struct gpio_desc **)handle;
	struct qm35_spi *qmspi =
		container_of(handle, struct qm35_spi, irq_gpio);
	int rc;

	trace_qmrom_spi_read_irq_line(qmspi);

	rc = gpiod_get_value(irq_gpio);

	trace_qmrom_spi_read_irq_line_return(qmspi, rc);
	return rc;
}

/**
 * qmrom_check_fw_boot_state() - Check firmware boot state wrapper function for the libqmrom.
 * @handle: libqmrom handle.
 * @timeout_ms: Timeout in ms.
 *
 * Return: 0.
 */
int qmrom_check_fw_boot_state(struct qmrom_handle *handle,
			      unsigned int timeout_ms)
{
	/* The driver reboots the QM35 and checks its state by itself. */
	return 0;
}
