# Settings for compiling roadstr camera architecture

# Localized KCONFIG settings
CONFIG_MOT_SENSOR_PRE_POWERUP := y

# Flags to pass into C preprocessor
ccflags-y += -DCONFIG_MOT_SENSOR_PRE_POWERUP=1
