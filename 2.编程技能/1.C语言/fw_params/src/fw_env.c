/*
 * (C) Copyright 2000-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2008
 * Guennadi Liakhovetski, DENX Software Engineering, lg@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define __user /* nothing */
#include <mtd/mtd-user.h>

#include "crc32.h"
#include "fw_env.h"

struct envdev_s {
    const char* devname; /* Device name */
    ulong devoff;        /* Device offset */
    ulong env_size;      /* environment size */
    ulong erase_size;    /* device erase size */
    ulong env_sectors;   /* number of environment sectors */
};

static struct envdev_s envdevices[2] = {};
static int dev_current;

#define DEVNAME(i) envdevices[(i)].devname
#define DEVOFFSET(i) envdevices[(i)].devoff
#define ENVSIZE(i) envdevices[(i)].env_size
#define DEVESIZE(i) envdevices[(i)].erase_size
#define ENVSECTORS(i) envdevices[(i)].env_sectors

#define CUR_ENVSIZE ENVSIZE(dev_current)

#define ENV_SIZE getenvsize()

struct env_image_single {
    uint32_t crc; /* CRC32 over data bytes    */
    char data[];
};

struct env_image_redundant {
    uint32_t crc; /* CRC32 over data bytes    */
    char data[];
};

struct environment {
    void* image;
    uint32_t* crc;
    char* data;
};

static struct environment environment = {};
static int HaveRedundEnv = 0;
static int HaveOneCrcError = 0;
//default nandflash
static int currmtdtype = MTD_NANDFLASH;

static int flash_io(int mode);
static char* envmatch(char* s1, char* s2);
static int parse_config(void);

static inline ulong getenvsize(void) {
    ulong rc = CUR_ENVSIZE - sizeof(uint32_t);

    return rc;
}

static inline int isflashempty(void* data) {
    return (*(unsigned int*)environment.data == 0xFFFFFFFF);
}

int getmtdtype(int fd) {
    int rc;
    struct mtd_info_user mtdinfo;

    rc = ioctl(fd, MEMGETINFO, &mtdinfo);
    if (rc >= 0) {
        if (mtdinfo.type != MTD_NORFLASH && mtdinfo.type != MTD_NANDFLASH) {
            fprintf(stderr, "Unsupported flash type %u on %s\n", mtdinfo.type, DEVNAME(dev_current));
        } else {
#ifdef DEBUG
            fprintf(stderr, "curr flash type %u on %s\n", mtdinfo.type, DEVNAME(dev_current));
#endif
        }
        return mtdinfo.type;
    }

    return MTD_ABSENT;
}

/*
 * Print the current definition of one, or more, or all
 * environment variables
 */
int fw_printenv(int argc, char* argv[]) {
    char *env, *nxt;
    int i;
    int rc = 0;

    if (fw_env_open())
        return -1;

    if (isflashempty(environment.data))
        return 0;

    if (argc == 1) { /* Print all env variables  */
        for (env = environment.data; *env; env = nxt + 1) {
            for (nxt = env; *nxt; ++nxt) {
                if (nxt >= &environment.data[ENV_SIZE]) {
                    fprintf(stderr,
                            "## Error: "
                            "environment not terminated\n");
                    return -1;
                }
            }

            printf("%s\n", env);
        }
        rc = 0;
        goto RET_SUCCEED;
    }

    for (i = 1; i < argc; ++i) { /* print single env variables   */
        char* name = argv[i];
        char* val = NULL;

        for (env = environment.data; *env; env = nxt + 1) {
            for (nxt = env; *nxt; ++nxt) {
                if (nxt >= &environment.data[ENV_SIZE]) {
                    fprintf(stderr,
                            "## Error: "
                            "environment not terminated\n");
                    return -1;
                }
            }
            val = envmatch(name, env);
            if (val) {
                fputs(name, stdout);
                putc('=', stdout);
                puts(val);
                break;
            }
        }
        if (!val) {
            fprintf(stderr, "## Error: \"%s\" not defined\n", name);
            rc = -1;
        }
    }

RET_SUCCEED:
    //if one device have crc error, we suggust that write good device content to the error device
    if (HaveOneCrcError)
        fw_env_close();

    return rc;
}

int fw_env_close(void) {
    int ret;

    /*
	 * Update CRC
	 */
    *environment.crc = crc32(0, (uint8_t*)environment.data, ENV_SIZE);
#ifdef DEBUG
    fprintf(stderr, "Update CRC: 0x%x length:0x%lx\n", *environment.crc, ENV_SIZE);
#endif

    /* write environment back to flash */
    if (flash_io(O_RDWR)) {
        fprintf(stderr, "Error: can't write fw_env to flash\n");
        return -1;
    }

    return 0;
}

/*
 * Set/Clear a single variable in the environment.
 * This is called in sequence to update the environment
 * in RAM without updating the copy in flash after each set
 */
int fw_env_write(char* name, char* value) {
    int len;
    char *env, *nxt;
    char *rd, *wr;
    int found = 0;
    int deleting, creating, overwriting;
    char* end = &environment.data[ENV_SIZE];

    if (isflashempty(environment.data)) {
        environment.data[0] = '\0';
        environment.data[1] = '\0';
        goto append;
    }

    wr = environment.data;
    rd = environment.data;

    while (*rd) {
        nxt = rd;
        while (*nxt) {
            if (nxt >= end) {
                fprintf(stderr,
                        "## Error: "
                        "environment not terminated\n");
                errno = EINVAL;
                return -1;
            }
            ++nxt;
        }

        if (envmatch(name, rd)) {
            found = 1;
            rd = nxt + 1;
        } else {
            int entry_len = nxt - rd + 1;
            if (rd != wr)
                memmove(wr, rd, entry_len);
            wr += entry_len;
            rd += entry_len;
        }
    }

    *wr++ = '\0';
    if (wr < end)
        *wr = '\0';

    deleting = (found && !(value && strlen(value)));
    creating = (!found && (value && strlen(value)));
    overwriting = (found && (value && strlen(value)));

#ifdef DEBUG
    if (deleting)
        fprintf(stderr, "deleting action\n");
    else if (creating)
        fprintf(stderr, "creating action\n");
    else if (overwriting)
        fprintf(stderr, "overwriting action\n");
#endif

    if (deleting)
        return 0;
    if (!creating && !overwriting)
        return 0;

append:
    for (env = environment.data; *env || *(env + 1); ++env)
        ;
    if (env > environment.data)
        ++env;
    /*
	 * Overflow when:
	 * "name" + "=" + "val" +"\0\0"  > CUR_ENVSIZE - (env-environment)
	 */
    len = strlen(name) + 2;
    /* add '=' for first arg, ' ' for all others */
    len += strlen(value) + 1;

#ifdef DEBUG
    fprintf(stderr, "set name value(%s %s) address(%p %p) startoffset(%ld)\n", name, value, env, environment.data,
            (env - environment.data));
#endif

    if (len > (&environment.data[ENV_SIZE] - env)) {
        fprintf(stderr, "Error: environment overflow, \"%s\" deleted\n", name);
        return -1;
    }

    while ((*env = *name++) != '\0')
        env++;
    *env = '=';
    while ((*++env = *value++) != '\0')
        ;
    *++env = '\0';

    return 0;
}

/*
 * Deletes or sets environment variables. Returns -1 and sets errno error codes:
 * 0	  - OK
 * EINVAL - need at least 1 argument
 * EROFS  - certain variables ("ethaddr", "serial#") cannot be
 *	    modified or deleted
 *
 */
int fw_setenv(int argc, char* argv[]) {
    int i, rc;
    size_t len;
    char* name;
    char* value = NULL;

    if (argc < 2) {
        errno = EINVAL;
        return -1;
    }

    if (fw_env_open()) {
        fprintf(stderr, "Error: environment not initialized\n");
        return -1;
    }

    name = argv[1];

    len = 0;
    for (i = 2; i < argc; ++i) {
        char* val = argv[i];
        size_t val_len = strlen(val);

        if (value)
            value[len - 1] = ' ';
        value = realloc(value, len + val_len + 1);
        if (!value) {
            fprintf(stderr, "Cannot malloc %zu bytes: %s\n", len, strerror(errno));
            return -1;
        }

        memcpy(value + len, val, val_len);
        len += val_len;
        value[len++] = '\0';
    }

#ifdef DEBUG
    fprintf(stderr, "set name value(%s %s)\n", name, value);
#endif
    fw_env_write(name, value);

    free(value);

    return fw_env_close();
}

/*
 * Read data from flash at an offset into a provided buffer. On NAND it skips
 * bad blocks but makes sure it stays within ENVSECTORS (dev) starting from
 * the DEVOFFSET (dev) block. On NOR the loop is only run once.
 */
static int flash_read_buf(int dev, int fd, void* buf, size_t count, off_t offset) {
    size_t processed = 0;   /* progress counter */
    size_t readlen = count; /* current read length */
    int rc;

    /*
	 * offset - see common/env_nand.c::writeenv()
	 */
    lseek(fd, offset, SEEK_SET);

    /* This only runs once on NOR flash */
    while (processed < count) {
        rc = read(fd, buf + processed, readlen);
        if (rc < 0) {
            fprintf(stderr, "Read error on %s: %s\n", DEVNAME(dev), strerror(errno));
            return -1;
        }

#ifdef DEBUG
        fprintf(stderr, "Read 0x%x bytes at 0x%lx on %s\n", rc, processed + offset, DEVNAME(dev));
#endif

        processed += rc;
        readlen -= rc;
    }

    return processed;
}

/*
 * Write count bytes at offset, but stay within ENVSECTORS (dev) sectors of
 * DEVOFFSET (dev). Similar to the read case above, on NOR and dataflash we
 * erase and write the whole data at once.
 */
static int flash_write_buf(int dev, int fd, void* buf, size_t count, off_t offset) {
    struct erase_info_user erase;
    size_t processed = 0;       /* progress counter */
    size_t write_total = count; /* total size to actually write - excluding
				   bad blocks */
    int rc;

    //nand flash need this when writing
    erase.start = offset;
    erase.length = count;
    if (currmtdtype != MTD_ABSENT) {
        ioctl(fd, MEMUNLOCK, &erase);
        //����λ��1�����
        if (ioctl(fd, MEMERASE, &erase) != 0) {
            fprintf(stderr, "MTD erase error on %s: %s\n", DEVNAME(dev), strerror(errno));
            goto RET_FAILED;
        }
        //����λ��0���ŵ�
    }
    lseek(fd, offset, SEEK_SET);
    /* This only runs once on NOR flash and SPI-dataflash */
    while (processed < write_total) {
        rc = write(fd, buf + processed, write_total);
        if (rc < 0) {
            fprintf(stderr, "Write error on %s: %s\n", DEVNAME(dev), strerror(errno));
            goto RET_FAILED;
        }

#ifdef DEBUG
        fprintf(stderr, "Write 0x%x bytes at 0x%lx\n", rc, offset + processed);
#endif

        processed += rc;
        write_total -= rc;
    }

    if (currmtdtype != MTD_ABSENT) {
        //nand flash need this
        ioctl(fd, MEMLOCK, &erase);
    }

    return processed;

RET_FAILED:
    if (currmtdtype != MTD_ABSENT) {
        //nand flash need this
        ioctl(fd, MEMLOCK, &erase);
    }

    return -1;
}

static int flash_write(int fd_current, int fd_target, int dev_target) {
    int rc;

#ifdef DEBUG
    fprintf(stderr, "Writing new environment at 0x%lx on %s\n", DEVOFFSET(dev_target), DEVNAME(dev_target));
    fprintf(stderr, "image start:%p crc:%p data:%p\n", environment.image, environment.crc, environment.data);
#endif

#ifdef DEBUG
    {
        //dump 128 chars
        int i;
        char* env = environment.image;
        for (i = 0; i < 128; i++) {
            printf("%02x ", env[i]);
            if ((i + 1) % 16 == 0)
                printf("\n");
            if ((i + 1) % 8 == 0)
                printf("\t");
        }
    }
#endif

    rc = flash_write_buf(dev_target, fd_target, environment.image, CUR_ENVSIZE, DEVOFFSET(dev_target));

    if (rc < 0)
        return rc;

    return 0;
}

static int flash_read(int fd) {
    struct stat st;
    int rc;

    rc = fstat(fd, &st);
    if (rc < 0) {
        fprintf(stderr, "Cannot stat the file %s\n", DEVNAME(dev_current));
        return -1;
    }

    currmtdtype = getmtdtype(fd);

    rc = flash_read_buf(dev_current, fd, environment.image, CUR_ENVSIZE, DEVOFFSET(dev_current));
    if (rc != CUR_ENVSIZE)
        return -1;

    return 0;
}

static int flash_io(int mode) {
    int fd_current, fd_target, rc, dev_target;

    /* dev_current: fd_current, erase_current */
    fd_current = open(DEVNAME(dev_current), mode);
    if (fd_current < 0) {
        fprintf(stderr, "Can't open %s: %s\n", DEVNAME(dev_current), strerror(errno));
        return -1;
    }

    if (mode == O_RDWR) {
        /* write current partition */
        dev_target = dev_current;
        fd_target = fd_current;
        rc = flash_write(fd_current, fd_target, dev_target);

        /* switch to next partition for writing */
        if (HaveRedundEnv) {
            dev_target = !dev_current;
            fd_target = open(DEVNAME(dev_target), mode);
            if (fd_target >= 0) {
                rc = flash_write(fd_current, fd_target, dev_target);
                close(fd_target);
            }
        }
    } else {
        rc = flash_read(fd_current);
    }

exit:
    if (close(fd_current)) {
        fprintf(stderr, "I/O error on %s: %s\n", DEVNAME(dev_current), strerror(errno));
        return -1;
    }

    return rc;
}

/*
 * s1 is either a simple 'name', or a 'name=value' pair.
 * s2 is a 'name=value' pair.
 * If the names match, return the value of s2, else NULL.
 */

static char* envmatch(char* s1, char* s2) {
    if (s1 == NULL || s2 == NULL)
        return NULL;

    while (*s1 == *s2++)
        if (*s1++ == '=')
            return s2;
    if (*s1 == '\0' && *(s2 - 1) == '=')
        return s2;
    return NULL;
}

/*
 * Prevent confusion if running from erased flash memory
 */
static int fw_env_dedup(void);
int fw_env_open(void) {
    int crc0, crc0_ok;
    void* addr0;

    int crc1, crc1_ok;
    unsigned char flag1;
    void* addr1;

    int ret;

    struct env_image_single* single;
    struct env_image_redundant* redundant;

    if (parse_config()) /* should fill envdevices */
        return -1;

    addr0 = calloc(1, CUR_ENVSIZE);
    if (addr0 == NULL) {
        fprintf(stderr, "Not enough memory for environment (%ld bytes)\n", CUR_ENVSIZE);
        return -1;
    }

    /* read environment from FLASH to local buffer */

    dev_current = 0;
    single = addr0;
    environment.image = addr0;
    environment.crc = &single->crc;
    environment.data = single->data;
    if (flash_io(O_RDONLY))
        return -1;
    crc0 = crc32(0, (uint8_t*)environment.data, ENV_SIZE);
    crc0_ok = (crc0 == *environment.crc);
#ifdef DEBUG
    fprintf(stderr, "CRC: 0x%x 0x%x length:0x%lx\n", *environment.crc, crc0, ENV_SIZE);
#endif

    if (!crc0_ok) {
        fprintf(stderr, "Warning:  MTD Bad CRC on %s\n", DEVNAME(dev_current));
        if (!HaveRedundEnv) {
            if (!isflashempty(environment.data)) {
#ifdef DEBUG
                fprintf(stderr, "Warning: MTD Bad CRC\n");
#endif
            } else {
#ifdef DEBUG
                fprintf(stderr, "Warning: MTD Not Inited\n");
#endif
            }
        } else {
            dev_current = 1;
            redundant = addr0;
            environment.image = addr0;
            environment.crc = &redundant->crc;
            environment.data = redundant->data;

            if (flash_io(O_RDONLY))
                return -1;
            crc1 = crc32(0, (uint8_t*)redundant->data, ENV_SIZE);
            crc1_ok = (crc1 == redundant->crc);
#ifdef DEBUG
            fprintf(stderr, "CRC: 0x%x 0x%x length:0x%lx\n", *environment.crc, crc1, ENV_SIZE);
#endif

            if (!crc1_ok) {
                fprintf(stderr, "Warning:  MTD Bad CRC on %s\n", DEVNAME(dev_current));
                if (!isflashempty(environment.data)) {
#ifdef DEBUG
                    fprintf(stderr, "Warning: Both MTDs Bad CRC\n");
#endif
                } else {
#ifdef DEBUG
                    fprintf(stderr, "Warning: Both MTDs Not Inited\n");
#endif
                }
            } else {
                HaveOneCrcError = 1;
            }
        }
    }

    //we always select a device to write env finally!
#ifdef DEBUG
    fprintf(stderr, "Selected env in %s\n", DEVNAME(dev_current));
#endif

    /*
	 * Deduplicate: if the same key appears more than once, keep only
	 * the first occurrence.  Updates the buffer CRC so the cleaned
	 * data gets written back on fw_env_close().
	 */
    if (fw_env_dedup()) {
        fprintf(stderr, "Warning: Duplicate environment entries removed\n");
        *environment.crc = crc32(0, (uint8_t*)environment.data, ENV_SIZE);
        HaveOneCrcError = 1;
    }

    return 0;
}

static int fw_env_dedup(void) {
    char *rd, *wr, *nxt, *check;
    char *end = &environment.data[ENV_SIZE];
    int removed = 0;

    if (isflashempty(environment.data))
        return 0;

    wr = environment.data;
    rd = environment.data;

    while (*rd) {
        nxt = rd;
        while (*nxt) {
            if (nxt >= end)
                return removed;
            ++nxt;
        }

        int is_dup = 0;
        check = environment.data;
        while (check < wr) {
            if (envmatch(rd, check)) {
                is_dup = 1;
                break;
            }
            while (*check)
                ++check;
            ++check;
        }

        if (is_dup) {
            rd = nxt + 1;
            removed = 1;
        } else {
            int entry_len = nxt - rd + 1;
            if (rd != wr)
                memmove(wr, rd, entry_len);
            wr += entry_len;
            rd += entry_len;
        }
    }

    *wr++ = '\0';
    if (wr < end)
        *wr = '\0';

    return removed;
}

static int parse_config() {
    struct stat st;

    DEVNAME(0) = DEVICE1_NAME;
    DEVOFFSET(0) = DEVICE1_OFFSET;
    ENVSIZE(0) = ENV1_SIZE;
    DEVESIZE(0) = DEVICE1_ESIZE;
    ENVSECTORS(0) = DEVICE1_ENVSECTORS;

#ifdef HAVE_REDUND
    DEVNAME(1) = DEVICE2_NAME;
    DEVOFFSET(1) = DEVICE2_OFFSET;
    ENVSIZE(1) = ENV2_SIZE;
    DEVESIZE(1) = DEVICE2_ESIZE;
    ENVSECTORS(1) = DEVICE2_ENVSECTORS;
    HaveRedundEnv = 1;
#endif

    if (stat(DEVNAME(0), &st)) {
        fprintf(stderr, "Cannot access MTD device %s: %s\n", DEVNAME(0), strerror(errno));
        return -1;
    }

    if (HaveRedundEnv && stat(DEVNAME(1), &st)) {
        fprintf(stderr, "Cannot access MTD device %s: %s\n", DEVNAME(1), strerror(errno));
        return -1;
    }
    return 0;
}
