/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * i8042.h -- defines the interface between EC core and the EC lib, which
 * talks to the LPC driver (on the EC side) peering to the keyboard driver
 * (on the host side).
 *
 * The EC lib implements this interface.
 */

#ifndef __INTERFACE_I8042_H
#define __INTERFACE_I8042_H

#include "common.h"


/* Keyboard command definition. Modified from Linux kernel atkbd.c file. */
/* port 0x60 */
#define I8042_CMD_MOUSE_1_1     0xe6
#define I8042_CMD_MOUSE_2_1     0xe7
#define I8042_CMD_MOUSE_RES     0xe8  /* dup to I8042_CMD_OK_GETID */
#define I8042_CMD_GET_MOUSE     0xe9
#define I8042_CMD_SETLEDS       0xed
#define I8042_CMD_DIAG_ECHO     0xee
#define I8042_CMD_GSCANSET      0xf0
#define I8042_CMD_SSCANSET      0xf0
#define I8042_CMD_GETID         0xf2
#define I8042_CMD_SETREP        0xf3
#define I8042_CMD_ENABLE        0xf4
#define I8042_CMD_RESET_DIS     0xf5
#define I8042_CMD_RESET_DEF     0xf6
#define I8042_CMD_ALL_TYPEM     0xf7
#define I8042_CMD_SETALL_MB     0xf8
#define I8042_CMD_SETALL_MBR    0xfa
#define I8042_CMD_SET_A_KEY_T   0xfb
#define I8042_CMD_SET_A_KEY_MR  0xfc
#define I8042_CMD_SET_A_KEY_M   0xfd
#define I8042_CMD_RESET_BAT     0xff
#define I8042_CMD_RESEND        0xfe
#define I8042_CMD_EX_ENABLE     0xea
#define I8042_CMD_EX_SETLEDS    0xeb
#define I8042_CMD_OK_GETID      0xe8

/* port 0x64 */
#define I8042_READ_CMD_BYTE     0x20
#define I8042_READ_CTL_RAM      0x21
#define I8042_READ_CTL_RAM_END  0x3f
#define I8042_WRITE_CMD_BYTE    0x60  /* expect a byte on port 0x60 */
#define I8042_WRITE_CTL_RAM     0x61
#define I8042_WRITE_CTL_RAM_END 0x7f
#define I8042_ROUTE_AUX0        0x90
#define I8042_ROUTE_AUX1        0x91
#define I8042_ROUTE_AUX2        0x92
#define I8042_ROUTE_AUX3        0x93
#define I8042_ENA_PASSWORD      0xa6
#define I8042_DIS_MOUSE         0xa7
#define I8042_ENA_MOUSE         0xa8
#define I8042_TEST_MOUSE        0xa9
#define I8042_RESET_SELF_TEST   0xaa
#define I8042_TEST_KB_PORT      0xab
#define I8042_DIS_KB            0xad
#define I8042_ENA_KB            0xae
#define I8042_ECHO_MOUSE        0xd3  /* expect a byte on port 0x60 */
#define I8042_SEND_TO_MOUSE     0xd4  /* expect a byte on port 0x60 */
#define I8042_PULSE_START       0xf0
#define I8042_PULSE_END         0xfd
#define I8042_SYSTEM_RESET      0xfe

/* port 0x60 return value */
#define I8042_RET_BAT           0xaa
#define I8042_RET_EMUL0         0xe0
#define I8042_RET_EMUL1         0xe1
#define I8042_RET_ECHO          0xee
#define I8042_RET_RELEASE       0xf0
#define I8042_RET_HANJA         0xf1
#define I8042_RET_HANGEUL       0xf2
#define I8042_RET_ACK           0xfa
#define I8042_RET_TEST_FAIL     0xfc
#define I8042_RET_INTERNAL_FAIL 0xfd
#define I8042_RET_NAK           0xfe
#define I8042_RET_ERR           0xff

/* port 64 - command byte bits */
#define I8042_XLATE             (1 << 6)
#define I8042_AUX_DIS           (1 << 5)
#define I8042_KBD_DIS           (1 << 4)
#define I8042_SYS_FLAG          (1 << 2)
#define I8042_ENIRQ12           (1 << 1)
#define I8042_ENIRQ1            (1 << 0)


void i8042_flush_buffer(void);


/* common/i8042.c implements this function, which is called by lpc.c
 * when an i8042 command/data from host side appears.
 *
 * Actually the a pair of data/command write would trigger 2 LPc interrupts.
 * So, we will queue the data byte first, then call keyboard routine after
 * receiving the command byte.
 */
void i8042_receives_data(int data);
void i8042_receives_command(int cmd);


/* Called by common/keyboard.c when the host doesn't want to receive
 * keyboard IRQ.
 */
void i8042_enable_keyboard_irq(void);
void i8042_disable_keyboard_irq(void);


/* Send the scan code to the host. The EC lib will push the scan code bytes
 * to host via port 0x60 and assert the IBF flag to trigger an interrupt.
 * The EC lib must queue them if the host cannot read the previous byte away
 * in time.
 *
 * Return:
 *   EC_ERROR_BUFFER_FULL -- the queue to host is full. Try again?
 */
enum ec_error_list i8042_send_to_host(int len, const uint8_t *to_host);


#endif  /* __INTERFACE_I8042_H */
