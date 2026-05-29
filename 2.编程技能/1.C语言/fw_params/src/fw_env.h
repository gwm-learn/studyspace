/*
 * (C) Copyright 2002-2008
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#define DEBUG

#define CMD_PRINTENV "fw_printparams"
#define CMD_SETENV "fw_setparams"

#define HAVE_REDUND /* For systems with 2 env sectors */

#define DEVICE1_NAME "/dev/mtd3"
#define DEVICE2_NAME "/dev/mtd3"

#define DEVICE1_OFFSET 0x00000
#define ENV1_SIZE 0x40000
#define DEVICE1_ESIZE 0x40000
#define DEVICE1_ENVSECTORS 2

#define DEVICE2_OFFSET 0x0000
#define ENV2_SIZE 0x40000
#define DEVICE2_ESIZE 0x40000
#define DEVICE2_ENVSECTORS 2

extern int fw_printenv(int argc, char* argv[]);
extern int fw_setenv(int argc, char* argv[]);
extern int fw_env_open(void);
extern int fw_env_write(char* name, char* value);
extern int fw_env_close(void);
