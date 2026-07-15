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
#ifndef __QM35_IDS_H
#define __QM35_IDS_H

#include "qm35.h"

/**
 * typedef qm35_ids_cb - callback function type for qm35_foreach_id()
 * @qm35: the current qm35 instance provided by qm35_foreach_id()
 * @cb_arg: the cb_arg provided to qm35_foreach_id()
 *
 * Return: Zero on success, else a negative error code.
 */
typedef int (*qm35_ids_cb)(struct qm35 *qm35, void *cb_arg);

int qm35_new_id(struct qm35 *qm35);
int qm35_remove_id(int dev_id);
struct qm35 *qm35_find_by_id(int dev_id);
int qm35_foreach_id(qm35_ids_cb cb, void *cb_arg);

#ifdef QM35_IDS_TESTS

/* Declare our wrapper functions */
int ku_idr_alloc(struct idr *idr, void *ptr, int start, int end, gfp_t gfp);

/* Redefine some functions to use our test wrappers */
#define idr_alloc ku_idr_alloc

/* Ensure modified functions aren't exported! */
#undef EXPORT_SYMBOL
#define EXPORT_SYMBOL(x)

#endif /* QM35_IDS_TESTS */

#endif /* __QM35_IDS_H */
