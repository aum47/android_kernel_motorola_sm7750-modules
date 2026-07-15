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
#ifndef __QM35_UCI_PROBE_H
#define __QM35_UCI_PROBE_H

#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/types.h>

/**
 * struct qm35_uci_probing - QM35 probing data.
 * @lock: Mutex protecting command execution.
 * @completion: Command completion.
 * @status: Command status.
 * @cmd_data: Command user_data pointer.
 * @cmd_data_sz: Command user_data size.
 * @cmd_sent: Counter to know if command has been sent and has to be received.
 */
struct qm35_uci_probing {
	struct mutex lock;
	struct completion completion;
	uint8_t status;
	void *cmd_data;
	size_t cmd_data_sz;
	atomic_t cmd_sent;
};

struct qm35_spi;
void qm35_uci_probe_setup(struct qm35_spi *qmspi);
void qm35_uci_probe_cleanup(struct qm35_spi *qmspi);

int qm35_uci_probe_device_reset(struct qm35_spi *qmspi);

struct qm35_uci_device_info;
int qm35_uci_probe_device_info(struct qm35_spi *qmspi,
			       struct qm35_uci_device_info *info,
			       size_t info_sz);

#endif /* __QM35_UCI_PROBE_H */
