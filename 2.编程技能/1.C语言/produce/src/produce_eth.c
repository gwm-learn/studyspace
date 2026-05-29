#include "produce_eth.h"
#include "produce_util.h"

static int get_port_num(const char* param) {
    if (strncmp(param, "port", 4) == 0) {
        return atoi(param + 4);
    }
    return -1;    // Invalid port string
}

static int get_link_status(char* ifname) {
    int status = 0;
    char cmd[32] = {0};
    char buf[256] = {0};
    FILE* fp = NULL;

    sprintf(cmd, "ethtool %s", ifname);
    fp = popen(cmd, "r");
    if (fp) {
        while ((fgets(buf, sizeof(buf) - 1, fp)) != NULL) {
            if (strstr(buf, "Link detected: yes") != NULL) {
                status = 1;
                break;
            }
        }

        pclose(fp);
    }

    return status;
}

void produce_eth(int param_flag, char* param) {
    int port_num = 0;
    int status = -1;

    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        port_num = get_port_num(param);
        switch (port_num) {
            case 0:
                status = get_link_status("eth1");
                break;
            case 1:
                status = get_link_status("lan1");
                break;
            case 2:
                status = get_link_status("lan2");
                break;
            case 3:
                status = get_link_status("lan3");
                break;
            default:
                printf("!INPUT ERROR!\n");
                return;
        }

        if (status == 0) {
            printf("~!LinkDown!~\n");
        } else if (status == 1) {
            printf("~!LinkUp!~\n");
        }
    } else {
        printf("!INPUT ERROR!\n");
    }
}