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
#include <linux/mutex.h>
#include <linux/idr.h>

#include "qm35_ids.h"

static DEFINE_IDR(qm35_ids);
static DEFINE_MUTEX(qm35_ids_lock);

/**
 * qm35_new_id() - allocate a new device id for a qm35 struct
 * @qm35: a qm35 struct to which to allocate an id.
 *
 * Allocate a new device id and set it in the dev_id member of the qm35 struct.
 *
 * Context: Called from bus device probing or module init functions.
 * Return: Zero on success, else -ENOMEM if memory allocation failed,
 * -ENOSPC if no free IDs could be found or -EINVAL if qm35 is NULL.
 */
int qm35_new_id(struct qm35 *qm35)
{
	int rc;

	if (qm35 == NULL)
		return -EINVAL;
	mutex_lock(&qm35_ids_lock);
	rc = idr_alloc(&qm35_ids, qm35, 0, QM35_NUM_DEVICES, GFP_KERNEL);
	mutex_unlock(&qm35_ids_lock);
	if (rc < 0)
		return rc;
	qm35->dev_id = rc;
	return 0;
}

/**
 * qm35_remove_id() - delete a previously allocated device id
 * @dev_id: the device id to delete.
 *
 * Delete a previous allocated device id.
 *
 * Context: Called from the module exit function.
 * Return: Zero on success, else -EINVAL if the device id is not valid.
 */
int qm35_remove_id(int dev_id)
{
	struct qm35 *qm35;

	mutex_lock(&qm35_ids_lock);
	qm35 = idr_remove(&qm35_ids, dev_id);
	mutex_unlock(&qm35_ids_lock);
	if (qm35 == NULL)
		return -EINVAL;
	return 0;
}

/**
 * qm35_find_by_id() - returns a qm35 instance from its id
 * @dev_id: the device id of the requested qm35 instance.
 *
 * Returns a qm35 instance from its id.
 *
 * Context: User context or kernel thread context.
 * Return: Pointer on the requested qm35 instance on success, else -EINVAL if
 * the device id is not valid.
 */
struct qm35 *qm35_find_by_id(int dev_id)
{
	struct qm35 *qm35;

	mutex_lock(&qm35_ids_lock);
	qm35 = idr_find(&qm35_ids, dev_id);
	mutex_unlock(&qm35_ids_lock);
	if (qm35 == NULL)
		return ERR_PTR(-EINVAL);
	return qm35;
}

/**
 * qm35_foreach_id() - iterates over all the q35 instances
 * @cb: the callback function called for each q35 instance
 * @cb_arg: the argument of the callback function
 *
 * Iterates over all the q35 instances by calling cb() for each q35 instance.
 *
 * Context: Any context.
 * Return: Zero on success, else the return value of the first call of the
 * callback function that returns a non zero value.
 */
int qm35_foreach_id(qm35_ids_cb cb, void *cb_arg)
{
	struct qm35 *qm35;
	int dev_id;
	int rc = 0;

	mutex_lock(&qm35_ids_lock);
	idr_for_each_entry (&qm35_ids, qm35, dev_id) {
		rc = cb(qm35, cb_arg);
		if (rc)
			break;
	}
	mutex_unlock(&qm35_ids_lock);
	return rc;
}
