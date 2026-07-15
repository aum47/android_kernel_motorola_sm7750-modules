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
#ifndef __QM35_SPI_PM_H
#define __QM35_SPI_PM_H

#include <linux/pm_runtime.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
#ifdef CONFIG_PM
#define pm_ptr(_ptr) (_ptr)
#else
#define pm_ptr(_ptr) NULL
#endif
#endif

extern const struct dev_pm_ops __maybe_unused qm35_dev_pm_ops;

struct qm35_spi;
int qm35_spi_pm_setup(struct qm35_spi *qmspi);
int qm35_spi_pm_remove(struct qm35_spi *qmspi);

int qm35_spi_pm_start(struct qm35_spi *qmspi);
int qm35_spi_pm_stop(struct qm35_spi *qmspi);
int qm35_spi_pm_resume(struct qm35_spi *qmspi);
int qm35_spi_pm_idle(struct qm35_spi *qmspi);

#ifdef QM35_SPI_PM_TESTS

#include "mocks/ku_base.h"

static int ku_regulator_enable(struct regulator *regulator);
static int ku_regulator_disable(struct regulator *regulator);

#define regulator_enable ku_regulator_enable
#define regulator_disable ku_regulator_disable

static void ku_disable_irq(unsigned int irq);
static void ku_enable_irq(unsigned int irq);

#define disable_irq ku_disable_irq
#define enable_irq ku_enable_irq

static int ku_dev_pm_set_wake_irq(struct device *dev, int irq);
static void ku_dev_pm_clear_wake_irq(struct device *dev);

#define dev_pm_set_wake_irq ku_dev_pm_set_wake_irq
#define dev_pm_clear_wake_irq ku_dev_pm_clear_wake_irq

static int ku_disable_irq_wake(unsigned int irq);
static int ku_enable_irq_wake(unsigned int irq);

#define disable_irq_wake ku_disable_irq_wake
#define enable_irq_wake ku_enable_irq_wake

static int ku_gpiod_set_value_cansleep(void *gpio, int value);
#define gpiod_set_value_cansleep ku_gpiod_set_value_cansleep

#endif /* QM35_SPI_PM_TESTS */
#endif /* __QM35_SPI_PM_H */
