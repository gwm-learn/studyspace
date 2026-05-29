#include "produce_cwmp.h"
#include "produce_util.h"

void produce_cwmp(int param_flag, char* param) {
    int ret;
    char tmp_buf[32] = {0};

    if (param_flag != PARAM_FLAG_HAS_SUBCOMMAND) {
        printf("!INPUT ERROR!\n");
        return;
    }

    ret = strcmp(param, "manufacturer");
    if (ret == 0) {
        printf("%s", CUSTOMER_MANUFACTURER);
        return;
    }

    ret = strcmp(param, "product_class");
    if (ret == 0) {
        get_produce_param(PRODUCE_PRODUCTCLASS_KEY, NULL, tmp_buf);
        printf("%s", tmp_buf[0] ? tmp_buf : CUSTOMER_PRODUCT_CLASS);
        return;
    }

    ret = strcmp(param, "hardware_version");
    if (ret == 0) {
        printf("%s", CUSTOMER_HW_VER);
        return;
    }

    ret = strcmp(param, "software_version");
    if (ret == 0) {
        printf("%s", CUSTOMER_SW_VER);
        return;
    }

    ret = strcmp(param, "oui");
    if (ret == 0) {
        get_produce_param(PRODUCE_MACADDR_KEY, NULL, tmp_buf);
        change_to_upcase(tmp_buf);
        tmp_buf[6] = 0;  // 6 byte prefix of mac
        printf("%s", tmp_buf);
        return;
    }

    ret = strcmp(param, "serial_number");
    if (ret == 0) {
        get_produce_param(PRODUCE_SERIALN_KEY, NULL, tmp_buf);
        if (tmp_buf[0] == 0) {
            get_produce_param(PRODUCE_MACADDR_KEY, NULL, tmp_buf);
            change_to_upcase(tmp_buf);
        }
        printf("%s", tmp_buf);
        return;
    }

    printf("!INPUT ERROR!\n");
}