/*
 * This file is part of the UWB stack for linux.
 *
 * Copyright (c) 2020-2023 Qorvo US, Inc.
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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM qm35_uci_dev

#if !defined(__QM35_UCI_DEV_TRACE) || defined(TRACE_HEADER_MULTI_READ)
#define __QM35_UCI_DEV_TRACE

#include <linux/tracepoint.h>

/* For qm35_get_dev_id() call. */
#include "qm35_core.h"

#define QM_ENTRY __field(int, qm_id)
#define QM_ASSIGN __entry->qm_id = 0
#define QM_PR_FMT "hw#%d"
#define QM_PR_ARG __entry->qm_id

/* We don't want clang-format to modify the following events definition!
   Look at net/wireless/trace.h for the required format. */
/* clang-format off */

/*****************************************
 *	QM35 UCI_DEV functions traces    *
 *****************************************/

TRACE_EVENT(qm35_uci_dev_open,
	TP_PROTO(struct qm35 *qm),
	TP_ARGS(qm),
	TP_STRUCT__entry(
		QM_ENTRY
	),
	TP_fast_assign(
		QM_ASSIGN;
	),
	TP_printk(QM_PR_FMT, QM_PR_ARG)
);

/* return the device id */
TRACE_EVENT(qm35_uci_dev_open_return,
	TP_PROTO(struct qm35 *qm, int ret),
	TP_ARGS(qm, ret),
	TP_STRUCT__entry(
		QM_ENTRY
		__field(int, ret)
	),
	TP_fast_assign(
		QM_ASSIGN;
		__entry->ret = ret;
	),
	TP_printk(QM_PR_FMT ", return: %d", QM_PR_ARG, __entry->ret)
);

TRACE_EVENT(qm35_uci_dev_close,
	TP_PROTO(struct qm35 *qm),
	TP_ARGS(qm),
	TP_STRUCT__entry(
		QM_ENTRY
	),
	TP_fast_assign(
		QM_ASSIGN;
	),
	TP_printk(QM_PR_FMT, QM_PR_ARG)
);

TRACE_EVENT(qm35_uci_dev_close_return,
	TP_PROTO(struct qm35 *qm, int ret),
	TP_ARGS(qm, ret),
	TP_STRUCT__entry(
		QM_ENTRY
		__field(int, ret)
	),
	TP_fast_assign(
		QM_ASSIGN;
		__entry->ret = ret;
	),
	TP_printk(QM_PR_FMT ", return: %d", QM_PR_ARG, __entry->ret)
);

TRACE_EVENT(qm35_uci_dev_read,
	TP_PROTO(struct qm35 *qm),
	TP_ARGS(qm),
	TP_STRUCT__entry(
		QM_ENTRY
	),
	TP_fast_assign(
		QM_ASSIGN;
	),
	TP_printk(QM_PR_FMT, QM_PR_ARG)
);

TRACE_EVENT(qm35_uci_dev_read_return,
	TP_PROTO(struct qm35 *qm, int ret),
	TP_ARGS(qm, ret),
	TP_STRUCT__entry(
		QM_ENTRY
		__field(int, ret)
	),
	TP_fast_assign(
		QM_ASSIGN;
		__entry->ret = ret;
	),
	TP_printk(QM_PR_FMT ", return: %d", QM_PR_ARG, __entry->ret)
);

TRACE_EVENT(qm35_uci_dev_write,
	TP_PROTO(struct qm35 *qm),
	TP_ARGS(qm),
	TP_STRUCT__entry(
		QM_ENTRY
	),
	TP_fast_assign(
		QM_ASSIGN;
	),
	TP_printk(QM_PR_FMT, QM_PR_ARG)
);

TRACE_EVENT(qm35_uci_dev_write_return,
	TP_PROTO(struct qm35 *qm, int ret),
	TP_ARGS(qm, ret),
	TP_STRUCT__entry(
		QM_ENTRY
		__field(int, ret)
	),
	TP_fast_assign(
		QM_ASSIGN;
		__entry->ret = ret;
	),
	TP_printk(QM_PR_FMT ", return: %d", QM_PR_ARG, __entry->ret)
);

TRACE_EVENT(qm35_uci_dev_poll,
	TP_PROTO(struct qm35 *qm),
	TP_ARGS(qm),
	TP_STRUCT__entry(
		QM_ENTRY
	),
	TP_fast_assign(
		QM_ASSIGN;
	),
	TP_printk(QM_PR_FMT, QM_PR_ARG)
);

TRACE_EVENT(qm35_uci_dev_poll_return,
	TP_PROTO(struct qm35 *qm, int ret),
	TP_ARGS(qm, ret),
	TP_STRUCT__entry(
		QM_ENTRY
		__field(int, ret)
	),
	TP_fast_assign(
		QM_ASSIGN;
		__entry->ret = ret;
	),
	TP_printk(QM_PR_FMT ", return: %d", QM_PR_ARG, __entry->ret)
);

/* clang-format on */
#endif /* !__QM35_UCI_DEV_TRACE || TRACE_HEADER_MULTI_READ */

#undef TRACE_INCLUDE_PATH
/* clang-format off */
#define TRACE_INCLUDE_PATH .
/* clang-format on */
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE qm35_uci_dev_trc
#include <trace/define_trace.h>
