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
#ifndef QM35_NOTIFIER_H
#define QM35_NOTIFIER_H

#include <linux/notifier.h>
#include <linux/version.h>

struct qm35;

/**
 * enum qm35_notifier_events - The qm35 events that can be notified.
 * @QM35_NOTIFIER_EVENT_NEW: A new qm35 device has been created.
 * @QM35_NOTIFIER_EVENT_DELETE: An old qm35 device will be deleted.
 * @QM35_NOTIFIER_EVENT_ONLINE: A qm35 device is online.
 * @QM35_NOTIFIER_EVENT_MAX: Number of different events defined.
 *
 * Notes:
 * A notified block may only return NOTIFY_DONE from
 * QM35_NOTIFIER_EVENT_NEW & QM35_NOTIFIER_EVENT_ONLINE events.
 *
 * A notified block may return NOTIFY_DONE or NOTIFY_STOP from
 * a QM35_NOTIFIER_EVENT_DELETE event.  In the later case, the previously
 * notified blocks in the chain will be rolled back by being sent a new
 * QM35_NOTIFIER_EVENT_NEW event.  The device will be deleted only
 * if no notified block returns NOTIFY_STOP.
 */
enum qm35_notifier_events {
	QM35_NOTIFIER_EVENT_NEW,
	QM35_NOTIFIER_EVENT_DELETE,
	QM35_NOTIFIER_EVENT_ONLINE,
	QM35_NOTIFIER_EVENT_MAX,
	/* Update qm35_notifier_event_label() when adding/changing enums. */
};

int qm35_register_notifier(struct notifier_block *nb);
int qm35_unregister_notifier(struct notifier_block *nb);

void qm35_notifier_notify(struct qm35 *qm35, enum qm35_notifier_events event);

#ifdef QM35_NOTIFIER_TESTS

#include "qm35_ids.h"

int ku_qm35_foreach_id(qm35_ids_cb process, void *argument);
int ku_blocking_notifier_chain_register(struct blocking_notifier_head *nh,
					struct notifier_block *n);

#define qm35_foreach_id(p, a) ku_qm35_foreach_id(p, a)
#define blocking_notifier_chain_register(h, n) \
	ku_blocking_notifier_chain_register(h, n)

/* pr_warn is a macro expanding to a printk() call */
int ku_printk(const char *format, ...);

#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
#define _printk ku_printk
#else
#define printk ku_printk
#endif

/* Ensure modified functions aren't exported! */
#undef EXPORT_SYMBOL
#define EXPORT_SYMBOL(x)

#endif /* QM35_NOTIFIER_TESTS */

#endif /* QM35_NOTIFIER_H */
