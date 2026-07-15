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
#ifndef __QM35_SPI_FW_H
#define __QM35_SPI_FW_H

#include "qmrom.h"
#include "qmrom_log.h"

/**
 * enum qm35_fw_format - qm35 firmware format.
 * @QM35FW_STITCHED: Stitched firmware used by the QM357xx.
 * @QM35FW_MACRO_PKG: Macro firmware package used by the QM357xx
 *                   (depends on firmware updater).
 * @QM35FW_FW_PKG: Firmware package used by the QM358xx.
 */
enum qm35_fw_format {
	QM35FW_STITCHED,
	QM35FW_MACRO_PKG,
	QM35FW_FW_PKG,
};

/**
 * struct qm35_firmware - qm35 firmware.
 * @update_lock: Mutex protecting firmware update.
 * @handle: libqmrom handle used to communicate with the bootrom..
 * @fw_img: Firmware binary data.
 * @is_loaded: Flag that indicates that a firmware is currently loaded.
 * @fw_format: Format of the currently loaded firmware.
 */
struct qm35_firmware {
	struct mutex update_lock;
	struct qmrom_handle *handle;
	const struct firmware *fw_img;
	bool is_loaded;
	enum qm35_fw_format fw_format;
};

int qm35_fw_upgrade(struct qm35_spi *qmspi);
int qm35_fw_get_device_id(struct qm35_spi *qmspi);
void qm35_fw_deinit_qmrom(struct qm35_spi *qmspi);
int qm35_fw_load(struct qm35_spi *qmspi, const char *fw_name);
int qm35_fw_free(struct qm35_spi *qmspi);
int qm35_fw_get_vendor_version(struct qm35_spi *qmspi,
			       struct qm35_fw_version *version);

#ifdef QM35_SPI_FW_TESTS

/* Declare our wrapper functions */
static struct device *ku_get_device(struct device *dev);
static void ku_put_device(struct device *dev);
static int ku_request_firmware_direct(const struct firmware **fw,
				      const char *name, struct device *device);
static void ku_release_firmware(const struct firmware *fw);
static void ku_qmrom_set_log_device(struct device *dev, enum log_level_e lvl);
struct qmrom_handle *ku_qmrom_init(void *spi_handle, void *reset_handle,
				   void *ss_irq_handle, int spi_speed,
				   int comms_retries, reset_device_fn reset,
				   enum device_generation_e dev_gen_hint);
static int ku_qm358xx_rom_load_fw_pkg(struct qmrom_handle *handle,
				      const struct firmware *fw);
static int ku_qm357xx_rom_flash_fw(struct qmrom_handle *handle,
				   const struct firmware *fw,
				   macro_pkg_img_t image_no);
static int ku_qm357xx_rom_fw_macro_pkg_get_fw_idx(const struct firmware *fw,
						  int idx, uint32_t *fw_size,
						  const uint8_t **fw_data);

static void ku_qmrom_deinit(struct qmrom_handle *handle);
static int ku_qmrom_spi_reset_device(void *reset_handle);
static void ku_qmrom_spi_set_freq(unsigned int freq);
static int ku_run_fwupdater(struct qmrom_handle *handle, const char *fwpkg_bin,
			    size_t size);
static int ku_qm35_enqueue(struct qm35_spi *qmspi, struct qm35_work *cmd);
static int ku_qm35_spi_reset_wait_ready(struct qm35_spi *qmspi, bool bootrom,
					bool wait);

/* Redefine some functions to use our test wrappers */
#define get_device ku_get_device
#define put_device ku_put_device
#define request_firmware_direct ku_request_firmware_direct
#define release_firmware ku_release_firmware
#define qmrom_set_log_device ku_qmrom_set_log_device
#define qmrom_init ku_qmrom_init
#define qm358xx_rom_load_fw_pkg ku_qm358xx_rom_load_fw_pkg
#define qm357xx_rom_flash_fw ku_qm357xx_rom_flash_fw
#define qm357xx_rom_fw_macro_pkg_get_fw_idx \
	ku_qm357xx_rom_fw_macro_pkg_get_fw_idx
#define qmrom_deinit ku_qmrom_deinit
#define qmrom_spi_reset_device ku_qmrom_spi_reset_device
#define qmrom_spi_set_freq ku_qmrom_spi_set_freq
#define run_fwupdater ku_run_fwupdater
#define qm35_enqueue ku_qm35_enqueue
#define qm35_spi_reset_wait_ready ku_qm35_spi_reset_wait_ready

#endif /* QM35_SPI_FW_TESTS */

#endif /* __QM35_SPI_FW_H */
