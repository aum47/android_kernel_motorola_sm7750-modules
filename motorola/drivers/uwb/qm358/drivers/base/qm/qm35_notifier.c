/*
 * This file is part of the UWB stack for linux.
 *
 * Copyright (c) 2021-2024 Qorvo US, Inc.
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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/version.h>

#include "qm35_notifier.h"
#include "qm35_ids.h"
#include "qm35_trc.h"

/**
 * struct qm35_notifier_data - Data for qm35_foreach_id().
 * @nb: The notifier_block that shall be notified for each qm35 devices.
 * @event: The event to be notified to nb.
 */
struct qm35_notifier_data {
	struct notifier_block *nb;
	enum qm35_notifier_events event;
};

/* qm35_notifier_list: the list of registered notifiers. */
static BLOCKING_NOTIFIER_HEAD(qm35_notifier_list);

/**
 * qm35_notifier_event_label() - Return a string describing the given event.
 * @event: A qm35_notifier_event value.
 *
 * Return: A literal string.
 */
static const char *qm35_notifier_event_label(enum qm35_notifier_events event)
{
	static const char
		*const qm35_notifier_event_labels[QM35_NOTIFIER_EVENT_MAX] = {
			"QM35_NOTIFIER_EVENT_NEW",
			"QM35_NOTIFIER_EVENT_DELETE",
			"QM35_NOTIFIER_EVENT_ONLINE",
		};
	return qm35_notifier_event_labels[event];
}

/**
 * notifier_stop_warning() - Warns about NOTIFY_STOP.
 * @event: The qm35 notifier event that was sent to the notifier.
 * @rc: The bad result code returned by the notifier.
 */
static void notifier_stop_warning(enum qm35_notifier_events event, int rc)
{
	pr_warn("some notifier in qm35_notifier_list returned %d "
		"(event = %s) ; "
		"this breaks consistency amongst notifiers!\n",
		rc, qm35_notifier_event_label(event));
}

/**
 * qm35_foreach_notify_one_cb() - Calls the notifier with the specified event.
 * @qm35: The qm35 device.
 * @argument: A pointer to a struct qm35_notifier_data containing the
 *            notifier block and the event to notify.
 *
 * Notifies the notifier block in argument, of the specified event,
 * for the given qm35 device.
 *
 * Return: The result code from the notifier call.
 */
static int qm35_foreach_notify_one_cb(struct qm35 *qm35, void *argument)
{
	struct notifier_block *nb = argument;
	int rc, evt;

	evt = QM35_NOTIFIER_EVENT_NEW;
	trace_qm35_notify_one(nb, evt, qm35);
	rc = nb->notifier_call(nb, evt, qm35);
	if ((rc & NOTIFY_STOP_MASK) != 0)
		goto warn;
	evt = QM35_NOTIFIER_EVENT_ONLINE;
	trace_qm35_notify_one(nb, evt, qm35);
	rc = nb->notifier_call(nb, evt, qm35);
	if ((rc & NOTIFY_STOP_MASK) != 0)
		goto warn;
	return 0;
warn:
	notifier_stop_warning(evt, rc);
	return notifier_to_errno(rc);
}

/**
 * qm35_register_notifier() - Register a new notifier_block.
 * @nb: The notifier_block.
 *
 * Adds the notifier to the qm35_notifier_list, then
 * notifies the notifier, of all existing qm35 devices.
 *
 * Return: The notifier register return code, or 0 on success.
 */
int qm35_register_notifier(struct notifier_block *nb)
{
	int rc;

	trace_qm35_register_notifier(nb);

	rc = blocking_notifier_chain_register(&qm35_notifier_list, nb);
	if (rc)
		return rc;
	/* When registering a new notifier, we notify it of every QM35 device
	 * already registered.
	 */
	return qm35_foreach_id(qm35_foreach_notify_one_cb, nb);
}
EXPORT_SYMBOL(qm35_register_notifier);

/**
 * qm35_unregister_notifier() - Unregister an old notifier_block.
 * @nb: The notifier_block.
 *
 * Removes the notifier_block from the qm35_notifier_list.
 *
 * Return: The result code from the notifier unregister call.
 */
int qm35_unregister_notifier(struct notifier_block *nb)
{
	trace_qm35_unregister_notifier(nb);

	return blocking_notifier_chain_unregister(&qm35_notifier_list, nb);
}
EXPORT_SYMBOL(qm35_unregister_notifier);

/**
 * qm35_notifier_notify() - Notify the notifiers in the chain.
 * @qm35: The qm35 device.
 * @event: The qm35 event to notify.
 *
 * If a notifier returns a stopping result, then
 * either a warning is printed about it if this breaks consistency
 * (eg. for QM35_NOTIFIER_EVENT_NEW), or the notification is reverted.
 */
void qm35_notifier_notify(struct qm35 *qm35, enum qm35_notifier_events event)
{
	int rc;

	trace_qm35_notifier_notify(qm35, event);

	rc = blocking_notifier_call_chain(&qm35_notifier_list, event, qm35);
	if (0 != (rc & NOTIFY_STOP_MASK))
		notifier_stop_warning(event, rc);
	trace_qm35_notifier_notify_return(qm35, rc);
}
