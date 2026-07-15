/*
 * This file is part of the UWB stack for linux.
 *
 * Copyright (c) 2020-2022 Qorvo US, Inc.
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
#if !defined(QM35_BYPASS_TRC_H) || defined(TRACE_HEADER_MULTI_READ)
#define QM35_BYPASS_TRC_H

/* QM35 bypass event */

#define BYPASS_EVENT_ENTRY __field(u8, event)
#define BYPASS_EVENT_ASSIGN(x) __entry->event = (x)
#define BYPASS_EVENT_PR_FMT ", event: %s"
#define bypass_event_name(name)           \
	{                                 \
		QM35_BYPASS_##name, #name \
	}
/* clang-format off */
#define BYPASS_EVENT_SYMBOLS			\
	bypass_event_name(IRQ),			\
	bypass_event_name(NOTIFICATION),	\
	bypass_event_name(RESPONSE)
/* clang-format on */
TRACE_DEFINE_ENUM(QM35_BYPASS_IRQ);
TRACE_DEFINE_ENUM(QM35_BYPASS_NOTIFICATION);
TRACE_DEFINE_ENUM(QM35_BYPASS_RESPONSE);
#define BYPASS_EVENT_TYPE_PR_ARG \
	__print_symbolic(__entry->event, BYPASS_EVENT_SYMBOLS)

/* QM35 bypass action */

#define BYPASS_ACTION_ENTRY __field(u8, action)
#define BYPASS_ACTION_ASSIGN(x) __entry->action = (x)
#define BYPASS_ACTION_PR_FMT ", action: %s"
#define bypass_action_name(name)                 \
	{                                        \
		QM35_BYPASS_ACTION_##name, #name \
	}
/* clang-format off */
#define BYPASS_ACTION_SYMBOLS			\
	bypass_action_name(RESET),		\
	bypass_action_name(FWUPD),		\
	bypass_action_name(POWER),		\
	bypass_action_name(MSG_TYPE)
/* clang-format on */
TRACE_DEFINE_ENUM(QM35_BYPASS_ACTION_RESET);
TRACE_DEFINE_ENUM(QM35_BYPASS_ACTION_FWUPD);
TRACE_DEFINE_ENUM(QM35_BYPASS_ACTION_POWER);
TRACE_DEFINE_ENUM(QM35_BYPASS_ACTION_MSG_TYPE);
#define BYPASS_ACTION_PR_ARG \
	__print_symbolic(__entry->action, BYPASS_ACTION_SYMBOLS)

#endif /* !QM35_BYPASS_TRC_H || TRACE_HEADER_MULTI_READ */
