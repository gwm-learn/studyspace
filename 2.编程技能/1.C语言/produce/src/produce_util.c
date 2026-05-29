#include "produce_util.h"

int get_produce_param(char* key /*in*/, char* tmp_file_name /*in*/, char* value /*out*/) {
    int ret = -1;

    char* p = NULL;
    char* pend = NULL;
    FILE* fs = NULL;
    char tmp_file_name_ex[64] = {0};
    char tmp_con_format[128] = {0};
    char cmd[128] = {0};
    char str[256] = {0};
    int try_cache = 0;    // 1 means read /var/pp_XXX file directly

    if (!tmp_file_name) {
        snprintf(tmp_file_name_ex, sizeof(tmp_file_name_ex) - 1, PRODUCE_TMP_FILE_FORMAT, key);
        tmp_file_name = &(tmp_file_name_ex[0]);
        if (access(tmp_file_name_ex, F_OK) == 0)
            try_cache = 1;
    }

DoAgain:
    if (try_cache != 1) {
        unlink(tmp_file_name);
        snprintf(cmd, sizeof(cmd) - 1, CMD_READ_PRODUCE_TOOL " %s &> %s", key, tmp_file_name);
        system(cmd); /*bl_uni_macaddr=021018aabbdd*/
    }

    fs = fopen(tmp_file_name, "r");

    if (fs != NULL) {
        if (fgets(str, sizeof(str) - 1, fs) > 0) {
            snprintf(tmp_con_format, sizeof(tmp_con_format) - 1, PRODUCE_KEY_VALUE_FORMAT, key);
            if ((p = strstr(str, tmp_con_format)) != NULL) {
                if ((pend = strchr(str, '\n')) != NULL || (pend = strchr(str, '\r')) != NULL) {
                    *pend = '\0';
                }

                if (value) {
                    strcpy(value, p + strlen(tmp_con_format));
                } else {
                    printf("~!%s!~\n", p + strlen(tmp_con_format));
                }
                ret = 0;
            }
        }

        fclose(fs);
    }

    if ((ret != 0) && (try_cache == 1)) {
        try_cache = 0;
        goto DoAgain;
    }

    return ret;
}

int set_produce_param(char* key, char* tmp_file_name, char* value) {
    int ret = -1;
    char tmp_file_name_ex[64] = {0};
    char cmd[128] = {0};

    if (!tmp_file_name) {
        snprintf(tmp_file_name_ex, sizeof(tmp_file_name_ex) - 1, PRODUCE_TMP_FILE_FORMAT, key);
        tmp_file_name = &(tmp_file_name_ex[0]);
    }

    if (MATCH_UNSET_STR(value)) {
        value[0] = 0;
    }

    unlink(tmp_file_name);
    snprintf(cmd, sizeof(cmd) - 1, CMD_WRITE_PRODUCE_TOOL " %s %s &> %s", key, value, tmp_file_name);
    system(cmd);    /*bl_uni_macaddr=021018aabbdd*/
    system("sync"); /*ensure all data write to flash*/

    ret = 0;

    return ret;
}

int write_buf_to_file(const char* fileName, char* buf, unsigned int iLen) {
    int ret = -1;

    FILE* fp = fopen(fileName, "w");
    if (fp) {
        ret = fwrite(buf, 1, iLen, fp);
        fflush(fp);
        fclose(fp);
    }

    return ret;
}

int read_file_to_buf(const char* fileName, char* buf, unsigned int iMaxLen) {
    int ret = -1;
    FILE* fp = fopen(fileName, "r");
    if (fp) {
        ret = fread(buf, 1, iMaxLen, fp);
        fclose(fp);
    }
    if (ret > 0 && (buf[ret - 1] == '\n' || buf[ret - 1] == '\r')) {
        buf[ret - 1] = 0;
    }
    return ret;
}

char* read_file_to_alloc_buf(const char* fullfilepath) {
    char* alloc_buf = NULL;

    struct stat statbuf;
    if (stat(fullfilepath, &statbuf) == 0) {
        if (statbuf.st_size > 0) {
            int allocSiz = statbuf.st_size + 4;
            alloc_buf = malloc(allocSiz);
            if (alloc_buf) {
                memset(alloc_buf, 0x0, allocSiz);
                read_file_to_buf(fullfilepath, alloc_buf, allocSiz - 1);
            }
        }
    }

    return alloc_buf;
}

int is_valid_mac(char* mac) {
    if (mac == NULL)
        return 0;

    int len = strlen(mac);
    if (len != 12)
        return 0;

    for (int i = 0; i < 12; i++) {
        if (!((mac[i] >= '0' && mac[i] <= '9') ||
              (mac[i] >= 'a' && mac[i] <= 'f') ||
              (mac[i] >= 'A' && mac[i] <= 'F')))
            return 0;
    }
    return 1;
}

void change_to_lowercase(char* mac) {
    for (int i = 0; i < (int)strlen(mac); i++) {
        if (mac[i] >= 'A' && mac[i] <= 'F')
            mac[i] = mac[i] - 'A' + 'a';
    }
}

void change_to_upcase(char* mac) {
    int i = 0;
    for (i = 0; i < strlen(mac); i++) {
        if (mac[i] == 'a') {
            mac[i] = 'A';
        } else if (mac[i] == 'b') {
            mac[i] = 'B';
        } else if (mac[i] == 'c') {
            mac[i] = 'C';
        } else if (mac[i] == 'd') {
            mac[i] = 'D';
        } else if (mac[i] == 'e') {
            mac[i] = 'E';
        } else if (mac[i] == 'f') {
            mac[i] = 'F';
        }
    }
}

void do_next_hex_char(char* o_ch) {
    char ch = *o_ch;
    if (ch >= '0' && ch < '9')
        ch = ch + 1;
    else if (ch == '9')
        ch = 'a';
    else if (ch >= 'a' && ch < 'f')
        ch = ch + 1;
    else if (ch == 'f')
        ch = '0';
    else if (ch >= 'A' && ch < 'F')
        ch = ch + 1;
    else if (ch == 'F')
        ch = '0';
    *o_ch = ch;
}

void conver_mac_addr_to_sp1(char* baseMacAddrStr, char* finaMacAddrStr) {
    int i, j;
    change_to_lowercase(baseMacAddrStr);
    for (i = 0, j = 0; i < 12; i++) {
        finaMacAddrStr[j] = baseMacAddrStr[i];
        j++;
        if (((j + 1) % 3) == 0 && j < 17) {
            finaMacAddrStr[j] = ':';
            j++;
        }
    }
}

int wps_pin_validate_checksum(unsigned long int PIN) {
    unsigned long int accum = 0;
    accum += 3 * ((PIN / 10000000) % 10);
    accum += 1 * ((PIN / 1000000) % 10);
    accum += 3 * ((PIN / 100000) % 10);
    accum += 1 * ((PIN / 10000) % 10);
    accum += 3 * ((PIN / 1000) % 10);
    accum += 1 * ((PIN / 100) % 10);
    accum += 3 * ((PIN / 10) % 10);
    accum += 1 * ((PIN / 1) % 10);
    return (0 == (accum % 10));
}

int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

char int_to_hex_char(int n) {
    if (n >= 0 && n <= 9) return '0' + n;
    if (n >= 10 && n <= 15) return 'a' + (n - 10);
    return '0';
}