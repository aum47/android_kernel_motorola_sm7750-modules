/*
 * This file is part of the UWB stack for linux.
 *
 * Copyright (c) 2020-2024 Qorvo US, Inc.
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
#if !defined(QM35_NOTIFIER_TRC_H) || defined(TRACE_HEADER_MULTI_READ)
#define QM35_NOTIFIER_TRC_H

#include "qm35_notifier.h"

/* QM35 notifier event */
#define NOTIFIER_EVENT_ENTRY __field(u8, event)
#define NOTIFIER_EVENT_ASSIGN(x) __entry->event = (x)
#define NOTIFIER_EVENT_PR_FMT ", event: %s"
#define notifier_event_name(name)                 \
	{                                         \
		QM35_NOTIFIER_EVENT_##name, #name \
	}
/* clang-format off */
#define NOTIFIER_EVENT_SYMBOLS			\
	notifier_event_name(NEW),		\
	notifier_event_name(DELETE),		\
	notifier_event_name(ONLINE)
/* clang-format on */
TRACE_DEFINE_ENUM(QM35_NOTIFIER_EVENT_NEW);
TRACE_DEFINE_ENUM(QM35_NOTIFIER_EVENT_DELETE);
TRACE_DEFINE_ENUM(QM35_NOTIFIER_EVENT_ONLINE);
#define NOTIFIER_EVENT_PR_ARG \
	__print_symbolic(__entry->event, NOTIFIER_EVENT_SYMBOLS)

#endif /* !QM35_NOTIFIER_TRC_H */
