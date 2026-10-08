#include "qcmap_mngr_info.h"
#include "qcmap_mngr_apn.h"

ST_MODULE_DESC  glb_module_desc;
CELL_ENTRY_LIST g_cell_entry_list;
int  need_update_cellinfo = 0;
int  last_cell_nettype    = 0;
char last_cell_pcid[32]   = {0};
char last_cell_rsrq[32]   = {0};
char last_cell_rsrp[32]   = {0};
char last_cell_rssi[32]   = {0};
char last_cell_sinr[32]   = {0};

void qcmap_release_info(void) {
    need_update_cellinfo = 0;
    last_cell_nettype    = 0;
    memset(last_cell_pcid, 0, sizeof(last_cell_pcid));
    memset(last_cell_rsrq, 0, sizeof(last_cell_rsrq));
    memset(last_cell_rsrp, 0, sizeof(last_cell_rsrp));
    memset(last_cell_rssi, 0, sizeof(last_cell_rssi));
    memset(last_cell_sinr, 0, sizeof(last_cell_sinr));
    memset(&glb_module_desc, 0, sizeof(glb_module_desc));
    memset(&g_cell_entry_list, 0, sizeof(g_cell_entry_list));
}

void qcmap_get_modem_name(void) {
    char result [32] = {0};
    char tmp_str[64]  = {0};

    read_first_line_from_file(QCMAP_MNGR_PID_FILE, result, sizeof(result));
    glb_module_desc.pid = strtoul(result, 0, 0);

    read_first_line_from_file(QCMAP_MNGR_VID_FILE, result, sizeof(result));
    glb_module_desc.vid = strtoul(result, 0, 0);

    write_buf_to_file(ODUMANUF_RESULT_FILE, QCMAP_MNGR_MANUFACTURER, strlen(QCMAP_MNGR_MANUFACTURER));
    write_buf_to_file(ODUMODEL_RESULT_FILE, QCMAP_MNGR_MODEL_NAME, strlen(QCMAP_MNGR_MODEL_NAME));

    sprintf(tmp_str, "%04x %04x\n%s\n", glb_module_desc.vid, glb_module_desc.pid, QCMAP_MNGR_MODEL_NAME);
    write_buf_to_file(USB_VID_PID_FILE, tmp_str, strlen(tmp_str));
    PRINT_DEBUG("modem name : [%s]\n", QCMAP_MNGR_MODEL_NAME);
}

void qcmap_get_sn(void) {
    char *p = NULL;
    char *p1 = NULL;
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CFSN);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("result:[%s]\n", result);
        if (strcasestr(result, "OK")) {
            if ((p = strstr(result, "\"")) != NULL) {
                p += 1;
                if ((p1 = strstr(p, "\"")) != NULL) {
                    *p1 = '\0';
                    write_buf_to_file(ODUSN_RESULT_FILE, p, strlen(p));
                    PRINT_DEBUG("get sn success [%s]\n", p);
                    return true;
                }
            }
        }
    }

    return false;
}

void qcmap_init_modem_info(void) {
    qcmap_get_modem_name();
    qcmap_get_module_version();
    qcmap_get_imsi();
    qcmap_get_imei();
    qcmap_get_iccid();
    qcmap_get_sn();
}

void qcmap_mobile_nr_cell_save(void) {
    char  tmp_str[32]  = {0};
    char  bufLine[128] = {0};
    FILE *fp           = fopen(CELL_NR_LIST_FILE, "w+");

    if (fp) {
        for (int i = 0; i < g_cell_entry_list.nr_cell_entry_num; i++) {
            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "nr_pcid%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, hex_str_to_dec_str(g_cell_entry_list.nr_cell_entry[i].pcid));
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "nr_arfcn%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, hex_str_to_dec_str(g_cell_entry_list.nr_cell_entry[i].arfcn));
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "nr_rsrp%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, g_cell_entry_list.nr_cell_entry[i].rsrp);
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "nr_rsrq%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, g_cell_entry_list.nr_cell_entry[i].rsrq);
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "nr_sinr%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, g_cell_entry_list.nr_cell_entry[i].sinr);
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "nr_cellid%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, g_cell_entry_list.nr_cell_entry[i].cellid);
            fputs(bufLine, fp);
        }
        fclose(fp);
    }
}

void qcmap_mobile_lte_cell_save(void) {
    char  tmp_str[32]  = {0};
    char  bufLine[128] = {0};
    FILE *fp           = fopen(CELL_LTE_LIST_FILE, "w+");

    if (fp) {
        for (int i = 0; i < g_cell_entry_list.lte_cell_entry_num; i++) {
            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "lte_pcid%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, hex_str_to_dec_str(g_cell_entry_list.lte_cell_entry[i].pcid));
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "lte_arfcn%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, hex_str_to_dec_str(g_cell_entry_list.lte_cell_entry[i].arfcn));
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "lte_rsrp%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, g_cell_entry_list.lte_cell_entry[i].rsrp);
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "lte_rsrq%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, g_cell_entry_list.lte_cell_entry[i].rsrq);
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "lte_sinr%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, g_cell_entry_list.lte_cell_entry[i].sinr);
            fputs(bufLine, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(bufLine, 0, sizeof(bufLine));
            sprintf(tmp_str, "lte_cellid%d", i);
            snprintf(bufLine, sizeof(bufLine), "%s=%s\n", tmp_str, g_cell_entry_list.lte_cell_entry[i].cellid);
            fputs(bufLine, fp);
        }
        fclose(fp);
    }
}

void qcmap_mobile_celllist_save(void) {
    qcmap_mobile_nr_cell_save();
    qcmap_mobile_lte_cell_save();
}

void qcmap_mobile_cell_num_save(void) {
    FILE *fp = fopen(CELL_NUM_FILE, "w+");
    if (fp) {
        char bufLine[128] = {0};

        snprintf(bufLine, sizeof(bufLine), "lte_num=%d\n", g_cell_entry_list.lte_cell_entry_num);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_num=%d\n", g_cell_entry_list.nr_cell_entry_num);
        fputs(bufLine, fp);

        fclose(fp);
    }
}

void qcmap_get_cell_list(void) {
    int  nr_count         = 0;
    int lte_count         = 0;
    int  read_flag        = 0;
    char buf[512]         = {0};
    FILE *fp              = NULL;

/* 
+GTCELLSCAN: 4,460,00,A174,BC,8109,6415B48,-96,-6,41,32,120
+GTCELLSCAN: 4,460,00,9FE8,BC,8109,6415B41,-99,-10,41,29,116
+GTCELLSCAN: 4,460,00,A0AE,BC,8109,6415B46,-95,-8,41,33,119
+GTCELLSCAN: 4,460,11,994,184,811B,EBED119,-95,-11,5,33,116
+GTCELLSCAN: 5,460,01,99240,C5,21000A,211017001,-98,-14,78,23,0
+GTCELLSCAN: 5,460,00,7B49E,2D7,280410,90D7E1004,-69,-11,41,52,0
*/
    PRINT_DEBUG("start cell scan ...\n");
    fp = popen("at-mngr -i 80 AT+GTCELLSCAN", "r");
    if (!fp) {
        PRINT_DEBUG("run at-mngr -i 80 AT+GTCELLSCAN fail\n");
        return;
    }
    while ((fgets(buf, sizeof(buf), fp)) != NULL) {
        PRINT_DEBUG("get_cell_list buf=%s\n",buf );
        if(strstr(buf,"+GTCELLSCAN:") && buf[13] == '4') {
           // if (!strcmp(glb_module_desc.rf.netType, "NR5G-NSA")) 
            //{
                sscanf(buf, "%*[^,],%*[^,],%*[^,],%[^,],%[^,],%*[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%*[^,]",
                g_cell_entry_list.lte_cell_entry[lte_count].arfcn, 
                g_cell_entry_list.lte_cell_entry[lte_count].pcid,
                g_cell_entry_list.lte_cell_entry[lte_count].cellid,
                g_cell_entry_list.lte_cell_entry[lte_count].rsrp,
                g_cell_entry_list.lte_cell_entry[lte_count].rsrq);
                PRINT_DEBUG("arfcn=%s pcid=%s cellid=%s rsrp=%s rsrq=%s\n",
                g_cell_entry_list.lte_cell_entry[lte_count].arfcn, 
                g_cell_entry_list.lte_cell_entry[lte_count].pcid, 
                g_cell_entry_list.lte_cell_entry[lte_count].cellid,
                g_cell_entry_list.lte_cell_entry[lte_count].rsrp,
                g_cell_entry_list.lte_cell_entry[lte_count].rsrq);

               // g_cell_entry_list.lte_cell_entry[lte_count].rsrq[strlen(g_cell_entry_list.lte_cell_entry[lte_count].rsrq) - 1] = '\0';
               // strncpy(g_cell_entry_list.lte_cell_entry[lte_count].cellid, glb_module_desc.rf.globalCellID, sizeof(g_cell_entry_list.lte_cell_entry[lte_count].cellid) - 1);
                lte_count++;
            } 
        if(strstr(buf,"+GTCELLSCAN:") && buf[13] == '5') {
            //if (!strcmp(glb_module_desc.rf.netType, "NR5G-NSA")) {
                sscanf(buf, "%*[^,],%*[^,],%*[^,],%[^,],%[^,],%*[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%*[^,]",
                g_cell_entry_list.nr_cell_entry[nr_count].arfcn, 
                g_cell_entry_list.nr_cell_entry[nr_count].pcid, 
                g_cell_entry_list.nr_cell_entry[nr_count].cellid,
                g_cell_entry_list.nr_cell_entry[nr_count].rsrp,
                g_cell_entry_list.nr_cell_entry[nr_count].rsrq);
                PRINT_DEBUG("arfcn=%s pcid=%s cellid=%s rsrp=%s rsrq=%s\n",
                g_cell_entry_list.nr_cell_entry[nr_count].arfcn, 
                g_cell_entry_list.nr_cell_entry[nr_count].pcid, 
                g_cell_entry_list.nr_cell_entry[nr_count].cellid,
                g_cell_entry_list.nr_cell_entry[nr_count].rsrp,
                g_cell_entry_list.nr_cell_entry[nr_count].rsrq);
                //g_cell_entry_list.nr_cell_entry[nr_count].rsrq[strlen(g_cell_entry_list.nr_cell_entry[nr_count].rsrq) - 1] = '\0';
                //strncpy(g_cell_entry_list.nr_cell_entry[nr_count].cellid, glb_module_desc.rf.globalCellID, sizeof(g_cell_entry_list.nr_cell_entry[nr_count].cellid) - 1);
                nr_count++;
            }
    }
    pclose(fp);

    g_cell_entry_list.nr_cell_entry_num = nr_count;
    g_cell_entry_list.lte_cell_entry_num = lte_count;
    PRINT_DEBUG("cell scan end\n");
}

void qcmap_get_imei(void) {
    int   i           = 0;
    char *p           = NULL;
    char  cmd[256]    = {0};
    char  tmpStr[256] = {0};
    char  imei[32]    = {0};
    FILE *fp          = NULL;

    sprintf(cmd, "at-mngr AT+CGSN?");
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(tmpStr, BUFLEN_256, fp) != NULL) {
        if (strstr(tmpStr, "AT") != NULL || strstr(tmpStr, "at") != NULL) {
            continue;
        }

        if (strlen(tmpStr) < 5) {
            continue;
        }

        if (strstr(tmpStr, "ERROR") != NULL || strstr(tmpStr, "error") != NULL) {
            continue;
        }

        for (i = 0; i < strlen(tmpStr); i++) {
            if (tmpStr[i] == '\n' || tmpStr[i] == '\r') {
                tmpStr[i] = 0;
                break;
            }
        }

        if (strncmp(tmpStr, "0x", 2) == 0) {
            if (is_valid_char(tmpStr + 2) == 1) {
                strcpy(imei, tmpStr);
                break;
            }
        } else if ((p = strstr(tmpStr, "CGSN:")) != NULL) {
            strcpy(tmpStr, p + strlen("CGSN:") + 1);
        }

        if (tmpStr[0] == '"') {
            sprintf(tmpStr, "%s", &tmpStr[1]);
            if ((p = strstr(tmpStr, "\"")) != NULL) {
                *p = 0;
            }
        }

        if (is_valid_char(tmpStr) == 0) {
            continue;
        }

        strcpy(imei, tmpStr);
        break;
    }
    pclose(fp);

    if (imei[0] == 0 || strlen(imei) < 5) {
        return;
    }

    if (strstr(imei, "tty") != NULL || strstr(imei, "TTY") != NULL || strstr(imei, "No") != NULL || strstr(imei, "NO") != NULL) {
        return;
    }
    write_buf_to_file(IMEI_RESULT_FILE, imei, strlen(imei));
    strncpy(glb_module_desc.imei, imei, sizeof(glb_module_desc.imei) - 1);
    PRINT_DEBUG("imei : [%s]\n", glb_module_desc.imei);
}

void qcmap_get_imsi(void) {
    int   i           = 0;
    char *p           = NULL;
    char  cmd[256]    = {0};
    char  tmpStr[256] = {0};
    char  imsi[32]    = {0};
    FILE *fp          = NULL;

    sprintf(cmd, "at-mngr  -i 2 AT+CIMI?");
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(tmpStr, BUFLEN_256, fp) != NULL) {
        if (strstr(tmpStr, "AT") != NULL || strstr(tmpStr, "at") != NULL) {
            continue;
        }

        if (strlen(tmpStr) < 5) {
            continue;
        }

        if (strstr(tmpStr, "ERROR") != NULL || strstr(tmpStr, "error") != NULL) {
            continue;
        }

        for (i = 0; i < strlen(tmpStr); i++) {
            if (tmpStr[i] == '\n' || tmpStr[i] == '\r') {
                tmpStr[i] = 0;
                break;
            }
        }

        if (strncmp(tmpStr, "0x", 2) == 0) {
            if (is_valid_char(tmpStr + 2) == 1) {
                strcpy(imsi, tmpStr);
                break;
            }
        } else if ((p = strstr(tmpStr, "CIMI:")) != NULL) {
            strcpy(tmpStr, p + strlen("CIMI:") + 1);
        }

        if (tmpStr[0] == '"') {
            sprintf(tmpStr, "%s", &tmpStr[1]);
            if ((p = strstr(tmpStr, "\"")) != NULL) {
                *p = 0;
            }
        }

        if (is_valid_char(tmpStr) == 0) {
            continue;
        }

        strcpy(imsi, tmpStr);
        break;
    }
    pclose(fp);

    if (imsi[0] == 0 || strlen(imsi) < 5) {
        return;
    }

    if (strstr(imsi, "tty") != NULL || strstr(imsi, "TTY") != NULL || strstr(imsi, "No") != NULL || strstr(imsi, "NO") != NULL) {
        return;
    }
    write_buf_to_file(IMSI_RESULT_FILE, imsi, strlen(imsi));
    strncpy(glb_module_desc.imsi, imsi, sizeof(glb_module_desc.imsi) - 1);
    PRINT_DEBUG("imsi : [%s]\n", glb_module_desc.imsi);
}

void qcmap_get_module_version(void) {
    int   i                 = 0;
    char *p                 = NULL;
    char  cmd[256]          = {0};
    char  tmpStr[256]       = {0};
    char  moduleVersion[64] = {0};
    FILE *fp                = NULL;

    sprintf(cmd, "at-mngr AT+GMR?");
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(tmpStr, BUFLEN_256, fp) != NULL) {
        if ((p = strstr(tmpStr, "+GMR:")) != NULL)
            strcpy(tmpStr, p + strlen("+GMR:") + 1);
        else
            continue;

        if (tmpStr[0] == '"') {
            sprintf(tmpStr, "%s", &tmpStr[1]);
            if ((p = strstr(tmpStr, "\"")) != NULL) {
                *p = 0;
            }
        }

        if (strlen(tmpStr) > 0) {
            strcpy(glb_module_desc.moduleVersion, tmpStr);
            write_buf_to_file(MODULE_VERSION_FILE, glb_module_desc.moduleVersion, strlen(glb_module_desc.moduleVersion));
            PRINT_DEBUG("module version : [%s]\n", glb_module_desc.moduleVersion);
        }
        break;
    }
    pclose(fp);
}

void qcmap_get_csq(void) {
    char *p           = NULL;
    char  cmd[256]    = {0};
    char  tmpStr[256] = {0};
    FILE *fp          = NULL;
    int   find        = 0;
    char *pend        = NULL;
    int rssi          = 0;

    sprintf(cmd, "at-mngr at+CSQ");
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(tmpStr, BUFLEN_256, fp) != NULL) {
        if ((p = strstr(tmpStr, "+CSQ:")) != NULL) {
            strcpy(tmpStr, p + strlen("+CSQ:") + 1);
            find = 1;
            break;
        }
    }
    pclose(fp);

    if (!find) {
        return;
    }

    pend = strchr(tmpStr, ',');
    if (pend == NULL) {
        return;
    }

    *pend               = '\0';
    glb_module_desc.csq = atoi(tmpStr);
    rssi = (atoi(tmpStr) * 2);
    if (rssi > 0) {
        rssi = -113 + rssi;
        sprintf(glb_module_desc.rf.RSSI, "%d", rssi);
        write_buf_to_file(RSSI_STATUS_FILE, glb_module_desc.rf.RSSI, strlen(glb_module_desc.rf.RSSI));
    } else {
        write_buf_to_file(RSSI_STATUS_FILE, tmpStr, strlen(tmpStr));
    }
}

void qcmap_update_mobile_info(void) {
    if(	strcmp(last_cell_pcid, glb_module_desc.rf.physicalCellID) || 
        strcmp(last_cell_rsrq, glb_module_desc.rf.RSRQ) || 
        strcmp(last_cell_rsrp, glb_module_desc.rf.RSRP) || 
        strcmp(last_cell_sinr, glb_module_desc.rf.SINR))
    {
        if (strcmp(last_cell_rsrp, glb_module_desc.rf.RSRP)) {
            PRINT_DEBUG("rsrp [%s] change to [%s]\n", last_cell_rsrp, glb_module_desc.rf.RSRP);
        }
        memset(last_cell_pcid, 0, sizeof(last_cell_pcid));
        sprintf(last_cell_pcid, "%s", glb_module_desc.rf.physicalCellID);
        memset(last_cell_rsrq, 0, sizeof(last_cell_rsrq));
        sprintf(last_cell_rsrq, "%s", glb_module_desc.rf.RSRQ);
        memset(last_cell_rsrp, 0, sizeof(last_cell_rsrp));
        sprintf(last_cell_rsrp, "%s", glb_module_desc.rf.RSRP);
        memset(last_cell_sinr, 0, sizeof(last_cell_sinr));
        sprintf(last_cell_sinr, "%s", glb_module_desc.rf.SINR);
        need_update_cellinfo = 1;
    }
}

int qcmap_update_signal_level(void) {
    char networkType[64] = {0};
    char signalVal[16]   = {0};
    int  iSignalVal      = 0;
    char signalLevel[16] = {0};
    int  iSignalLevel    = 0;

    read_first_line_from_file(NETWORK_TYPE_FILE, networkType, sizeof(networkType));

    if (strcasestr(networkType, "LTE") || strcasestr(networkType, "4G") || strcasestr(networkType, "5G")) {
        read_first_line_from_file(RSRP_RESULT_FILE, signalVal, sizeof(signalVal));
        iSignalVal = atoi(signalVal);
        if (iSignalVal == 0) {
            iSignalLevel = 0;
        } else if (iSignalVal < -140) {
            iSignalLevel = 0;
        } else if (iSignalVal < -115) {
            iSignalLevel = 1;
        } else if (iSignalVal < -105) {
            iSignalLevel = 2;
        } else if (iSignalVal < -95) {
            iSignalLevel = 3;
        } else if (iSignalVal < -85) {
            iSignalLevel = 4;
        } else {
            iSignalLevel = 5;
        }
    } else {
        return -1;
    }

    sprintf(signalLevel, "%d", iSignalLevel);
    write_buf_to_file(SIGNAL_LEVEL_FILE, signalLevel, strlen(signalLevel));

    return iSignalLevel;
}

void qcmap_get_iccid(void) {
    int   i           = 0;
    char *p           = NULL;
    char  cmd[256]    = {0};
    char  tmpStr[256] = {0};
    char  ccid[32]    = {0};
    FILE *fp          = NULL;

    sprintf(cmd, "at-mngr AT+ICCID");

    fp = popen(cmd, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(tmpStr, BUFLEN_256, fp) != NULL) {
        if (strstr(tmpStr, "AT") != NULL || strstr(tmpStr, "at") != NULL) {
            continue;
        }

        if (strlen(tmpStr) < 5) {
            continue;
        }

        if (strstr(tmpStr, "ERROR") != NULL || strstr(tmpStr, "error") != NULL) {
            continue;
        }

        for (i = 0; i < strlen(tmpStr); i++) {
            if (tmpStr[i] == '\n' || tmpStr[i] == '\r') {
                tmpStr[i] = 0;
                break;
            }
        }

        if (strncmp(tmpStr, "0x", 2) == 0) {
            if (is_valid_char(tmpStr + 2) == 1) {
                strcpy(ccid, tmpStr);
                break;
            }
        }

        else if ((p = strstr(tmpStr, "ICCID:")) != NULL) {
            strcpy(tmpStr, p + strlen("ICCID:") + 1);
        }

        if (tmpStr[0] == '"') {
            sprintf(tmpStr, "%s", &tmpStr[1]);
            if ((p = strstr(tmpStr, "\"")) != NULL) {
                *p = 0;
            }
        }

        if (is_valid_char(tmpStr) == 0) {
            continue;
        }

        strcpy(ccid, tmpStr);
        break;
    }
    pclose(fp);

    if (ccid[0] == 0 || strlen(ccid) < 5) {
        return;
    }

    if (strstr(ccid, "tty") != NULL || strstr(ccid, "TTY") != NULL || strstr(ccid, "No") != NULL || strstr(ccid, "NO") != NULL) {
        return;
    }
    write_buf_to_file(ICCID_RESULT_FILE, ccid, strlen(ccid));
    strncpy(glb_module_desc.ccid, ccid, sizeof(glb_module_desc.ccid) - 1);
    PRINT_DEBUG("iccid : [%s]\n", glb_module_desc.ccid);
}

void qcmap_get_cell_info(void) {
    int  is_nsa = 0;
    int  bNextLineIsCont = 0;
    char buf[512]         = {0};
    FILE *fp              = NULL;
    char *parm_list[16] = {0};
    cell_info_t cell_info;

    fp = popen("at-mngr AT+GTCCINFO?", "r");
    if (!fp) {
        PRINT_DEBUG("run at-mngr AT+GTCCINFO? fail\n");
        return;
    }

    while ((fgets(buf, sizeof(buf), fp)) != NULL) {
        if (strstr(buf, "service cell:")) {
            if (strstr(buf, "LTE-NR")) {
                strcpy(cell_info.netType, "NR5G-NSA");
            } else if (strstr(buf, "NR")) {
                strcpy(cell_info.netType, "NR5G-SA");
            } else if (strstr(buf, "LTE")) {
                strcpy(cell_info.netType, "LTE");
            } else if (strstr(buf, "UMTS") || strstr(buf, "WCDMA")) {
                strcpy(cell_info.netType, "3G");
            } else {
                strcpy(cell_info.netType, "-");
            }
            bNextLineIsCont = 1;
            continue;
        }

        if (!bNextLineIsCont) {
            continue;
        }
        if (strstr(buf, ",") && buf[0] == '1') {
            if (!strcmp(cell_info.netType, "NR5G-NSA")) {
                split_string_ext(buf,',',parm_list,14);
                sprintf(cell_info.tac,"%s",(parm_list[4] == NULL) ? "":parm_list[4]);
                sprintf(cell_info.cellid,"%s",(parm_list[5] == NULL) ? "":parm_list[5]);
                sprintf(cell_info.earfcn,"%s",(parm_list[6] == NULL) ? "":parm_list[6]);
                sprintf(cell_info.pcid,"%s",(parm_list[7] == NULL) ? "":parm_list[7]);
                sprintf(cell_info.band,"%s",(parm_list[8] == NULL) ? "":parm_list[8]);
                sprintf(cell_info.bandwidth,"%s",(parm_list[9] == NULL) ? "":parm_list[9]);
                sprintf(cell_info.rsrp,"%s",(parm_list[12] == NULL) ? "":parm_list[12]);
                sprintf(cell_info.rsrq,"%s",(parm_list[13] == NULL) ? "":parm_list[13]);

                //sscanf(buf, "%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,]", tac, cellid, earfcn, pcid,
                 //       band, bandwidth, rsrp, rsrq);
                if ((fgets(buf, sizeof(buf), fp)) != NULL) {
                    is_nsa = 1;
                    split_string_ext(buf,',',parm_list,14);
                    sprintf(cell_info.tac_rf2,"%s",(parm_list[4] == NULL) ? "":parm_list[4]);
                    sprintf(cell_info.cellid_rf2,"%s",(parm_list[5] == NULL) ? "":parm_list[5]);
                    sprintf(cell_info.earfcn_rf2,"%s",(parm_list[6] == NULL) ? "":parm_list[6]);
                    sprintf(cell_info.pcid_rf2,"%s",(parm_list[7] == NULL) ? "":parm_list[7]);
                    sprintf(cell_info.band_rf2,"%s",(parm_list[8] == NULL) ? "":parm_list[8]);
                    sprintf(cell_info.bandwidth_rf2,"%s",(parm_list[9] == NULL) ? "":parm_list[9]);
                    sprintf(cell_info.sinr_rf2,"%s",(parm_list[10] == NULL) ? "":parm_list[10]);
                    sprintf(cell_info.rsrp_rf2,"%s",(parm_list[12] == NULL) ? "":parm_list[12]);
                    sprintf(cell_info.rsrq_rf2,"%s",(parm_list[13] == NULL) ? "":parm_list[13]);
                   // sscanf(buf, "%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%[^,]", tac_rf2, cellid_rf2, earfcn_rf2, pcid_rf2,
                    //    band_rf2, bandwidth_rf2, sinr_rf2, rsrp_rf2, rsrq_rf2);
                }
            } else if (!strcmp(cell_info.netType, "NR5G-SA")) {
                split_string_ext(buf,',',parm_list,14);
                sprintf(cell_info.tac,"%s",(parm_list[4] == NULL) ? "":parm_list[4]);
                sprintf(cell_info.cellid,"%s",(parm_list[5] == NULL) ? "":parm_list[5]);
                sprintf(cell_info.earfcn,"%s",(parm_list[6] == NULL) ? "":parm_list[6]);
                sprintf(cell_info.pcid,"%s",(parm_list[7] == NULL) ? "":parm_list[7]);
                sprintf(cell_info.band,"%s",(parm_list[8] == NULL) ? "":parm_list[8]);
                sprintf(cell_info.bandwidth,"%s",(parm_list[9] == NULL) ? "":parm_list[9]);
                sprintf(cell_info.sinr,"%s",(parm_list[10] == NULL) ? "":parm_list[10]);
                sprintf(cell_info.ss_rsrp,"%s",(parm_list[12] == NULL) ? "":parm_list[12]);
                sprintf(cell_info.ss_rsrq,"%s",(parm_list[13] == NULL) ? "":parm_list[13]);
                //sscanf(buf, "%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%[^,]", tac, cellid, earfcn, pcid,
                 //       band, bandwidth, sinr, ss_rsrp, ss_rsrq);
            } else if (!strcmp(cell_info.netType, "LTE")) {
                split_string_ext(buf,',',parm_list,14);
                sprintf(cell_info.tac,"%s",(parm_list[4] == NULL) ? "":parm_list[4]);
                sprintf(cell_info.cellid,"%s",(parm_list[5] == NULL) ? "":parm_list[5]);
                sprintf(cell_info.earfcn,"%s",(parm_list[6] == NULL) ? "":parm_list[6]);
                sprintf(cell_info.pcid,"%s",(parm_list[7] == NULL) ? "":parm_list[7]);
                sprintf(cell_info.band,"%s",(parm_list[8] == NULL) ? "":parm_list[8]);
                sprintf(cell_info.bandwidth,"%s",(parm_list[9] == NULL) ? "":parm_list[9]);
                sprintf(cell_info.rsrp,"%s",(parm_list[12] == NULL) ? "":parm_list[12]);
                sprintf(cell_info.rsrq,"%s",(parm_list[13] == NULL) ? "":parm_list[13]);
                //sscanf(buf, "%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,]", tac, cellid, earfcn, pcid,
                 //       band, bandwidth, rsrp, rsrq);
            } else if (!strcmp(cell_info.netType, "3G")) {
                split_string_ext(buf,',',parm_list,15);;
                sprintf(cell_info.lac,"%s",(parm_list[4] == NULL) ? "":parm_list[4]);
                sprintf(cell_info.cellid,"%s",(parm_list[5] == NULL) ? "":parm_list[5]);
                sprintf(cell_info.earfcn,"%s",(parm_list[6] == NULL) ? "":parm_list[6]);
                sprintf(cell_info.band,"%s",(parm_list[8] == NULL) ? "":parm_list[8]);
                sprintf(cell_info.rscp,"%s",(parm_list[10] == NULL) ? "":parm_list[10]);

                //sscanf(buf, "%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%*[^,],%[^,],%*[^,],%*[^,],%*[^,],%*[^,]", lac, cellid, earfcn,
                 //       band, rscp);
            }
        } else {
            cell_info.netType[0] = 0;
        }

        break;
    }

    pclose(fp);

    if (!strcmp(cell_info.netType, "NR5G-SA")) {
        glb_module_desc.rsrq = (atof(cell_info.ss_rsrq) / 2);
        glb_module_desc.rsrq = -43 + glb_module_desc.rsrq;
        memset(cell_info.rsrq, 0x0, sizeof(cell_info.rsrq));
        sprintf(cell_info.rsrq, "%.1f", glb_module_desc.rsrq);

        glb_module_desc.rsrp = atoi(cell_info.ss_rsrp);
        glb_module_desc.rsrp = -156 + glb_module_desc.rsrp;
        memset(cell_info.rsrp, 0x0, sizeof(cell_info.rsrp));
        sprintf(cell_info.rsrp, "%d", glb_module_desc.rsrp);

        glb_module_desc.sinr = (atof(cell_info.sinr) / 2);
        glb_module_desc.sinr = -23 + glb_module_desc.sinr;
        memset(cell_info.sinr, 0x0, sizeof(cell_info.sinr));
        sprintf(cell_info.sinr, "%.1f", glb_module_desc.sinr);
    } else {
        glb_module_desc.rsrq = (atof(cell_info.rsrq) / 2);
        glb_module_desc.rsrq = -19.5 + glb_module_desc.rsrq;
        memset(cell_info.rsrq, 0x0, sizeof(cell_info.rsrq));
        sprintf(cell_info.rsrq, "%.1f", glb_module_desc.rsrq);

        glb_module_desc.rsrp = atoi(cell_info.rsrp);
        glb_module_desc.rsrp = -140 + glb_module_desc.rsrp;
        memset(cell_info.rsrp, 0x0, sizeof(cell_info.rsrp));
        sprintf(cell_info.rsrp, "%d", glb_module_desc.rsrp);
    }

    write_buf_to_file(NETWORK_TYPE_FILE, cell_info.netType, strlen(cell_info.netType));
    write_buf_to_file(RSRP_RESULT_FILE, cell_info.rsrp, strlen(cell_info.rsrp));
    strncpy(glb_module_desc.frequencyBand, cell_info.band, sizeof(glb_module_desc.frequencyBand) - 1);

    strncpy(glb_module_desc.rf.netType, cell_info.netType, sizeof(glb_module_desc.rf.netType) - 1);
    strncpy(glb_module_desc.rf.frequencyBand, cell_info.band, sizeof(glb_module_desc.rf.frequencyBand) - 1);
    // strncpy(glb_module_desc.rf.duplexingMode, duplexmode, sizeof(glb_module_desc.rf.duplexingMode) - 1);
    strncpy(glb_module_desc.rf.RSRQ, cell_info.rsrq, sizeof(glb_module_desc.rf.RSRQ) - 1);
    strncpy(glb_module_desc.rf.RSRP, cell_info.rsrp, sizeof(glb_module_desc.rf.RSRP) - 1);
    // strncpy(glb_module_desc.rf.RSSI, rssi, sizeof(glb_module_desc.rf.RSSI) - 1);
    strncpy(glb_module_desc.rf.SINR, cell_info.sinr, sizeof(glb_module_desc.rf.SINR) - 1);
    strncpy(glb_module_desc.rf.TAC, cell_info.tac, sizeof(glb_module_desc.rf.TAC) - 1);
    strncpy(glb_module_desc.rf.DLEARFCN, cell_info.earfcn, sizeof(glb_module_desc.rf.DLEARFCN) - 1);
    strncpy(glb_module_desc.rf.physicalCellID, cell_info.pcid, sizeof(glb_module_desc.rf.physicalCellID) - 1);
    strncpy(glb_module_desc.rf.globalCellID, cell_info.cellid, sizeof(glb_module_desc.rf.globalCellID) - 1);
    strncpy(glb_module_desc.rf.bandWidth, cell_info.bandwidth, sizeof(glb_module_desc.rf.bandWidth) - 1);

    if (is_nsa) {
        int nr_rsrp = 0;
        float nr_rsrq = 0;
        float nr_sinr = 0;

        nr_rsrq = (atof(cell_info.rsrq_rf2) / 2);
        nr_rsrq = -43 + nr_rsrq;
        memset(cell_info.rsrq_rf2, 0x0, sizeof(cell_info.rsrq_rf2));
        sprintf(cell_info.rsrq_rf2, "%.1f", nr_rsrq);

        nr_rsrp = atoi(cell_info.rsrp_rf2);
        nr_rsrp = -156 + nr_rsrp;
        memset(cell_info.rsrp_rf2, 0x0, sizeof(cell_info.rsrp_rf2));
        sprintf(cell_info.rsrp_rf2, "%d", nr_rsrp);

        nr_sinr = (atof(cell_info.sinr_rf2) / 2);
        nr_sinr = -23 + nr_sinr;
        memset(cell_info.sinr_rf2, 0x0, sizeof(cell_info.sinr_rf2));
        sprintf(cell_info.sinr_rf2, "%.1f", nr_sinr);

        strncpy(glb_module_desc.rf2.SINR, cell_info.sinr_rf2, sizeof(glb_module_desc.rf2.SINR) - 1);
        strncpy(glb_module_desc.rf2.RSRP, cell_info.rsrp_rf2, sizeof(glb_module_desc.rf2.RSRP) - 1);
        strncpy(glb_module_desc.rf2.RSRQ, cell_info.rsrq_rf2, sizeof(glb_module_desc.rf2.RSRQ) - 1);
        strncpy(glb_module_desc.rf2.globalCellID, cell_info.cellid_rf2, sizeof(glb_module_desc.rf2.globalCellID) - 1);
        strncpy(glb_module_desc.rf2.DLEARFCN, cell_info.earfcn_rf2, sizeof(glb_module_desc.rf2.DLEARFCN) - 1);
        strncpy(glb_module_desc.rf2.physicalCellID, cell_info.pcid_rf2, sizeof(glb_module_desc.rf2.physicalCellID) - 1);
        strncpy(glb_module_desc.rf2.frequencyBand, cell_info.band_rf2, sizeof(glb_module_desc.rf2.frequencyBand) - 1);
    }
}

void qcmap_get_rate(void) {
    char *p           = NULL;
    char  cmd[256]    = {0};
    char  tmpStr[256] = {0};
    FILE *fp          = NULL;
    int   find        = 0;
    char *token       = NULL;

    sprintf(cmd, "at-mngr at+GTSTATIS?");
    fp = popen(cmd, "r");
    if (fp == NULL) {
        PRINT_DEBUG("run at-mngr AT+GTSTATIS? fail\n");
        return;
    }

    while (fgets(tmpStr, 256, fp) != NULL) {
        if ((p = strstr(tmpStr, "+GTSTATIS:")) != NULL) {
            // 提取+GTSTATIS:后面的内容
            strcpy(tmpStr, p + strlen("+GTSTATIS:") + 1);
            find = 1;
            break;
        }
    }
    pclose(fp);

    if (!find) {
        PRINT_DEBUG("run at-mngr AT+GTSTATIS? no value return\n");
        return;
    }

    // 分割字符串获取前两个值
    token = strtok(tmpStr, ",");
    if (token != NULL) {
        strncpy(glb_module_desc.rf3.dlrate, token, sizeof(glb_module_desc.rf3.dlrate) - 1);  // 留一个位置给结束符
    } else {
        PRINT_DEBUG("GTSTATIS format error: no first value\n");
        return;
    }

    token = strtok(NULL, ",");
    if (token != NULL) {
        strncpy(glb_module_desc.rf3.ulrate, token, sizeof(glb_module_desc.rf3.ulrate) - 1);
    } else {
        PRINT_DEBUG("GTSTATIS format error: no second value\n");
        return;
    }
}

void qcmap_update_more_rf(void) {
    char tmpStr[512] = {0};
    int  len         = 0;

    len = snprintf(tmpStr, sizeof(tmpStr), "netType=%s\n", glb_module_desc.rf.netType);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "dl_rate=%s\n", glb_module_desc.rf3.dlrate);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "ul_rate=%s\n", glb_module_desc.rf3.ulrate);

    write_buf_to_file(MORE_RF_PARA_FILE, tmpStr, strlen(tmpStr));
}

void qcmap_update_mobilestatus_rf(void) {
    char tmpStr[1024] = {0};
    int  len          = 0;

    len = snprintf(tmpStr, sizeof(tmpStr), "duplexingMode=%s\n", glb_module_desc.rf.duplexingMode);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "netType=%s\n", glb_module_desc.rf.netType);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "PLMN=%s\n", glb_module_desc.rf.PLMN);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "CQI=%s\n", glb_module_desc.rf.CQI);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "RANK=%s\n", glb_module_desc.rf.RANK);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "SINR=%s\n", glb_module_desc.rf.SINR);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "RSRQ=%s\n", glb_module_desc.rf.RSRQ);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "RSRP=%s\n", glb_module_desc.rf.RSRP);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "RSSI=%s\n", glb_module_desc.rf.RSSI);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "MCS=%s\n", glb_module_desc.rf.rxMCS);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "TAC=%s\n", glb_module_desc.rf.TAC);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "physicalCellID=%s\n", hex_str_to_dec_str(glb_module_desc.rf.physicalCellID));
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "globalCellID=%s\n", glb_module_desc.rf.globalCellID);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "DLEARFCN=%s\n", hex_str_to_dec_str(glb_module_desc.rf.DLEARFCN));
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "band=%s\n", glb_module_desc.rf.frequencyBand);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "bandWidth=%s\n", glb_module_desc.rf.bandWidth);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "CAActive=%d\n", glb_module_desc.rf.CAActive);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "CAInfo=%s\n", glb_module_desc.rf.CAInfo);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "transmissionMode=%s\n", glb_module_desc.rf.transmissionMode);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "currentDownstreamRate=%s\n", glb_module_desc.rf.currentDownstreamRate);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "currentUpstreamRate=%s\n", glb_module_desc.rf.currentUpstreamRate);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "downlinkMaxThrp=%s\n", glb_module_desc.rf.downlinkMaxThrp);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "uplinkMaxThrp=%s\n", glb_module_desc.rf.uplinkMaxThrp);

    write_buf_to_file(RFPARAM_INFOS_FILE, tmpStr, strlen(tmpStr));
}

void qcmap_update_mobilestatus_rf2(void) {
    char tmpStr[512] = {0};
    int  len         = 0;

    len = snprintf(tmpStr, sizeof(tmpStr), "NR-PCID=%s\n", glb_module_desc.rf2.physicalCellID);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "NR-ARFCN=%s\n", hex_str_to_dec_str(glb_module_desc.rf2.DLEARFCN));
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "NR-BAND=%s\n", glb_module_desc.rf2.frequencyBand);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "SS-RSRP=%s\n", glb_module_desc.rf2.RSRP);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "SS-RSRQ=%s\n", glb_module_desc.rf2.RSRQ);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "SS-SINR=%s\n", glb_module_desc.rf2.SINR);

    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "NR-MCS=%s\n", glb_module_desc.rf2.rxMCS);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "NR-RANK=%s\n", glb_module_desc.rf2.RANK);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "NR-ULMCS=%s\n", glb_module_desc.rf2.txMCS);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "NR-CQI=%s\n", glb_module_desc.rf2.CQI);
    len += snprintf(tmpStr + len, sizeof(tmpStr) - len, "NR-netType=%s\n", glb_module_desc.rf2.netType);

    write_buf_to_file(NSA_SIGNAL_FILE, tmpStr, strlen(tmpStr));
}

void qcmap_mobile_cellinfo_save(void) {
    if (!strcmp(glb_module_desc.rf.netType, "NR5G-SA")) {
        mobile_nr5g_sa_cellinfo_save();
    } else if (!strcmp(glb_module_desc.rf.netType, "NR5G-NSA")) {
        mobile_nr5g_nsa_cellinfo_save();
    } else if (!strcmp(glb_module_desc.rf.netType, "LTE")) {
        mobile_lte_cellinfo_save();
    }
}

void mobile_lte_cellinfo_save(void) {
    FILE *fp = fopen(CELL_INFO_FILE, "w+");
    if (fp) {
        char bufLine[128] = {0};

        snprintf(bufLine, sizeof(bufLine), "netType=%s\n", glb_module_desc.rf.netType);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_cellid=%s\n", glb_module_desc.rf.globalCellID);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_pcid=%s\n", hex_str_to_dec_str(glb_module_desc.rf.physicalCellID));
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_tac=%s\n", glb_module_desc.rf.TAC);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_arfcn=%s\n", hex_str_to_dec_str(glb_module_desc.rf.DLEARFCN));
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rssi=%s\n", glb_module_desc.rf.RSSI);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rsrp=%s\n", glb_module_desc.rf.RSRP);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rsrq=%s\n", glb_module_desc.rf.RSRQ);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_sinr=%s\n", glb_module_desc.rf.SINR);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_band=%s\n", glb_module_desc.rf.frequencyBand);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_cellid=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_pcid=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_tac=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_arfcn=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rssi=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rsrp=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rsrq=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_sinr=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_band=%s\n", "");
        fputs(bufLine, fp);

        fclose(fp);
    }
}

void mobile_nr5g_sa_cellinfo_save(void) {
    FILE *fp = fopen(CELL_INFO_FILE, "w+");
    if (fp) {
        char bufLine[128] = {0};

        snprintf(bufLine, sizeof(bufLine), "netType=%s\n", glb_module_desc.rf.netType);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_cellid=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_pcid=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_tac=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_arfcn=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rssi=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rsrp=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rsrq=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_sinr=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_band=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_cellid=%s\n", glb_module_desc.rf.globalCellID);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_pcid=%s\n", hex_str_to_dec_str(glb_module_desc.rf.physicalCellID));
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_tac=%s\n", glb_module_desc.rf.TAC);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_arfcn=%s\n", hex_str_to_dec_str(glb_module_desc.rf.DLEARFCN));
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rssi=%s\n", glb_module_desc.rf.RSSI);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rsrp=%s\n", glb_module_desc.rf.RSRP);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rsrq=%s\n", glb_module_desc.rf.RSRQ);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_sinr=%s\n", glb_module_desc.rf.SINR);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_band=%s\n", glb_module_desc.rf.frequencyBand);
        fputs(bufLine, fp);

        fclose(fp);
    }
}

void mobile_nr5g_nsa_cellinfo_save(void) {
    FILE *fp = fopen(CELL_INFO_FILE, "w+");
    if (fp) {
        char bufLine[128] = {0};

        snprintf(bufLine, sizeof(bufLine), "netType=%s\n", glb_module_desc.rf.netType);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_cellid=%s\n", glb_module_desc.rf.globalCellID);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_pcid=%s\n", hex_str_to_dec_str(glb_module_desc.rf.physicalCellID));
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_tac=%s\n", glb_module_desc.rf.TAC);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_arfcn=%s\n", hex_str_to_dec_str(glb_module_desc.rf.DLEARFCN));
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rssi=%s\n", glb_module_desc.rf.RSSI);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rsrp=%s\n", glb_module_desc.rf.RSRP);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_rsrq=%s\n", glb_module_desc.rf.RSRQ);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_sinr=%s\n", glb_module_desc.rf.SINR);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "lte_band=%s\n", glb_module_desc.rf.frequencyBand);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_cellid=%s\n", glb_module_desc.rf2.globalCellID);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_pcid=%s\n", hex_str_to_dec_str(glb_module_desc.rf2.physicalCellID));
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_tac=%s\n", "");
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_arfcn=%s\n", hex_str_to_dec_str(glb_module_desc.rf2.DLEARFCN));
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rssi=%s\n", glb_module_desc.rf2.RSSI);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rsrp=%s\n", glb_module_desc.rf2.RSRP);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_rsrq=%s\n", glb_module_desc.rf2.RSRQ);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_sinr=%s\n", glb_module_desc.rf2.SINR);
        fputs(bufLine, fp);
        snprintf(bufLine, sizeof(bufLine), "nr_band=%s\n", glb_module_desc.rf2.frequencyBand);
        fputs(bufLine, fp);

        fclose(fp);
    }
}

//获取modem相关信息
void qcmap_timer_check_info(void) {
    static unsigned int s_last_check_time = 0;
    unsigned int now_time = qcmap_get_uptime_in_ms();

    if (POS_DIFF_VAL(now_time, s_last_check_time) < MOD_STATUS_UPDATE_INTERVAL) {
        return;
    }

    s_last_check_time = now_time;

    if (access(SCAN_CELLLIST_FLG, F_OK) == 0) {
        qcmap_get_cell_list();
        qcmap_mobile_cell_num_save();
        qcmap_mobile_celllist_save();
        unlink(SCAN_CELLLIST_FLG);
    }

    qcmap_get_cell_info();
    qcmap_update_mobile_info();
    qcmap_get_rate();
    qcmap_update_more_rf();
    if (need_update_cellinfo != 0) {
        qcmap_update_mobilestatus_rf();
        qcmap_update_mobilestatus_rf2();
        //qcmap_get_csq(); //only 2G 3G need to update CSQ
        qcmap_mobile_cellinfo_save();
        qcmap_update_signal_level();
        need_update_cellinfo = 0;
    }
    if (access(PROVIDER_RESULT_FILE, F_OK) != 0) {
        qcmap_get_provider();
    }
}

void qcmap_reset_signal(void) {
    memset(glb_module_desc.rf.RSRP, '\0', sizeof(glb_module_desc.rf.RSRP));
    memset(glb_module_desc.rf.RSSI, '\0', sizeof(glb_module_desc.rf.RSSI));
    glb_module_desc.csq = 0;
}