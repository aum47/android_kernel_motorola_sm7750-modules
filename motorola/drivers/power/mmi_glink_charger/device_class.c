/*
 * Copyright (C) 2024 Motorola Mobility LLC
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
 #include <linux/module.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/version.h>
#include "device_class.h"

static struct class *mmi_glink_class;
static struct srcu_notifier_head mmi_glink_notifier_chain;

int mmi_glink_register_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_register(&mmi_glink_notifier_chain, nb);
}

void mmi_glink_unregister_notifier(struct notifier_block *nb)
{
	srcu_notifier_chain_unregister(&mmi_glink_notifier_chain, nb);
}

int mmi_glink_notifier(DEV_TYPE type, void *data)
{
	return srcu_notifier_call_chain(
		&mmi_glink_notifier_chain, type, data);
}

static int match_device_by_name(struct device *dev,
	const void *data)
{
	const char *name = data;

	return strcmp(dev_name(dev), name) == 0;
}

struct glink_device *get_dev_by_name(const char *name)
{
	struct device *dev;

	if (!name)
		return (struct glink_device *)NULL;
	dev = class_find_device(mmi_glink_class, NULL, name,
				match_device_by_name);

	return dev ? to_glink_device(dev) : NULL;
}

int is_dev_exist(const char *name)
{
	if (get_dev_by_name(name) == NULL)
		return 0;
	return 1;
}

void glink_device_release(struct device *dev)
{
	struct glink_device *chg_dev = to_glink_device(dev);

	kfree(chg_dev);
}

#define MAX_DEV_NAME_LEN 16
struct glink_device *glink_device_register(const char *name,
		struct device *parent, DEV_TYPE type, void *devdata)
{
	struct glink_device *glink_dev;
	int rc;

	pr_info("mmi glink device register: name=%s\n",name);
	glink_dev = kzalloc(sizeof(struct glink_device),GFP_KERNEL);
	if (!glink_dev)
		return ERR_PTR(-ENOMEM);

	mutex_init(&glink_dev->dev_lock);
	glink_dev->dev.class = mmi_glink_class;
	glink_dev->dev.parent = parent;
	glink_dev->dev.release = glink_device_release;
	//strncpy(glink_dev->name, name, MAX_DEV_NAME_LEN);
	glink_dev->dev_type = type;
	dev_set_name(&glink_dev->dev, "%s", name);
	dev_set_drvdata(&glink_dev->dev, devdata);

	rc = device_register(&glink_dev->dev);
	if (rc) {
		kfree(glink_dev);
		return ERR_PTR(rc);
	}

	pr_info("mmi glink device register: name=%s, successfully\n",name);
	return glink_dev;
}

void glink_device_unregister(struct glink_device *glink_dev)
{
	if (!glink_dev)
		return;
	device_unregister(&glink_dev->dev);
}

void mmi_glink_class_exit(void)
{
	class_destroy(mmi_glink_class);
}

int mmi_glink_class_init(void)
{
	static struct lock_class_key key;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
	mmi_glink_class = class_create("mmi_glink_charger");
#else
	mmi_glink_class = class_create(THIS_MODULE, "mmi_glink_charger");
#endif
	if (IS_ERR(mmi_glink_class)) {
		pr_err("Unable to create mmi glink charger class; error = %ld\n",
			PTR_ERR(mmi_glink_class));
		return PTR_ERR(mmi_glink_class);
	}

	srcu_init_notifier_head(&mmi_glink_notifier_chain);
	/* Rename srcu's lock to avoid LockProve warning */
	lockdep_init_map(&(&mmi_glink_notifier_chain.srcu)->dep_map, "mmi_glink_charger", &key, 0);

	pr_info("success to create mmi glink charger class \n");
	return 0;
}

