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
#ifndef __QM35_SPI_THREAD_H
#define __QM35_SPI_THREAD_H

#include <linux/workqueue.h>
#include <linux/mutex.h>

struct qm35_spi;

/* Pending work bits */
enum {
	QM35_GENERIC_WORK = BIT(0),
	QM35_IRQ_WORK = BIT(1),
};

/* Custom function for command */
typedef int (*work_func)(struct qm35_spi *qmspi, const void *in, void *out);

/**
 * struct qm35_work - Generic command descriptor.
 * @cmd: Function to execute in worker thread context.
 * @in: Input parameters.
 * @out: Output buffer for results.
 * @ret: Function return value.
 */
struct qm35_work {
	work_func cmd;
	const void *in;
	void *out;
	int ret;
};

/**
 * struct qm35_worker - QM35 worker thread data.
 * @thread: Worker thread.
 * @pending_work: Pending work bitmap to execute.
 * @work: Generic work to execute.
 * @work_wq: Work wait queue.
 * @mtx: Mutex protecting work field in this structure.
 */
struct qm35_worker {
	struct task_struct *thread;
	unsigned long pending_work;
	struct qm35_work *work;
	wait_queue_head_t work_wq;
	struct mutex mtx;
};

/* API */
int qm35_thread_run(struct qm35_spi *qmspi, int cpu);
void qm35_thread_stop(struct qm35_spi *qmspi);

int qm35_enqueue(struct qm35_spi *qmspi, struct qm35_work *cmd);
void qm35_enqueue_irq(struct qm35_spi *qmspi);

#endif /* __QM35_SPI_THREAD_H */
