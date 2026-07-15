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
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include "qm35_spi.h"
#include "qm35_spi_trc.h"

/* First version with sched_setattr_nocheck: v4.16-rc1~164^2~5 */
#if (KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE)
#include <uapi/linux/sched/types.h>
#endif

static inline int qm35_set_sched_attr(struct task_struct *p)
{
#if (KERNEL_VERSION(5, 9, 0) > LINUX_VERSION_CODE)
	struct sched_param sched_par = { .sched_priority = MAX_RT_PRIO - 2 };
	/* Increase thread priority */
	return sched_setscheduler(p, SCHED_FIFO, &sched_par);
#else
#if defined(CONFIG_QM35_FORCE_SCHED_PRIO_IN_DRIVER)
	struct sched_attr attr = { .sched_policy = SCHED_FIFO,
				   .sched_priority = MAX_RT_PRIO - 2,
				   .sched_flags = SCHED_FLAG_UTIL_CLAMP_MIN,
				   .sched_util_min = SCHED_CAPACITY_SCALE };
	return sched_setattr_nocheck(p, &attr);
#else
	/* Priority must be set by user-space now. */
	sched_set_fifo(p);
	return 0;
#endif
#endif
}

/**
 * qm35_enqueue() - Queue a generic work.
 * @qmspi: QM35 SPI instance.
 * @cmd: The work command to execute.
 *
 * Execute a generic work inside the thread context of the worker.
 *
 * The executed command MUST be responsible of freeing the memory it uses, not
 * the caller, as qm35_enqueue() might be interrupted by a signal.
 *
 * Returns: %-EINTR if it was interrupted by a signal, else the value returned
 * by the executed function.
 */
int qm35_enqueue(struct qm35_spi *qmspi, struct qm35_work *cmd)
{
	struct qm35_worker *wk = &qmspi->worker;
	unsigned long flags;
	int work = QM35_GENERIC_WORK;

	if (current == wk->thread) {
		/* We can't enqueue a new work from the same context and wait,
		   but it can be executed directly instead. */
		trace_qm35_enqueue_cmd_exec(qmspi, cmd);
		cmd->ret = cmd->cmd(qmspi, cmd->in, cmd->out);
		goto done;
	}
	trace_qm35_enqueue_cmd(qmspi, cmd);
	/* Ensure no other call of qm35_enqueue() is in progress. */
	if (mutex_lock_interruptible(&wk->mtx) == -EINTR) {
		dev_err(qmspi->base.dev,
			"Work enqueuing interrupted by signal");
		cmd->ret = -EINTR;
		goto done;
	}
	/* Slow path if not in worker thread context */
	spin_lock_irqsave(&wk->work_wq.lock, flags);
	wk->pending_work |= work;
	wk->work = cmd;
	wake_up_locked(&wk->work_wq);
	if (wait_event_interruptible_locked_irq(
		    wk->work_wq, !(wk->pending_work & work)) == -ERESTARTSYS) {
		dev_err(qmspi->base.dev,
			"Waiting for work completion interrupted by signal");
		wk->pending_work &= ~work;
		wk->work = NULL;
		cmd->ret = -EINTR;
	}
	spin_unlock_irqrestore(&wk->work_wq.lock, flags);
	mutex_unlock(&wk->mtx);
done:
	trace_qm35_enqueue_return(qmspi, cmd->ret);
	return cmd->ret;
}

/**
 * qm35_enqueue_irq() - Queue an IRQ work.
 * @qmspi: QM35 SPI instance.
 *
 * Queue an IRQ work to be executed as soon as possible by the worker thread.
 */
void qm35_enqueue_irq(struct qm35_spi *qmspi)
{
	struct qm35_worker *wk = &qmspi->worker;
	unsigned long flags;

	trace_qm35_enqueue_irq(qmspi);
	spin_lock_irqsave(&wk->work_wq.lock, flags);
	if (!(wk->pending_work & QM35_IRQ_WORK)) {
		wk->pending_work |= QM35_IRQ_WORK;
		disable_irq_nosync(qmspi->spi->irq);
	}
	wake_up_locked(&wk->work_wq);
	spin_unlock_irqrestore(&wk->work_wq.lock, flags);
}

/**
 * qm35_dequeue() - Remove pending work.
 * @qmspi: QM35 SPI instance.
 * @work: Handled work to remove.
 * @cmd: Descriptor of the executed command.
 * @ret: Return value of the executed command.
 */
static void qm35_dequeue(struct qm35_spi *qmspi, unsigned long work,
			 struct qm35_work *cmd, int ret)
{
	struct qm35_worker *wk = &qmspi->worker;
	unsigned long flags;

	spin_lock_irqsave(&wk->work_wq.lock, flags);
	/* Only access cmd if it is still valid. Otherwise, it indicates that
	 * the calling qm35_enqueue() has been interrupted by a signal.
	 */
	if (cmd == wk->work) {
		cmd->ret = ret;
		wk->pending_work &= ~work;
		wake_up_locked(&wk->work_wq);
	}
	spin_unlock_irqrestore(&wk->work_wq.lock, flags);
}

/**
 * qm35_clear_irq() - Clear IRQ work.
 * @qmspi: QM35 SPI instance.
 *
 * Clear IRQ work from pending and re-enable IRQ.
 */
static void qm35_clear_irq(struct qm35_spi *qmspi)
{
	struct qm35_worker *wk = &qmspi->worker;
	unsigned long flags;

	spin_lock_irqsave(&wk->work_wq.lock, flags);
	wk->pending_work &= ~QM35_IRQ_WORK;
	enable_irq(qmspi->spi->irq);
	spin_unlock_irqrestore(&wk->work_wq.lock, flags);
}

/**
 * qm35_wait_pending_work() - Wait for new work.
 * @qmspi: QM35 SPI instance.
 *
 * Wait for new work to handle.
 */
static void qm35_wait_pending_work(struct qm35_spi *qmspi)
{
	struct qm35_worker *wk = &qmspi->worker;
	unsigned long flags;

	spin_lock_irqsave(&wk->work_wq.lock, flags);
	wait_event_interruptible_locked_irq(
		wk->work_wq, wk->pending_work || kthread_should_stop());
	spin_unlock_irqrestore(&wk->work_wq.lock, flags);
}

/**
 * qm35_get_pending_work() - Read pending work.
 * @qmspi: QM35 SPI instance.
 *
 * Returns: Current pending work bitmap.
 */
static unsigned long qm35_get_pending_work(struct qm35_spi *qmspi)
{
	struct qm35_worker *wk = &qmspi->worker;
	unsigned long work;
	unsigned long flags;

	spin_lock_irqsave(&wk->work_wq.lock, flags);
	work = wk->pending_work;
	spin_unlock_irqrestore(&wk->work_wq.lock, flags);

	return work;
}

/**
 * qm35_spi_thread() - Event handling thread function.
 * @data: QM35 SPI instance.
 *
 * Process event (IRQ) and execute generic work.
 *
 * Returns: Always 0.
 */
static int qm35_spi_thread(void *data)
{
	struct qm35_spi *qmspi = data;
	struct qm35_worker *wk = &qmspi->worker;
	unsigned long pending_work = 0;

	dev_info(qmspi->base.dev, "Worker thread started\n");
	/* Run until stopped */
	while (!kthread_should_stop()) {
		/* Pending work items */
		pending_work = qm35_get_pending_work(qmspi);
		/* Check IRQ activity */
		if (pending_work & QM35_IRQ_WORK) {
			/* Handle the event in the ISR */
			qm35_spi_isr(qmspi);
			qm35_clear_irq(qmspi);
			continue;
		}
		/* Execute generic works */
		if (pending_work & QM35_GENERIC_WORK) {
			struct qm35_work *cmd = wk->work;
			int ret;
			trace_qm35_process_cmd(qmspi, cmd);
			ret = cmd->cmd(qmspi, cmd->in, cmd->out);
			qm35_dequeue(qmspi, QM35_GENERIC_WORK, cmd, ret);
			trace_qm35_process_cmd_return(qmspi, ret);
		}
		/* Wait for more work */
		if (!pending_work) {
			qm35_wait_pending_work(qmspi);
		}
	}
	dev_warn(qmspi->base.dev, "Worker thread finished\n");
	return 0;
}

/**
 * qm35_thread_run() - Launch QM35 SPI worker thread.
 * @qmspi: QM35 SPI instance.
 * @cpu: The cpu on which to bind the thread.
 *
 * Returns: Zero on success or a negative error.
 */
int qm35_thread_run(struct qm35_spi *qmspi, int cpu)
{
	struct qm35_worker *wk = &qmspi->worker;
	unsigned long flags;

	/* Create worker thread */
	wk->thread = kthread_create(qm35_spi_thread, qmspi, "qm35-%s",
				    dev_name(qmspi->base.dev));
	if (IS_ERR(wk->thread))
		return PTR_ERR(wk->thread);
	if (cpu >= 0)
		kthread_bind(wk->thread, (unsigned)cpu);

	/* Increase thread priority */
	qm35_set_sched_attr(wk->thread);

	/* Ensure spurious IRQ that may come during qm35_setup_irq() (because
	   IRQ pin is already HIGH) isn't handle by the WK thread. */
	spin_lock_irqsave(&wk->work_wq.lock, flags);
	if (wk->pending_work & QM35_IRQ_WORK) {
		wk->pending_work &= ~QM35_IRQ_WORK;
		enable_irq(qmspi->spi->irq);
	}
	spin_unlock_irqrestore(&wk->work_wq.lock, flags);

	/* Start worker thread */
	wake_up_process(wk->thread);
	/* Let worker thread initialise before continuing. */
	mdelay(1);
	return 0;
}

/**
 * qm35_thread_stop() - Stop QM35 SPI worker thread.
 * @qmspi: QM35 SPI instance.
 */
void qm35_thread_stop(struct qm35_spi *qmspi)
{
	kthread_stop(qmspi->worker.thread);
}
