// SPDX-License-Identifier: GPL-2.0+
/*
 * DIO8015, Multi-Output Regulators
 * Copyright (C) 2019  Motorola Mobility LLC,
 *
 * Author: Huqian, Motorola Mobility LLC,
 */

#ifndef __DIO8015_REGISTERS_H__
#define __DIO8015_REGISTERS_H__

/* Registers */
#define DIO8015_REG_NUM (DIO8015_SEQ_STATUS-DIO8015_CHIP_REV+1)

#define DIO8015_CHIP_REV 0x00
#define DIO8015_REG_CHIPID2 0x19
#define DIO8015_CURRENT_LIMITSEL 0x01
#define DIO8015_DISCHARGE_RESISTORS 0x02
#define DIO8015_LDO1_VOUT 0x03
#define DIO8015_LDO2_VOUT 0x04
#define DIO8015_LDO3_VOUT 0x05
#define DIO8015_LDO4_VOUT 0x06
#define DIO8015_LDO1_LDO2_SEQ 0x0a
#define DIO8015_LDO3_LDO4_SEQ 0x0b
#define DIO8015_LDO_EN 0x0e
#define DIO8015_SEQ_STATUS 0x0f


/* DIO8015_LDO1_VSEL ~ DIO8015_LDO4_VSEL =
 * 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
 */
#define  DIO8015_LDO1_VSEL                      DIO8015_LDO1_VOUT
#define  DIO8015_LDO2_VSEL                      DIO8015_LDO2_VOUT
#define  DIO8015_LDO3_VSEL                      DIO8015_LDO3_VOUT
#define  DIO8015_LDO4_VSEL                      DIO8015_LDO4_VOUT


#define  DIO8015_VSEL_SHIFT                     0
#define  DIO8015_VSEL_MASK                      (0xff << 0)

#endif /* __DIO8015_REGISTERS_H__ */
