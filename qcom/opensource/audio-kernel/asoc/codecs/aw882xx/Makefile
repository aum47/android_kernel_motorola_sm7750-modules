# In case of out of tree build, build as a module
# (when build inside kernel we will not enter this directory and this will have no effect)
ifeq ($(CONFIG_SND_SOC_AW882XX),)
CONFIG_SND_SOC_AW882XX := m
endif

ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS += -Werror

obj-$(CONFIG_SND_SOC_TFA98XX) := snd-soc-aw882xx.o

snd-soc-aw882xx-objs += aw882xx.o
snd-soc-aw882xx-objs += aw882xx_bin_parse.o
snd-soc-aw882xx-objs += aw882xx_calib.o
snd-soc-aw882xx-objs += aw882xx_device.o
snd-soc-aw882xx-objs += aw882xx_dsp.o
snd-soc-aw882xx-objs += aw882xx_init.o
snd-soc-aw882xx-objs += aw882xx_monitor.o
snd-soc-aw882xx-objs += aw882xx_spin.o

else

MAKEARCH := $(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

all:
	$(MAKEARCH) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKEARCH) -C $(KDIR) M=$(PWD) clean
	rm -f $(snd-soc-aw882xx-objs)

endif
