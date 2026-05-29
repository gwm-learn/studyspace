#include "produce_mac.h"
#include "produce_util.h"

static int gen_all_plan_macs(char* base_mac_addr_str) {
    FILE* fp = NULL;
    char fina_mac_addr_str[24] = {0};

    if (access(PLAN_MACs_FILE, F_OK) == 0) {
        struct stat statbuf;
        if (stat(PLAN_MACs_FILE, &statbuf) == 0) {
            if (statbuf.st_size > MIN_SIZE_OF_MACs_FILE)
                return 0;
        }
    }

    conver_mac_addr_to_sp1(base_mac_addr_str, fina_mac_addr_str);

    fp = fopen(PLAN_MACs_FILE, "w+");
    if (fp) {
        fprintf(fp, "LANMAC\t%s\n", fina_mac_addr_str);

        do_next_hex_char(&fina_mac_addr_str[16]);
        fprintf(fp, "WANMAC01\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[16]);
        fprintf(fp, "WANMAC02\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[16]);
        fprintf(fp, "WANMAC03\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[16]);
        fprintf(fp, "WANMAC04\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[16]);
        fprintf(fp, "WANMAC05\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[16]);
        fprintf(fp, "WANMAC06\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[16]);
        fprintf(fp, "WANMAC07\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[16]);
        fprintf(fp, "WANMAC08\t%s\n", fina_mac_addr_str);
        fina_mac_addr_str[16] = base_mac_addr_str[11];

        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC01\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC02\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC03\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC04\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC05\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC06\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC07\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC08\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC09\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC10\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC11\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC12\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC13\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC14\t%s\n", fina_mac_addr_str);
        do_next_hex_char(&fina_mac_addr_str[9]);
        fprintf(fp, "WLANMAC15\t%s\n", fina_mac_addr_str);

        fclose(fp);
    }

    return 0;
}

static int get_mac_by_type(const char* mac_type, char* mac_addr) {
    int iRet = -1;
    char base_mac_addr_str[24] = {0};

    if (get_produce_param(PRODUCE_MACADDR_KEY, NULL, base_mac_addr_str) == 0) {

        gen_all_plan_macs(base_mac_addr_str);

        if (strcmp(mac_type, "gen")) {
            FILE* fp = fopen(PLAN_MACs_FILE, "r");
            if (fp) {
                char buf[64] = {0};

                //char *fgets(char *s, int size, FILE *stream);
                while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
                    if (strstr(buf, mac_type)) {
                        char mac_format[64] = {0};
                        char mac_val[64] = {0};
                        snprintf(mac_format, sizeof(mac_format) - 1, "%s	%%s", mac_type);
                        if (sscanf(buf, mac_format, mac_val) == 1) {
                            //output finnal mac_addr
                            if (mac_addr) {
                                strcpy(mac_addr, mac_val);
                            } else {
                                printf("%s", mac_val);
                                fflush(stdout);
                            }
                            iRet = 0;
                        }
                    }
                }

                fclose(fp);
            }
        } else {
            //just gen macs only
            iRet = 0;
        }
    }

    return iRet;
}

void produce_macrw(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (is_valid_mac(param) == 0 && !MATCH_UNSET_STR(param)) {
            printf("!fail!\n");
            return;
        }

        change_to_lowercase(param);
        if (set_produce_param(PRODUCE_MACADDR_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_MACADDR_KEY, NULL, NULL);
    }
}

void produce_mactype(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        get_mac_by_type(param, NULL);
    } else {
        printf("LANMAC:		use for br\n");
        printf("WANMACxx:	use for first WAN interfaces, xx arange is [01 ~ 08]\n");
        printf("WLANMACxx:	use for first WiFi interfaces, xx arange is [01 ~ 15]\n");
    }
}

void produce_show_alloc_wan_mac(int param_flag, char* param) {
    char* alloced_mac = read_file_to_alloc_buf(USES_MACs_FILE);

    if (alloced_mac) {
        //STD output
        printf("%s\n", alloced_mac);
    }

    if (alloced_mac) {
        free(alloced_mac);
    }
}

void produce_add_alloc_wan_mac(int param_flag, char* param) {
    char* alloced_mac = read_file_to_alloc_buf(USES_MACs_FILE);
    int found = (alloced_mac && strcasestr(alloced_mac, param));

    if (!found) {
        FILE* fpw = fopen(USES_MACs_FILE, "a+");
        if (fpw) {
            fputs(param, fpw);
            fputs("\n", fpw);

            fclose(fpw);
        }
    }

    if (alloced_mac) {
        free(alloced_mac);
    }
}

void produce_alloc_wan_mac(int param_flag, char* param) {
    FILE* fpf = fopen(PLAN_MACs_FILE, "r");
    char* alloced_mac = read_file_to_alloc_buf(USES_MACs_FILE);

    //someone broken this file, we should rebuild it directly
    if (access(PLAN_MACs_FILE, F_OK) != 0) {
        get_mac_by_type("gen", NULL);
    }

    if (fpf) {
        char buf[128] = {0};
        char val[32] = {0};
        int found = 0;

        while (fgets(buf, sizeof(buf) - 1, fpf) != NULL) {
            if (strstr(buf, "WANMAC")) {
                if (sscanf(buf, "%*s %s", val) == 1) {
                    if (!alloced_mac || (alloced_mac && !strcasestr(alloced_mac, val))) {
                        found = 1;
                        break;
                    }
                }
            }
        }
        fclose(fpf);

        if (found && val[0]) {
            //record this
            char tmp_file_name[256] = {};
            sprintf(tmp_file_name, "%s_XXXXXX", USES_MACs_FILE);
            mktemp(tmp_file_name);
            FILE* fpw = fopen(tmp_file_name, "w");
            if (fpw) {
                if (alloced_mac && alloced_mac[0]) {
                    int iLen = strlen(alloced_mac);
                    fputs(alloced_mac, fpw);
                    if (alloced_mac[iLen - 1] != '\n') {
                        fputs("\n", fpw);
                    }
                }
                fputs(val, fpw);
                fputs("\n", fpw);

                fclose(fpw);

                if (rename(tmp_file_name, USES_MACs_FILE) < 0) {
                    //try again
                    //printf("bad news, rename failed from %s to %s\n", tmp_file_name, USES_MACs_FILE);
                    usleep(1 * 1000);
                    rename(tmp_file_name, USES_MACs_FILE);
                }
            }

            //STD output
            printf("%s\n", val);
        }
    }

    if (alloced_mac) {
        free(alloced_mac);
    }
}

void produce_free_wan_mac(int param_flag, char* param) {
    char* alloced_mac = read_file_to_alloc_buf(USES_MACs_FILE);
    int found = (alloced_mac && strcasestr(alloced_mac, param));

    if (found) {
        char* pPos = strcasestr(alloced_mac, param);
        if (pPos) {
            pPos[0] = 0;
            pPos = strstr(pPos + 1, "\n");
            {
                if (pPos) {
                    pPos += 1;
                }
            }

            //record this
            {
                char tmp_file_name[256] = {};
                sprintf(tmp_file_name, "%s_XXXXXX", USES_MACs_FILE);
                mktemp(tmp_file_name);
                FILE* fpw = fopen(tmp_file_name, "w");
                if (fpw) {
                    if (alloced_mac && alloced_mac[0]) {
                        int iLen = strlen(alloced_mac);
                        fputs(alloced_mac, fpw);
                        if (alloced_mac[iLen - 1] != '\n') {
                            fputs("\n", fpw);
                        }
                    }
                    if (pPos)
                        fputs(pPos, fpw);

                    fclose(fpw);

                    if (rename(tmp_file_name, USES_MACs_FILE) < 0) {
                        //try again
                        //printf("bad news, rename failed from %s to %s\n", tmp_file_name, USES_MACs_FILE);
                        usleep(1 * 1000);
                        rename(tmp_file_name, USES_MACs_FILE);
                    }
                }
            }

            //STD output
            printf("%s\n", "successed");
        }
    }

    if (alloced_mac) {
        free(alloced_mac);
    }
}

void produce_free_all_wan_mac(int param_flag, char* param) {
    unlink(USES_MACs_FILE);
}
