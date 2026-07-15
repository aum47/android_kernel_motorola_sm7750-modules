/*
 * Copyright (C) 2018 Motorola Mobility LLC.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

//#define DEBUG

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <cam_cci_dev.h>
#include <cam_sensor_i2c.h>
#include <media/v4l2-ioctl.h>
#include <media/cci_intf.h>

extern struct camera_io_master *common_io_master_info;

static int32_t cci_intf_xfer(
		struct msm_cci_intf_xfer *xfer,
		unsigned int cmd)
{
	int32_t rc, rc2;
	uint16_t addr;
	uint16_t i2c_addr;
	struct cam_sensor_i2c_reg_setting write_setting;
	unsigned char *buf = NULL;

	struct cam_sensor_cci_client cci_info = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		.cci_subdev     = cam_cci_get_subdev(xfer->cci_device),
#else
		.cci_subdev     = cam_cci_get_subdev(),
#endif
		.cci_i2c_master = xfer->cci_bus,
		.sid            = xfer->slave_addr,
	};
	struct cam_cci_ctrl cci_ctrl = {
		.cci_info = &cci_info,
	};
	int i;
	struct cam_sensor_i2c_reg_array *reg_conf_tbl;

	printk("%s cmd:%d bus:%d devaddr:%02x regw:%d rega:%04x count:%d master_type:%d\n",
			__func__, cmd, xfer->cci_bus, xfer->slave_addr,
			xfer->reg.width, xfer->reg.addr, xfer->data.count, common_io_master_info->master_type);

	if (xfer->slave_addr > 0x7F ||
			xfer->reg.width < 1 || xfer->reg.width > 2 ||
			xfer->reg.addr > ((1<<(8*xfer->reg.width))-1) ||
			xfer->data.count < 1 ||
			xfer->data.count > MSM_CCI_INTF_MAX_XFER)
	{
		printk("%s: cci/i2c bus mismatch!!!\n", __func__);
		return -EINVAL;
	}

	if(common_io_master_info->master_type == CCI_MASTER)
	{
		/* init */
		cci_ctrl.cmd = MSM_CCI_INIT;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		rc = v4l2_subdev_call(cam_cci_get_subdev(xfer->cci_device),
#else
		rc = v4l2_subdev_call(cam_cci_get_subdev(),
#endif
				core, ioctl, VIDIOC_MSM_CCI_CFG, &cci_ctrl);
		if (rc < 0) {
			printk("%s: cci init fail (%d)\n", __func__, rc);
			return rc;
		}
	}

	switch (cmd) {
	case MSM_CCI_INTF_READ:
		/* read */
		cci_ctrl.cmd = MSM_CCI_I2C_READ;
		cci_ctrl.cfg.cci_i2c_read_cfg.addr = xfer->reg.addr;
		cci_ctrl.cfg.cci_i2c_read_cfg.addr_type =
			(xfer->reg.width == 1 ?
			 CAMERA_SENSOR_I2C_TYPE_BYTE :
			 CAMERA_SENSOR_I2C_TYPE_WORD);
		cci_ctrl.cfg.cci_i2c_read_cfg.data = xfer->data.buf;
		cci_ctrl.cfg.cci_i2c_read_cfg.num_byte = xfer->data.count;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		rc = v4l2_subdev_call(cam_cci_get_subdev(xfer->cci_device),
#else
		rc = v4l2_subdev_call(cam_cci_get_subdev(),
#endif
			core, ioctl, VIDIOC_MSM_CCI_CFG, &cci_ctrl);
		if (rc < 0) {
			printk("%s: cci read fail (%d)\n", __func__, rc);
			goto release;
		}
		rc = cci_ctrl.status;
		for(int i = 0; i< xfer->data.count; i++)
			printk("%s: i2c read success (0x%x)\n", __func__, xfer->data.buf[i]);
		break;
	case MSM_CCI_INTF_WRITE:
		/* write */
		reg_conf_tbl = kzalloc(xfer->data.count *
				sizeof(struct cam_sensor_i2c_reg_array),
				GFP_KERNEL);
		if (!reg_conf_tbl) {
			rc = -ENOMEM;
			goto release;
		}
		addr = xfer->reg.addr;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		for (i = 0; i < xfer->data.count; i += xfer->data.width) {
			reg_conf_tbl[i].reg_addr = addr++;
			if(xfer->data.width == 4) {
				reg_conf_tbl[i].reg_data =
					((uint32_t)(xfer->data.buf[i]) << 24) |
					((uint32_t)(xfer->data.buf[i+1]) << 16) |
					((uint32_t)(xfer->data.buf[i+2]) << 8) |
					((uint32_t)(xfer->data.buf[i+3]));
				printk("%s: cci writing %x", __func__, reg_conf_tbl[i].reg_data);
			} else if(xfer->data.width == 2) {
				reg_conf_tbl[i].reg_data =
					((uint32_t)(xfer->data.buf[i]) << 8) |
					((uint32_t)(xfer->data.buf[i+1]));
			} else {
				reg_conf_tbl[i].reg_data = xfer->data.buf[i];
			}
			reg_conf_tbl[i].delay = 0;
			printk("%s: cci writing %x", __func__, reg_conf_tbl[i].reg_data);
		}
#else
		for (i = 0; i < xfer->data.count; i += 1) {
			reg_conf_tbl[i].reg_addr = addr++;
			reg_conf_tbl[i].reg_data = xfer->data.buf[i];
			reg_conf_tbl[i].delay = 0;
		}
#endif
		cci_ctrl.cmd = MSM_CCI_I2C_WRITE;
		cci_ctrl.cfg.cci_i2c_write_cfg.reg_setting = reg_conf_tbl;
		cci_ctrl.cfg.cci_i2c_write_cfg.addr_type =
			(xfer->reg.width == 1 ?
				CAMERA_SENSOR_I2C_TYPE_BYTE :
				CAMERA_SENSOR_I2C_TYPE_WORD);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		cci_ctrl.cfg.cci_i2c_write_cfg.data_type = xfer->data.width;
#else
		cci_ctrl.cfg.cci_i2c_write_cfg.data_type =
			CAMERA_SENSOR_I2C_TYPE_BYTE;
#endif
		cci_ctrl.cfg.cci_i2c_write_cfg.size = xfer->data.count;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		rc = v4l2_subdev_call(cam_cci_get_subdev(xfer->cci_device),
#else
		rc = v4l2_subdev_call(cam_cci_get_subdev(),
#endif
				core, ioctl, VIDIOC_MSM_CCI_CFG, &cci_ctrl);
		kfree(reg_conf_tbl);
		if (rc < 0) {
			printk("%s: cci write fail (%d)\n", __func__, rc);
			goto release;
		}
		rc = cci_ctrl.status;
		break;
	case MSM_I2C_INTF_READ:
		if ((common_io_master_info->qup_client != NULL) &&
			(common_io_master_info->qup_client->i2c_client != NULL) &&
			(common_io_master_info->qup_client->i2c_client->adapter != NULL) &&
			(common_io_master_info->qup_client->pm_ctrl_client_enable)) {

			buf = kzalloc((xfer->reg.width == 1 ? CAMERA_SENSOR_I2C_TYPE_BYTE : CAMERA_SENSOR_I2C_TYPE_WORD) + xfer->data.count, GFP_KERNEL);

			if ((xfer->reg.width == 1 ? CAMERA_SENSOR_I2C_TYPE_BYTE : CAMERA_SENSOR_I2C_TYPE_WORD) == CAMERA_SENSOR_I2C_TYPE_BYTE) {
				buf[0] = xfer->reg.addr;
			} else if ((xfer->reg.width == 1 ? CAMERA_SENSOR_I2C_TYPE_BYTE : CAMERA_SENSOR_I2C_TYPE_WORD) == CAMERA_SENSOR_I2C_TYPE_WORD) {
				buf[0] = xfer->reg.addr >> 8;
				buf[1] = xfer->reg.addr;
			} else if ((xfer->reg.width == 1 ? CAMERA_SENSOR_I2C_TYPE_BYTE : CAMERA_SENSOR_I2C_TYPE_WORD) == CAMERA_SENSOR_I2C_TYPE_3B) {
				buf[0] = xfer->reg.addr >> 16;
				buf[1] = xfer->reg.addr >> 8;
				buf[2] = xfer->reg.addr;
			} else {
				buf[0] = xfer->reg.addr >> 24;
				buf[1] = xfer->reg.addr >> 16;
				buf[2] = xfer->reg.addr >> 8;
				buf[3] = xfer->reg.addr;
			}

			struct i2c_msg msgs[] = {
				{
					.addr  = xfer->slave_addr,
					.flags = 0,
					.len   = xfer->reg.width == 1 ? CAMERA_SENSOR_I2C_TYPE_BYTE : CAMERA_SENSOR_I2C_TYPE_WORD,
					.buf   = buf,
				},
				{
					.addr  = xfer->slave_addr,
					.flags = I2C_M_RD,
					.len   = xfer->data.count,
					.buf   = xfer->data.buf,
				},
			};

			rc = i2c_transfer(common_io_master_info->qup_client->i2c_client->adapter, msgs, 2);

			if(rc < 0)
			{
				printk("%s: i2c read fail (%d)\n", __func__, rc);
				kfree(buf);
				goto release;
			}

			for(int i = 0; i< xfer->data.count; i++)
				printk("%s: i2c read success (0x%x)\n", __func__, msgs[1].buf[i]);
			kfree(buf);
		}
		break;
	case MSM_I2C_INTF_WRITE:
		if ((common_io_master_info->qup_client != NULL) &&
			(common_io_master_info->qup_client->i2c_client != NULL) &&
			(common_io_master_info->qup_client->i2c_client->adapter != NULL) &&
			(common_io_master_info->qup_client->pm_ctrl_client_enable)) {
			reg_conf_tbl = kzalloc(xfer->data.count *
					sizeof(struct cam_sensor_i2c_reg_array),
					GFP_KERNEL);
			if (!reg_conf_tbl) {
				rc = -ENOMEM;
				goto release;
			}
			addr = xfer->reg.addr;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
			for (i = 0; i < xfer->data.count; i += xfer->data.width) {
				reg_conf_tbl[i].reg_addr = addr++;
				if(xfer->data.width == 4) {
					reg_conf_tbl[i].reg_data =
						((uint32_t)(xfer->data.buf[i]) << 24) |
						((uint32_t)(xfer->data.buf[i+1]) << 16) |
						((uint32_t)(xfer->data.buf[i+2]) << 8) |
						((uint32_t)(xfer->data.buf[i+3]));
					printk("%s: cci writing %x", __func__, reg_conf_tbl[i].reg_data);
				} else if(xfer->data.width == 2) {
					reg_conf_tbl[i].reg_data =
						((uint32_t)(xfer->data.buf[i]) << 8) |
						((uint32_t)(xfer->data.buf[i+1]));
				} else {
					reg_conf_tbl[i].reg_data = xfer->data.buf[i];
				}
				reg_conf_tbl[i].delay = 0;
				printk("%s: cci writing %x", __func__, reg_conf_tbl[i].reg_data);
			}
#else
			for (i = 0; i < xfer->data.count; i += 1) {
				reg_conf_tbl[i].reg_addr = addr++;
				reg_conf_tbl[i].reg_data = xfer->data.buf[i];
				reg_conf_tbl[i].delay = 0;
			}
#endif
			write_setting.reg_setting = reg_conf_tbl;
			write_setting.addr_type =
				(xfer->reg.width == 1 ?
					CAMERA_SENSOR_I2C_TYPE_BYTE :
					CAMERA_SENSOR_I2C_TYPE_WORD);
			write_setting.data_type = xfer->data.width;
			write_setting.size = xfer->data.count;

			i2c_addr = common_io_master_info->qup_client->i2c_client->addr;
			common_io_master_info->qup_client->i2c_client->addr = xfer->slave_addr << 1;
			rc = cam_qup_i2c_write_table(common_io_master_info, &write_setting);
			common_io_master_info->qup_client->i2c_client->addr = i2c_addr;
			kfree(reg_conf_tbl);
			if (rc < 0)
			{
				printk("%s: i2c write fail (%d)\n", __func__, rc);
				goto release;
			}
		}
		break;
	default:
		printk("%s: Unknown command (%d)\n", __func__, cmd);
		rc = -EINVAL;
		break;
	}

release:
	if(common_io_master_info->master_type == CCI_MASTER)
	{
		/* release */
		cci_ctrl.cmd = MSM_CCI_RELEASE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
		rc2 = v4l2_subdev_call(cam_cci_get_subdev(xfer->cci_device),
#else
		rc2 = v4l2_subdev_call(cam_cci_get_subdev(),
#endif
				core, ioctl, VIDIOC_MSM_CCI_CFG, &cci_ctrl);
		if (rc2 < 0) {
			printk("%s: cci release fail (%d)\n", __func__, rc2);
			return rc2;
		}
	}
	return rc;
}

static long cci_intf_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	struct msm_cci_intf_xfer xfer;
	int rc;

	printk("%s cmd=%x arg=%lx\n", __func__, cmd, arg);

	switch (cmd) {
	case MSM_CCI_INTF_READ:
	case MSM_CCI_INTF_WRITE:
	case MSM_I2C_INTF_READ:
	case MSM_I2C_INTF_WRITE:
		if (copy_from_user(&xfer, (void __user *)arg, sizeof(xfer)))
			return -EFAULT;
		rc = cci_intf_xfer(&xfer, cmd);
		if (copy_to_user((void __user *)arg, &xfer, sizeof(xfer)))
			return -EFAULT;
		return rc;
	default:
		return -ENOIOCTLCMD;
	}
}

#ifdef CONFIG_COMPAT
static long cci_intf_ioctl_compat(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	printk("%s cmd=%x\n", __func__, cmd);

	switch (cmd) {
	case MSM_CCI_INTF_READ32:
		cmd = MSM_CCI_INTF_READ;
		break;
	case MSM_CCI_INTF_WRITE32:
		cmd = MSM_CCI_INTF_WRITE;
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return cci_intf_ioctl(file, cmd, arg);
}
#endif

static const struct file_operations cci_intf_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cci_intf_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cci_intf_ioctl_compat,
#endif
};

static struct miscdevice cci_intf_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cci_intf",
	.fops = &cci_intf_fops,
};

int cam_cci_debug_sub_module_init(void)
{
	int rc;

	printk("%s\n", __func__);

	rc = misc_register(&cci_intf_misc);
	if (unlikely(rc)) {
		printk("failed to register misc device %s\n", cci_intf_misc.name);
		return rc;
	}

	return 0;
}

void cam_cci_debug_sub_module_exit(void)
{
	printk("%s\n", __func__);

	misc_deregister(&cci_intf_misc);
}
