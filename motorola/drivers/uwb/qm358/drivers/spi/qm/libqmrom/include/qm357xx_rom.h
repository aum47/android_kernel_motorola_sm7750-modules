// SPDX-License-Identifier: GPL-2.0 OR Apache-2.0
/*
 * Copyright 2021 Qorvo US, Inc.
 *
 */

#ifndef __QM357XX_ROM_H__
#define __QM357XX_ROM_H__

#define PEG_ERR_TIMEOUT PEG_ERR_BASE - 1
#define PEG_ERR_ROM_NOT_READY PEG_ERR_BASE - 2
#define PEG_ERR_SEND_CERT_WRITE PEG_ERR_BASE - 3
#define PEG_ERR_WRONG_REVISION PEG_ERR_BASE - 4
#define PEG_ERR_FIRST_KEY_CERT_OR_FW_VER PEG_ERR_BASE - 5

#define HBK_LOC 12
typedef enum {
	HBK_2E_ICV,
	HBK_2E_OEM,
	HBK_1E_ICV_OEM,
} hbk_t;

#define ROM_VERSION_A0 0x01a0
#define ROM_VERSION_B0 0xb000

#define QM357XX_ROM_SOC_ID_LEN 0x20
#define QM357XX_ROM_UUID_LEN 0x10

/* Life cycle state definitions. */

/*! Defines the CM life-cycle state value. */
#define CC_BSV_CHIP_MANUFACTURE_LCS 0x0
/*! Defines the DM life-cycle state value. */
#define CC_BSV_DEVICE_MANUFACTURE_LCS 0x1
/*! Defines the Secure life-cycle state value. */
#define CC_BSV_SECURE_LCS 0x5
/*! Defines the RMA life-cycle state value. */
#define CC_BSV_RMA_LCS 0x7

struct unstitched_firmware {
	struct firmware *fw_img;
	struct firmware *fw_crt;
	struct firmware *key1_crt;
	struct firmware *key2_crt;
};

struct qm357xx_soc_infos {
	uint8_t soc_id[QM357XX_ROM_SOC_ID_LEN];
	uint8_t uuid[QM357XX_ROM_UUID_LEN];
	uint32_t lcs_state;
};

typedef enum {
	NOT_MACRO_PKG = -1,
	FW_UPDATER = 0,
	OEM_FW_IMAGE = 1,
} macro_pkg_img_t;

int qm357xx_rom_unstitch_fw(const struct firmware *fw,
			    struct unstitched_firmware *unstitched_fw,
			    enum chip_revision_e revision);
int qm357xx_rom_fw_macro_pkg_get_fw_idx(const struct firmware *fw, int idx,
					uint32_t *fw_size,
					const uint8_t **fw_fata);
int qm357xx_rom_unpack_fw_macro_pkg(const struct firmware *fw,
				    struct unstitched_firmware *all_fws,
				    macro_pkg_img_t image_no);
int qm357xx_rom_unpack_fw_pkg(const struct firmware *fw_pkg,
			      struct unstitched_firmware *all_fws);
int qm357xx_rom_flash_dbg_cert(struct qmrom_handle *handle,
			       struct firmware *dbg_cert);
int qm357xx_rom_erase_dbg_cert(struct qmrom_handle *handle);
int qm357xx_rom_flash_fw(struct qmrom_handle *handle, const struct firmware *fw,
			 macro_pkg_img_t image_no);
int qm357xx_rom_flash_unstitched_fw(struct qmrom_handle *handle,
				    const struct unstitched_firmware *fw);

#endif /* __QM357XX_ROM_H__ */
