#include "qcmap_mngr_util.h"

int g_apn_num = 1;
int g_max_apn_num = 1;
//int g_apn_auth_num = 0;
boolean state_machine_running_status = false;
int global_state_machine_status = STATE_MACHINE_NONE;
QCMAP_Client* qcmap_client = NULL;
boolean is_ipv6nat_enabled = false;
unsigned long indication_mask = QCMAP_REG_ALL_IND_MASK;
char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0}; /*char array of mac address*/
qcmap_wan5g_config_t qcmap_wan5g_list[QCMAP_MNGR_DEFAULT_PROFILE_NUM];

/* MTPE History Buffers */
static mtpe_history_entry* mtpe_history_entries = NULL;
static uint32_t mtpe_txn_id = 0;
static uint32_t len_mtpe_history = 0;
static boolean mtpe_history_ready_flag = false;
static pthread_mutex_t mtpe_hist_mutex;

void qcmap_release_client(void) {
    g_apn_num = 1;
    g_max_apn_num = 1;
    //g_apn_auth_num = 0;
}

void qcmap_release_util(void) {
    qcmap_init_info_file();
}

boolean qcmap_get_bridge_enable(int index) {
    if (index < QCMAP_MNGR_DEFAULT_PROFILE_NUM)
        return qcmap_wan5g_list[index].bridge_enable;
    else
        return false;
}

void qcmap_set_bridge_enable(int index, bool enable) {
    if (index < QCMAP_MNGR_DEFAULT_PROFILE_NUM)
        qcmap_wan5g_list[index].bridge_enable = enable;
}

boolean qcmap_get_state_machine_running_status(void) {
    return state_machine_running_status;
}

void qcmap_enable_state_machine(void) {
    PRINT_DEBUG("enable state machine!\n");
    state_machine_running_status = true;
}

void qcmap_disable_state_machine(void) {
    PRINT_DEBUG("disable state machine!\n");
    state_machine_running_status = false;
}

void qcmap_update_state_machine_status(int status) {
    PRINT_DEBUG("update state machine status [%d] to [%d]\n", global_state_machine_status, status);
    global_state_machine_status = status;
}

int qcmap_get_state_machine_status(void) {
    return global_state_machine_status;
}

void qcmap_init_cfun(bool reload) {
    static bool init = false;
 
    if (!init || reload) { 
        if (!init) { 
            init = true; 
        }

        if (access(QCMAP_BOOT_FILE, F_OK) == 0) {
            qcmap_execute_cfun_sequence(CFUN_0); 
            qcmap_enable_state_machine(); 
            qcmap_update_state_machine_status(STATE_MACHINE_NONE);
        }
    }
}

/**
 * @brief set CFUN settings with enhanced state machine
 *
 * This function performs the following operations:
 * 1. If value is 1, set CFUN to 1 and return immediately
 * 2. If value is not 1, perform state machine operations:
 *    - Set CFUN to specified value
 *    - Set CFUN to 1 (full functionality mode)
 *    - Verify CFUN settings
 *
 * @param value CFUN value to set
 */
void qcmap_execute_cfun_sequence(int value) {
    static int wait_num = 0;
    static cfun_state_t current_state = CFUN_STATE_INIT;
    
    PRINT_DEBUG("Starting CFUN set, target value: %d\n", value);
    
    // If value is 1, set directly and return
    if (value == CFUN_1) {
        PRINT_DEBUG("Setting CFUN to 1 directly\n");
        if (qcmap_update_cfun(CFUN_1)) {
            PRINT_DEBUG("CFUN set to 1 successfully\n");
        }
        current_state = CFUN_STATE_INIT; // Reset state machine
        return;
    }
    
    while (1) {
        switch (current_state) {
            case CFUN_STATE_INIT:
                // Step 1: Set CFUN to specified value
                PRINT_DEBUG("Step 1: Setting CFUN to value %d\n", value);
                if (qcmap_update_cfun(value)) {
                    current_state = CFUN_STATE_SET_VALUE;
                    PRINT_DEBUG("Step 1 completed: CFUN set to %d\n", value);
                }
                break;
                
            case CFUN_STATE_SET_VALUE:
                // Step 2: Set CFUN to 1 (full functionality mode)
                PRINT_DEBUG("Step 2: Setting CFUN to full functionality mode(1)\n");
                if (qcmap_update_cfun(CFUN_1)) {
                    current_state = CFUN_STATE_SET_CFUN1;
                    PRINT_DEBUG("Step 2 completed: CFUN set to full functionality mode\n");
                }
                break;
                
            case CFUN_STATE_SET_CFUN1:
                // Step 3: Verify CFUN settings
                PRINT_DEBUG("Step 3: Verifying CFUN settings\n");
                if (qcmap_check_cfun(CFUN_1)) {
                    wait_num = 0;
                    current_state = CFUN_STATE_INIT;  // Reset state machine
                    PRINT_DEBUG("Step 3 completed: CFUN verification successful, reset process completed\n");
                    return;  // Successfully completed, exit function
                } else {
                    wait_num++;
                    PRINT_DEBUG("wait cfun 1 result\n");
                }
                if (wait_num == 5) {
                    wait_num = 0;
                    PRINT_DEBUG("wait cfun 1 for 5 time, retry cfun 1\n");
                    current_state = CFUN_STATE_SET_VALUE;
                }
                break;
                
            default:
                // Unknown state, reset to initial state
                PRINT_DEBUG("Warning: Unknown state %d, resetting state machine\n", current_state);
                current_state = CFUN_STATE_INIT;
                break;
        }
        
        // Wait 1.5 second after each loop to avoid too frequent operations
        usleep(1.5 * 1000 * 1000);
    }
}

bool qcmap_update_cfun(int value){
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    PRINT_DEBUG("update cfun %d\n", value);
    snprintf(command, QCMAP_MAX_STRING_LEN, "%s %s=%d", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CFUN, value);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("[%s]\n", result);
        if (strcasestr(result, "OK")) {
            return true;
        }
        if (strcasestr(result, "Receive timed out")) {
            return false;
        }
    }

    return false;
}

/**
 * @brief Check if CFUN is set to the expected value
 *
 * @param expected_value Expected CFUN value to check
 * @return true if CFUN is set to expected value, false otherwise
 */
bool qcmap_check_cfun(int expected_value){
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};
    char expected_str[16] = {0};

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CFUN);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("[%s]\n", result);
        // Create the expected string pattern
        snprintf(expected_str, sizeof(expected_str), "+CFUN: %d", expected_value);
        if (strcasestr(result, expected_str)) {
            PRINT_DEBUG("CFUN check passed: expected %d, actual %d\n", expected_value, expected_value);
            return true;
        }
        if (strcasestr(result, "Receive timed out")) {
            PRINT_DEBUG("CFUN check failed: AT command timeout\n");
            return false;
        }
    }

    PRINT_DEBUG("CFUN check failed: expected %d, but got different value\n", expected_value);
    return false;
}

unsigned int qcmap_get_uptime_in_ms(void) {
    struct timespec currTime = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &currTime) != 0) {
        PRINT_DEBUG("bad news, get time err!\n");
    }

    return (unsigned int)(currTime.tv_sec * 1000U + currTime.tv_nsec / 1000000);
}

char *hex_str_to_dec_str(const char *hex_str) {
    unsigned long long int hex = 0;
    static char dec_str[16];

    memset(dec_str, '\0', sizeof(dec_str));
    hex = strtoull(hex_str, NULL, 16);
    sprintf(dec_str, "%llu", hex);
    return dec_str;
}

void qcmap_init_info_file(void) {
    if (access(DIR_MOBILE_PATH, F_OK) != 0) {
        system_ex("mkdir -p " DIR_MOBILE_PATH, 0);
    } else {
        system_ex("find " DIR_MOBILE_PATH " -type f ! -name '*_support' ! -name '*boot' -exec rm {} +", 0);
    }
}

int read_interface_info_v6(const char *interface, int *ifindex, char *addr6, uint8_t *arp) {
    FILE         *f;
    int           scope, prefix_len;
    unsigned char ipv6[16];
    char          dname[IFNAMSIZ];
    char          address[INET6_ADDRSTRLEN];
    int           ret = -1;

    f = fopen("/proc/net/if_inet6", "r");
    if (f == NULL) {
        return ret;
    }

    while (0 < fscanf(f, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx %*x %x %x %*x %s", &ipv6[0], &ipv6[1], &ipv6[2],
                      &ipv6[3], &ipv6[4], &ipv6[5], &ipv6[6], &ipv6[7], &ipv6[8], &ipv6[9], &ipv6[10], &ipv6[11], &ipv6[12], &ipv6[13], &ipv6[14], &ipv6[15],
                      &prefix_len, &scope, dname)) {

        if (strcmp(interface, dname) != 0) {
            continue;
        }

        if (inet_ntop(AF_INET6, ipv6, address, sizeof(address)) == NULL) {
            continue;
        }

        // printf("IPv6 address: %s, prefix: %d, scope: %x\n", address, prefix_len, scope);
        if (strncasecmp("FE80", address, strlen("FE80")) == 0) {
            continue;
        }

        sprintf(addr6, "%s/%d", address, prefix_len);
        // printf("IPv6 address: %s, prefix: %d, \n", addr6, prefix_len);
        ret = 0;
        break;
    }

    fclose(f);

    return ret;
}

int read_interface_info_v4(const char *interface, int *ifindex, uint32_t *addr, uint8_t *arp) {
    int                 fd;
    struct ifreq        ifr;
    struct sockaddr_in *our_ip;

    memset(&ifr, 0, sizeof(ifr));
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));

    if (addr) {
        if (ioctl(fd, SIOCGIFADDR, &ifr, "is interface %s up and configured?", interface)) {
            close(fd);
            return -1;
        }
        our_ip = (struct sockaddr_in *)&ifr.ifr_addr;
        *addr  = our_ip->sin_addr.s_addr;
        // printf("%s (our ip) = %s\n", ifr.ifr_name, inet_ntoa(our_ip->sin_addr));
    }

    if (ifindex) {
        if (ioctl(fd, SIOCGIFINDEX, &ifr) != 0) {
            close(fd);
            return -1;
        }
        // printf("adapter index %d\n", ifr.ifr_ifindex);
        *ifindex = ifr.ifr_ifindex;
    }

    if (arp) {
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) != 0) {
            close(fd);
            return -1;
        }
        memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
        // printf("adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x\n",arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
    }

    close(fd);
    return 0;
}

int is_valid_char(char *str) {
    int  i           = 0;
    char validchar[] = "0123456789ABCDEFabcdef\n\r";

    for (i = 0; i < strlen(str); i++) {
        if (strchr(validchar, str[i]) == NULL) {
            return 0;
        }
    }

    return 1;
}

int system_ex(char *command, int printFlag) {
    int pid = 0, status = 0;

    if (!command) {
        PRINT_DEBUG("system_ex: Null Command, Error!");
        return -1;
    }

    PRINT_DEBUG("command:%s\n", command);

    pid = fork();
    if (pid == -1) {
        PRINT_DEBUG("system_ex fork fail\n");
        return -1;
    }

    if (pid == 0) {
        char    *argv[4];
        sigset_t sigset;
        int      sig = 0;

        sigemptyset(&sigset);
        for (sig = 0; sig < (_NSIG - 1); sig++) {
            sigaddset(&sigset, sig);
        }
        sigprocmask(SIG_UNBLOCK, &sigset, NULL);
        for (sig = 0; sig < (_NSIG - 1); sig++) {
            signal(sig, SIG_DFL);
        }

        argv[0] = "sh";
        argv[1] = "-c";
        argv[2] = command;
        argv[3] = 0;
        if (printFlag) {
            PRINT_DEBUG("[system]: %s\r\n", command);
        }
        execv("/bin/sh", argv);
        exit(127);
    }

    /* wait for child process return */
    do {
        if (waitpid(pid, &status, 0) == -1) {
            if (errno != EINTR && errno != ECHILD) {
                PRINT_DEBUG("system_ex waitpid fail, errno->%s\n", strerror(errno));
                return -1;
            } else if (errno == ECHILD) {
                return 0;
            }
        } else {
            return status;
        }
    } while (1);

    return status;
}

bool execute_cmd(const char *cmd, char *result, uint8_t result_len) {
    if (!cmd || !result || result_len == 0) {
        PRINT_DEBUG("Invalid arguments passed\n", 0, 0, 0);
        return false;
    }

    memset(result, 0, result_len);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        printf("Failed to execute command: %s\n", cmd);
        return false;
    }

    size_t bytes_read = fread(result, 1, result_len - 1, fp);
    pclose(fp);
    result[bytes_read] = '\0';

    if (bytes_read > 0) {
        size_t len = bytes_read;
        while (len > 0 && (result[len-1] == '\n' || result[len-1] == '\r')) {
            len--;
        }
        result[len] = '\0';
    }

    return true;
}

bool qcmap_get_uci_value(char* package, char* section, char* name, char* value, int value_len) {
    char               *p_name = NULL;
    struct uci_context *ctx    = NULL;
    struct uci_ptr      p;
    struct uci_element *e         = NULL;
    char                buf[1024] = {0};
    bool                sep       = false;
    int                 len       = 0;
    char                uci_full_name[256] = {0};

    if (!package || !section || !name || !value) {
        PRINT_DEBUG("Parameter error!!!");
        return false;
    }

    memset(value, 0, value_len);

    snprintf(uci_full_name, sizeof(uci_full_name), "%s.%s.%s", package, section, name);
    p_name = strdup(uci_full_name);
    ctx    = uci_alloc_context(); /* register a context */

    if (uci_lookup_ptr(ctx, &p, p_name, true) != UCI_OK) {
        PRINT_DEBUG("uci_lookup_ptr ERROR!\n");
        free(p_name);
        return false;
    }

    if (p.flags & UCI_LOOKUP_COMPLETE) {
        e = p.last;
        switch (e->type) {
        case UCI_TYPE_SECTION:
            strncpy(value, p.s->type, value_len);
            break;
        case UCI_TYPE_OPTION:
            if (p.o->type == UCI_TYPE_STRING) {
                strncpy(value, p.o->v.string, value_len);
            } else if (p.o->type == UCI_TYPE_LIST) {
                uci_foreach_element(&p.o->v.list, e) {
                    if (sep) {
                        len += sprintf(buf + len, " %s", e->name);
                    } else {
                        len += sprintf(buf + len, "%s", e->name);
                    }
                    sep = true;
                }
                strncpy(value, buf, value_len);
            }
            break;
        default:
            break;
        }
    }

    uci_free_context(ctx);
    free(p_name);

    return true;
}

int qcmap_set_uci_value(char* package, char* section, char* name, char* value) {
    struct uci_ptr      ptr;
    struct uci_context *ctx = NULL;
    struct uci_element *e   = NULL;
    char                buf[512];
    int                 ret = -1;
    char                uci_full_name[256] = {0};

    if (!package || !section || !name || !value) {
        printf("Parameter error!!!");
        return -1;
    }

    snprintf(uci_full_name, sizeof(uci_full_name), "%s.%s.%s", package, section, name);

    memset(buf, 0, 512);
    ctx = uci_alloc_context();

    snprintf(buf, 512, "%s=%s", uci_full_name, value);

    if (uci_lookup_ptr(ctx, &ptr, buf, true) == UCI_OK) {
        e = ptr.last;
        uci_set(ctx, &ptr);
        uci_commit(ctx, &ptr.p, false);
        uci_unload(ctx, ptr.p);
        ret = 1;
    }

    uci_free_context(ctx);
    return ret;
}

int qcmap_uci_addlist(const char *name, const char *value) {
    int                 ret = -1;
    struct uci_ptr      ptr;
    char                buf[512] = {};
    struct uci_context *l_ctx    = uci_alloc_context(); // 申请上下文
    sprintf(buf, "%s=%s", name, value);

    if (uci_lookup_ptr(l_ctx, &ptr, buf, true) != UCI_OK) {
        printf("%s-%d:uci_load %s error!!!\n", __FILE__, __LINE__, name);
        goto FREE_UCI_CONTENT;
    }

    if (uci_add_list(l_ctx, &ptr) != UCI_OK) {
        goto FREE_UCI_PACKAGE;
    }

    ret = 0;
    uci_save(l_ctx, ptr.p);
    uci_commit(l_ctx, &ptr.p, false); // 提交保存更改
FREE_UCI_PACKAGE:
    uci_unload(l_ctx, ptr.p); // 卸载包
FREE_UCI_CONTENT:
    uci_free_context(l_ctx); // 释放上下文

    return ret;
}

int qcmap_uci_dellist(const char *name, const char *value) {
    int                 ret = -1;
    struct uci_ptr      ptr;
    char                buf[512] = {};
    struct uci_context *l_ctx    = uci_alloc_context(); // 申请上下文
    sprintf(buf, "%s=%s", name, value);

    if (uci_lookup_ptr(l_ctx, &ptr, buf, true) != UCI_OK) {
        printf("%s-%d:uci_load %s error!!!\n", __FILE__, __LINE__, name);
        goto FREE_UCI_CONTENT;
    }
    if (uci_del_list(l_ctx, &ptr) != UCI_OK) {
        goto FREE_UCI_PACKAGE;
    }

    ret = 0;
    uci_save(l_ctx, ptr.p);
    uci_commit(l_ctx, &ptr.p, false); // 提交保存更改
FREE_UCI_PACKAGE:
    uci_unload(l_ctx, ptr.p); // 卸载包
FREE_UCI_CONTENT:
    uci_free_context(l_ctx); // 释放上下文

    return ret;
}

int write_buf_to_file(const char *filename, const char *buf, int len) {
    FILE *in  = 0;
    int   ret = 0;

    if ((in = fopen(filename, "w+")) != NULL) {
        fwrite(buf, 1, len, in);
        fclose(in);
        ret = 1;
    } else {
        ret = 0;
    }

    return ret;
}

int read_first_line_from_file(char *fileName, char *line, int lineSize) {
    FILE *fp = NULL;

    if ((fp = fopen(fileName, "r")) == NULL) {
        printf("failed to open file %s", fileName);
        return -1;
    }

    memset(line, 0, lineSize);

    if (fgets(line, lineSize, fp)) {
        /*Terminate CR*/
        int iLen = strlen(line);
        if (iLen > 0 && (line[iLen - 1] == '\n' || line[iLen - 1] == '\r'))
            line[iLen - 1] = '\0';
    }

    fclose(fp);

    return 0;
}

int split_string_ext(char *rule_str, char ge, char **array, int max) {
    int   i = 0, j = 0;
    char *cp    = rule_str;
    int   is_ge = 0;

    if (rule_str == NULL)
        return 0;

    for (j = 0; j < max; j++)
        array[j] = NULL;

    while (cp && *cp && i < max) {
        array[i++] = cp;
        is_ge      = 0;
        while (*cp && *cp != ge) {
            cp++;
        }
        if (*cp) {
            is_ge = 1;
            *cp++ = 0;
        }
    }
    if ((is_ge == 1) && (i < max))
        array[i++] = cp;
    return i;
}

int split_string(const char *src, const char *delim, char dest[][STR_LEN_16], int max) {
    int index = 0;
    char *token = NULL;

    token = strtok(src, delim);
    while( token != NULL ) {
        if(strlen(token) >= STR_LEN_16)
            return 0;
        if(index >= max)
            return index;
        strncpy(dest[index], token, STR_LEN_16);
        index++;
        token = strtok(NULL, delim);
    }
    return index;
}

void get_ip_from_route(const char* interface, const char* ip, char *gateway) { 
    FILE *fp; 
    char path[1035]; 
    char line[1024]; 
 
    // 执行 ip route 命令并打开其输出作为文件流 
    fp = popen("ip route", "r"); 
    if (fp == NULL) { 
        PRINT_DEBUG("Failed to run command\n"); 
        return; 
    } 
 
    // 逐行读取 ip route 命令的输出 
    while (fgets(line, sizeof(line), fp) != NULL) { 
        // 检查当前行是否包含指定的接口名 
        if (strstr(line, interface) != NULL && strstr(line, ip) != NULL) { 
            // 解析出 IP 地址，格式为 x.x.x.x/xx 
            char *token = strtok(line, " "); 
            if (token != NULL) { 
                // 复制 IP 地址到结果缓冲区 
                sprintf(gateway, "%s", token); 
                // 去除可能的子网掩码信息 
                char *slash = strchr(gateway, '/'); 
                if (slash != NULL) { 
                    *slash = '\0'; 
                } 
                pclose(fp); 
                return;
            } 
        } 
    } 
 
    // 没有找到匹配的接口 
    pclose(fp); 
    return ; 
} 

void get_default_route(char *addr) { 
    FILE *fp; 
    char line[1024]; 
 
    // 执行命令获取默认路由 
    fp = popen("ip route | grep default | awk '{print $3}'", "r"); 
    if (fp == NULL) { 
        PRINT_DEBUG("Failed to run command\n"); 
        return; 
    } 
 
    while (fgets(line, sizeof(line), fp) != NULL) { 
        if (qcmap_check_ipv4_address(line)) { 
            // 复制地址到 addr 
            sprintf(addr, "%s", line); 
            // 查找并移除换行符 
            char *newline = strchr(addr, '\n'); 
            if (newline != NULL) { 
                *newline = '\0'; 
            } 
            //PRINT_DEBUG("get default route : [%s]\n", addr); 
            break; 
        } 
    } 
 
    pclose(fp); 
    return; 
} 

bool check_default_route(void) { 
    FILE *fp; 
    bool flag = false;
    char path[1035]; 
    char line[1024]; 
 
    // 执行 ip route 命令并打开其输出作为文件流 
    fp = popen("ip route", "r"); 
    if (fp == NULL) { 
        PRINT_DEBUG("Failed to run command\n"); 
        return false; 
    } 
 
    // 逐行读取 ip route 命令的输出 
    while (fgets(line, sizeof(line), fp) != NULL) { 
        if (strstr(line, "default") != NULL) { 
            flag = true;
            break;
        } 
    } 
    pclose(fp); 
    return flag; 
} 

bool qcmap_check_ipv4_address(const char *addr) {
    int IP[4];

    if (4 == sscanf(addr, "%d.%d.%d.%d", &IP[0], &IP[1], &IP[2], &IP[3]))
    {
        if (0 <= IP[0] && IP[0] <= 255 
            && 0 <= IP[1] && IP[1] <= 255 
            && 0 <= IP[2] && IP[2] <= 255 
            && 0 <= IP[3] && IP[3] <= 255)
        {
            return true;
        }
    }

    PRINT_DEBUG("ipv4 address (%s) err!\n", addr);

    return false;
}

void print_mtpe_entry(mtpe_history_entry* entry) {
    PRINT_DEBUG("%s [RTT: %u ms, GPS: (%.5f, %.5f), Peak Throughput: %.5f kbps]\n", ctime(&entry->timestamp), entry->avg_ping_time, entry->latitude, entry->longitude, entry->peak_rate);
}

void qcmap_msgr_qmi_qcmap_ind(qmi_client_type user_handle, /* QMI user handle       */
                              unsigned int msg_id,         /* Indicator message ID  */
                              void* ind_buf,               /* Raw indication data   */
                              unsigned int ind_buf_len,    /* Raw data length       */
                              void* ind_cb_data            /* User call back handle */
) {
    qmi_client_error_type qmi_error;
    profile_handle_type_v01 profile_handle;
    qcmap_msgr_subscription_enum_v01 subs_id = QCMAP_MSGR_SUBSCRIPTION_ENUM_MAX_ENUM_VAL_V01;
    char command[MAX_COMMAND_STR_LEN] = {0}, buffer[MAX_BACKHAUL_TYPE_LENGTH] = {0};
    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: user_handle %X msg_id %d ind_buf_len %d.\n", user_handle, msg_id, ind_buf_len);

    switch (msg_id) {
        case QMI_QCMAP_MSGR_PACKET_STATS_STATUS_IND_V01: {
            qcmap_msgr_packet_stats_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_packet_stats_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }
            /* Process packet service status indication for packet stats for QCMAP*/
            switch (ind_data.conn_status) {
                case QCMAP_MSGR_PACKET_STATS_CLIENT_CONNECTED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: A new client is Connected\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_CLIENT_DISCONNECTED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Client is disconnected\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_IPV4_UPDATED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: IPV4 Updated\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_IPV6_UPDATED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: IPV6 Updated\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_WLAN_DISABLED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: WLAN Disabled\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_MOBILEAP_DISABLED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: MOBILEAP Teardown\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_USB_DISCONNECTED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: USB Disconnected\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_IPV4_WWAN_DISCONNECTED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: IPV4 BH Disconnected\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_IPV6_WWAN_DISCONNECTED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: IPV6 BH Disconnected\n\n");
                    break;
                case QCMAP_MSGR_PACKET_STATS_BH_SWITCHED_V01:
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Backhaul is Switched \n\n");
                    break;
            }
            DisplayClientInfo(ind_data);
            break;
        }

        case QMI_QCMAP_MSGR_BRING_UP_WWAN_IND_V01: {
            qcmap_msgr_bring_up_wwan_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_bring_up_wwan_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }

            profile_handle = (ind_data.profile_handle_valid == TRUE) ? ind_data.profile_handle : 0xFFFF;
            subs_id = (ind_data.subs_id_valid == TRUE) ? ind_data.subs_id : 0xFFFF;
            /* Process packet service status indication for WWAN for QCMAP*/
            if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Connected\n", profile_handle, subs_id);
                    PRINT_BACKHAUL_WWAN_DETAILS(ind_data);

                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d IPV4 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d)  IPV6 WWAN Connected\n", profile_handle, subs_id);

                    PRINT_BACKHAUL_WWAN_DETAILS(ind_data);
                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_FAIL_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTED_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d)  ETH WWAN Connected\n", profile_handle, subs_id);
                    PRINT_DEBUG("\n   Eth Wwan Interface Name                 : %s\n", ind_data.wwan_info.iface_name);
                    PRINT_DEBUG("\n   Eth Wwan Vlan Mapping Id Start          : %d\n", ind_data.wwan_info.vlan_start);
                    PRINT_DEBUG("\n   Eth Wwan Vlan Mapping Id End            : %d\n", ind_data.wwan_info.vlan_end);
                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTING_FAIL_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

                    return;
                }
            }
            break;
        }
        case QMI_QCMAP_MSGR_TEAR_DOWN_WWAN_IND_V01: {
            qcmap_msgr_tear_down_wwan_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_tear_down_wwan_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }

            profile_handle = (ind_data.profile_handle_valid == TRUE) ? ind_data.profile_handle : 0xFFFF;
            subs_id = (ind_data.subs_id_valid == TRUE) ? ind_data.subs_id : 0xFFFF;
            if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Disconnected...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Disconnected...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTING_FAIL_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTED_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Disconnected...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

                    return;
                }
            } else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTING_FAIL_V01) {
                if (ind_data.mobile_ap_handle == qcmap_client->mobile_ap_handle) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                    return;
                }
            }
            break;
        }
        case QMI_QCMAP_MSGR_WWAN_STATUS_IND_V01: {
            char file_name[64] = {0};
            qcmap_msgr_wwan_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_wwan_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }

            profile_handle = (ind_data.profile_handle_valid == TRUE) ? ind_data.profile_handle : 0xFFFF;
            subs_id = (ind_data.subs_id_valid == TRUE) ? ind_data.subs_id : 0xFFFF;
            if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01) {
                sprintf(file_name, "%s_%d", QCMAP_MNGR_DIAL_DISCONNECT_IPV4, profile_handle);
                write_buf_to_file(file_name, "1", sizeof("1"));
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Disconnected...WAN CallendType=%d, CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01) {
                sprintf(file_name, "%s_%d", QCMAP_MNGR_DIAL_CONNECT_IPV4, profile_handle);
                write_buf_to_file(file_name, "1", sizeof("1"));
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPv4 WWAN Connected...\n", profile_handle, subs_id);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01) {
                sprintf(file_name, "%s_%d", QCMAP_MNGR_DIAL_DISCONNECT_IPV6, profile_handle);
                write_buf_to_file(file_name, "1", sizeof("1"));
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Disconnected...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTING_FAIL_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01) {
                sprintf(file_name, "%s_%d", QCMAP_MNGR_DIAL_CONNECT_IPV6, profile_handle);
                write_buf_to_file(file_name, "1", sizeof("1"));
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Connected...\n", profile_handle, subs_id);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_FAIL_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTED_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Disconnected...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTING_FAIL_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTED_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Connected...\n", profile_handle, subs_id);
                return;
            } else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTING_FAIL_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d\n", profile_handle, subs_id, ind_data.wwan_call_end_reason.wwan_call_end_reason_type, ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
                return;
            }

            break;
        }
        case QMI_QCMAP_MSGR_MOBILE_AP_STATUS_IND_V01: {
            qcmap_msgr_mobile_ap_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_mobile_ap_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }

            if (ind_data.mobile_ap_status == QCMAP_MSGR_MOBILE_AP_STATUS_CONNECTED_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Connected...\n");
                return;
            } else if (ind_data.mobile_ap_status == QCMAP_MSGR_MOBILE_AP_STATUS_DISCONNECTED_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Disconnected...\n");
                return;
            }
            break;
        }

        case QMI_QCMAP_MSGR_STATION_MODE_STATUS_IND_V01: {
            qcmap_msgr_station_mode_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_station_mode_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }

            if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_CONNECTED_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode Connected...\n");
                return;
            } else if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_DISCONNECTED_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode Disconnected...\n");
                return;
            } else if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_ASSOCIATION_FAIL_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode Association Failed. Going back to AP+STA Router Mode\n");
                return;
            } else if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_DHCP_IP_ASSIGNMENT_FAIL_V01) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode IP Assignment via DHCP Failed. Will switch to Static IP if available\n");
                return;
            }
            break;
        }

        case QMI_QCMAP_MSGR_CONNECTED_DEVICES_INFO_IND_V01: {
            qmi_error_type_v01 qmi_err_num;
            /*Initialize QMI Error Number*/
            qmi_err_num = QMI_ERR_NONE_V01;
            qcmap_msgr_connected_devices_info_ind_msg_v01 ind_data;
            char tmpIPv4[INET_ADDRSTRLEN] = {0};
            in6_addr allipv6[QCMAP_MSGR_MAX_IPV6_ADDR_V01];
            in6_addr tmpipv6;
            boolean flag = false;
            uint8 zero_buff[QCMAP_MSGR_IPV6_ADDR_LEN_V01] = {0};
            char ip6_addr_buf[INET6_ADDRSTRLEN] = {0};
            int connDevCount = 0, num_entries = 0;
            char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0}; /*char array of mac address*/

            ZERO_INIT_ARG(ind_data);
            ZERO_INIT_ARG(tmpIPv4);
            ZERO_INIT_ARG(tmpipv6);
            ZERO_INIT_ARG(ip6_addr_buf);
            ZERO_INIT_ARG(mac_addr_str);

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_connected_devices_info_ind_msg_v01));

            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }

            if (ind_data.transaction_id_valid) {
                PRINT_DEBUG("Indication received with transaction ID : %d\n", ind_data.transaction_id);
            } else {
                PRINT_DEBUG("Indication received with invalid transaction ID\n");
                break;
            }

            if (ind_data.connected_devices_info_valid) {
                num_entries = ind_data.connected_devices_info_len;
                PRINT_DEBUG("Num of Connected Devices Entries in this indication: %d vlan_id:%d\n", num_entries, ind_data.connected_devices_info[0].vlan_id);

                PRINT_DEBUG("\n Printing Connected Device Info for this indication : \n");
                for (connDevCount = 0; connDevCount < num_entries; connDevCount++) {
                    PRINT_DEBUG("Device No : %d \n", connDevCount + 1);
                    ds_mac_addr_ntop(ind_data.connected_devices_info[connDevCount].client_mac_addr, mac_addr_str);
                    PRINT_DEBUG("MAC Address : %s \n", mac_addr_str);
                    if (inet_ntop(AF_INET, (void*)&ind_data.connected_devices_info[connDevCount].ipv4_addr, tmpIPv4, INET_ADDRSTRLEN)) {
                        PRINT_DEBUG("IPv4 Address : %s \n", tmpIPv4);
                    }
                    memset(&tmpipv6, 0, sizeof(tmpipv6));
                    memcpy(&tmpipv6.s6_addr, ind_data.connected_devices_info[connDevCount].ll_ipv6_addr, QCMAP_MSGR_IPV6_ADDR_LEN_V01);
                    if (inet_ntop(AF_INET6, (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf))) {
                        PRINT_DEBUG("Link Local IPv6 Address : %s\n", ip6_addr_buf);
                    }

                    if (is_ipv6nat_enabled && flag) {
                        memset(&tmpipv6, 0, sizeof(tmpipv6));

                        memcpy(&tmpipv6.s6_addr, ind_data.connected_devices_info[connDevCount].ula_ipv6_addr, QCMAP_MSGR_IPV6_ADDR_LEN_V01);

                        if (inet_ntop(AF_INET6, (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf)) != NULL) {
                            PRINT_DEBUG("ULA IPv6 Address : %s\n", ip6_addr_buf);
                        }
                    }

                    for (int i = 0; i < QCMAP_MSGR_MAX_IPV6_ADDR_V01; i++) {
                        memset(&allipv6[i], 0, sizeof(in6_addr));
                        memset(ip6_addr_buf, 0, INET6_ADDRSTRLEN);

                        memcpy(&allipv6[i].s6_addr, ind_data.connected_devices_info[connDevCount].ipv6[i].addr, QCMAP_MSGR_IPV6_ADDR_LEN_V01);
                        if (!memcmp(&allipv6[i].s6_addr, zero_buff, QCMAP_MSGR_IPV6_ADDR_LEN_V01))
                            break;
                        if (inet_ntop(AF_INET6, (void*)&allipv6[i], ip6_addr_buf, sizeof(ip6_addr_buf))) {
                            PRINT_DEBUG("IPv6 Address %d: %s\n", i, ip6_addr_buf);
                        }
                    }

                    switch (ind_data.connected_devices_info[connDevCount].device_type) {
                        case QCMAP_MSGR_DEVICE_TYPE_PRIMARY_AP_V01:
                            PRINT_DEBUG("Device Type : Primary AP\n");
                            break;
                        case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_V01:
                            PRINT_DEBUG("Device Type :Guest AP1\n");
                            break;
                        case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_2_V01:
                            PRINT_DEBUG("Device Type :Guest AP2\n");
                            break;
                        case QCMAP_MSGR_DEVICE_TYPE_USB_V01:
                            PRINT_DEBUG("Device Type :USB\n");
                            break;
                        case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01:
                            PRINT_DEBUG("Device Type :Ethernet\n");
                            break;
                        default:
                            PRINT_DEBUG("Device Type : Invalid\n");
                            break;
                    }
                    PRINT_DEBUG("Host Name : %s\n", ind_data.connected_devices_info[connDevCount].host_name);
                    PRINT_DEBUG("rx bytes : %lu\n", ind_data.connected_devices_info[connDevCount].bytes_rx);
                    PRINT_DEBUG("tx bytes : %lu\n", ind_data.connected_devices_info[connDevCount].bytes_tx);
                    PRINT_DEBUG("Lease Expiry Time (in minutes) : %d\n", ind_data.connected_devices_info[connDevCount].lease_expiry_time);
                    if (ind_data.connected_devices_info[connDevCount].vlan_id != 0) {
                        PRINT_DEBUG("VLAN ID :%d\n", ind_data.connected_devices_info[connDevCount].vlan_id);
                    }
                }

                if (ind_data.num_inds_pending_valid && ind_data.num_inds_pending > 0) {
                    PRINT_DEBUG("%d follow-on indications are pending for transaction id : %d\n", ind_data.num_inds_pending, ind_data.transaction_id);
                }
            }
            break;
        }

        case QMI_QCMAP_MSGR_CRADLE_MODE_STATUS_IND_V01: {
            qcmap_msgr_cradle_mode_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_cradle_mode_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            if (ind_data.cradle_status == QCMAP_MSGR_CRADLE_CONNECTED_V01) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "Mobile AP Cradle mode Connected...\n");
                return;
            } else if (ind_data.cradle_status == QCMAP_MSGR_CRADLE_DISCONNECTED_V01) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "Mobile AP Cradle mode Disconnected...\n");
                return;
            }
            break;
        }

        case QMI_QCMAP_MSGR_WLAN_STATUS_IND_V01: {
            qcmap_msgr_wlan_status_ind_msg_v01 ind_data;
            int i = 0;
            in_addr ip4_addr;
            char ip6_addr[INET6_ADDRSTRLEN];

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_wlan_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }

            if (ind_data.scm_ind_valid) {
                switch (ind_data.scm_ind) {
                    case QCMAP_MSGR_SCM_DYNAMIC_RECONFIG_IND_V01:
                        PRINT_DEBUG("QCMAP_MSGR_SCM_DYNAMIC_RECONFIG_IND_V01 SCM indication received\n");
                        break;
                    case QCMAP_MSGR_SCM_STATION_STATE_IND_V01:
                        PRINT_DEBUG("QCMAP_MSGR_SCM_STATION_STATE_IND_V01 SCM indication received\n");
                        break;
                    case QCMAP_MSGR_SCM_SYS_CONTROL_IND_V01:
                        PRINT_DEBUG("QCMAP_MSGR_SCM_SYS_CONTROL_IND_V01 SCM indication received\n");
                        break;
                    default:
                        break;
                }
            } else {
                if (ind_data.wlan_status == QCMAP_MSGR_WLAN_ENABLED_V01) {
                    PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN is ENABLED...\n");
                } else if (ind_data.wlan_status == QCMAP_MSGR_WLAN_DISABLED_V01) {
                    PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN is DISABLED...\n");
                } else if (ind_data.wlan_status == QCMAP_MSGR_WLAN_SWITCH_TO_2G_V01) {
                    PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN AP/STA trying to switch to 2G...\n");
                    if (ind_data.wlan_switch_status == QCMAP_WLAN_2G_SWITCH_SUCCESS_V01 || ind_data.wlan_switch_status == QCMAP_WLAN_STA_SWITCH_SUCCESS_V01) {
                        if (ind_data.wlan_switch_status == QCMAP_WLAN_STA_SWITCH_SUCCESS_V01) {
                            PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN STA is switched to 2G...\n");
                        }
                        if (ind_data.wlan_switch_status == QCMAP_WLAN_2G_SWITCH_SUCCESS_V01) {
                            PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN AP is switched to 2G...\n");
                        }
                    } else {
                        if (ind_data.wlan_switch_status == QCMAP_WLAN_2G_SWITCH_FAIL_V01) {
                            PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN failed to switch to 2G...\n");
                        }
                    }
                } else if (ind_data.wlan_status == QCMAP_MSGR_WLAN_SWITCH_TO_5G_V01) {
                    PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN AP/STA trying to switch to 5G...\n");
                    if (ind_data.wlan_switch_status == QCMAP_WLAN_5G_SWITCH_SUCCESS_V01 || ind_data.wlan_switch_status == QCMAP_WLAN_STA_SWITCH_SUCCESS_V01) {
                        if (ind_data.wlan_switch_status == QCMAP_WLAN_5G_SWITCH_SUCCESS_V01) {
                            PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN AP is switched to 5G...\n");
                        }
                        if (ind_data.wlan_switch_status == QCMAP_WLAN_STA_SWITCH_SUCCESS_V01) {
                            PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN STA is switched to 5G...\n");
                        }
                    } else {
                        if (ind_data.wlan_switch_status == QCMAP_WLAN_5G_SWITCH_FAIL_V01) {
                            PRINT_DEBUG("\n qcmap_msgr_qmi_qcmap_ind: WLAN failed to switch to 5G...\n");
                        }
                    }
                } else {
                    PRINT_DEBUG("\n Invalid wlan status %d \n", ind_data.wlan_status);
                }

                if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_V01) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: WLAN is in AP Mode...\n");
                } else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_V01) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+AP Mode...\n");
                } else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_V01) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+AP+AP Mode...\n");
                } else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+STA Mode...\n");
                    PRINT_DEBUG("STA is in %s mode...\n", (ind_data.bridge_mode ? "Bridge" : "Router"));
                } else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+AP+STA Mode...\n");
                    PRINT_DEBUG("STA is in %s mode...\n", (ind_data.bridge_mode ? "Bridge" : "Router"));
                } else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: WLAN is STA Mode...\n");
                    PRINT_DEBUG("STA is in %s mode...\n", (ind_data.bridge_mode ? "Bridge" : "Router"));
                } else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_AP_V01) {
                    PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+AP+AP+AP Mode...\n");
                } else {
                    PRINT_DEBUG(" Invalid wlan mode %d ...\n", ind_data.wlan_mode);
                }

                for (i = 0; i < ind_data.wlan_state_len; i++) {
                    PRINT_DEBUG("\n WLAN State for Iface %s \n", ind_data.wlan_state[i].wlan_iface_name);
                    PRINT_DEBUG("IP type %d \n", ind_data.wlan_state[i].ip_type);
                    PRINT_DEBUG("Iface type %d \n", ind_data.wlan_state[i].wlan_iface_type);

                    if (ind_data.wlan_state[i].wlan_iface_state == QCMAP_MSGR_WLAN_CONNECTED_V01) {
                        PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Iface state is connected...\n");
                    } else if (ind_data.wlan_state[i].wlan_iface_state == QCMAP_MSGR_WLAN_DISCONNECTED_V01) {
                        PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Iface state is disconnected...\n");
                    } else if (ind_data.wlan_state[i].wlan_iface_state == QCMAP_MSGR_WLAN_CONNECTING_V01) {
                        PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Iface state is connecting...\n");
                    } else {
                        PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: Incorrect Iface state %d \n", ind_data.wlan_state[i].wlan_iface_state);
                    }

                    ip4_addr.s_addr = ind_data.wlan_state[i].ip4_addr;
                    PRINT_DEBUG("IP4 address of the iface: %s\n", inet_ntoa(ip4_addr));

                    inet_ntop(AF_INET6, (void*)&ind_data.wlan_state[i].ip6_addr, ip6_addr, sizeof(ip6_addr));
                    PRINT_DEBUG("IPv6 Address of the iface: %s\n", ip6_addr);
                }
            }
            break;
        }
        case QMI_QCMAP_MSGR_WLAN_STATUS_EX_IND_V01: {
            qcmap_msgr_wlan_status_ex_ind_msg_v01 ind_data;
            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_wlan_status_ex_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n", qmi_error);
                break;
            }
            if (ind_data.module_load_status_valid) {
                switch (ind_data.module_load_status) {
                    case QCMAP_MSGR_WLAN_SUCCESS_V01: {
                        PRINT_DEBUG("WLAN Module successfully loaded\n");
                        break;
                    }

                    case QCMAP_MSGR_WLAN_FAILURE_V01: {
                        PRINT_DEBUG("WLAN Module failed to load\n");
                        break;
                    }

                    default:
                        break;
                }
            }
            if (ind_data.hostapd_attach_status_valid) {
                switch (ind_data.hostapd_attach_status) {
                    case QCMAP_MSGR_WLAN_SUCCESS_V01: {
                        PRINT_DEBUG("HostAPD Attach Success\n");
                        if (ind_data.wlan_iface_name_valid) {
                            PRINT_DEBUG(" for Iface Name: %s\n", ind_data.wlan_iface_name);
                        }
                        if (ind_data.ap_type_valid) {
                            PRINT_DEBUG(" AP Type: %d\n", ind_data.ap_type);
                        }
                        break;
                    }

                    case QCMAP_MSGR_WLAN_FAILURE_V01: {
                        PRINT_DEBUG("HostAPD Attach Failure\n");
                        if (ind_data.wlan_iface_name_valid) {
                            PRINT_DEBUG(" for Iface Name: %s\n", ind_data.wlan_iface_name);
                        }
                        if (ind_data.ap_type_valid) {
                            PRINT_DEBUG(" AP Type: %d\n", ind_data.ap_type);
                        }
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
        case QMI_QCMAP_MSGR_ETHERNET_MODE_STATUS_IND_V01: {
            qcmap_msgr_ethernet_mode_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_ethernet_mode_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            if (ind_data.eth_status == QCMAP_MSGR_ETH_BACKHAUL_CONNECTED_V01) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "Mobile AP ETHERNET Backhaul Connected...\n");
                return;
            } else if (ind_data.eth_status == QCMAP_MSGR_ETH_BACKHAUL_DISCONNECTED_V01) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "Mobile AP ETHERNET Backhaul Disconnected...\n");
                return;
            }
            break;
        }
        case QMI_QCMAP_MSGR_BACKHAUL_STATUS_IND_V01: {
            qcmap_msgr_backhaul_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_backhaul_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            if (ind_data.backhaul_type_valid == TRUE) {
                if (convert_backhaul_enum_to_string(ind_data.backhaul_type, buffer)) {
                    if (ind_data.backhaul_v4_status_valid == TRUE) {
                        snprintf(command, MAX_COMMAND_STR_LEN, "IPV4 %s Backhaul %s...\n", buffer, (ind_data.backhaul_v4_status) ? "connected" : "disconneted");
                        PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: %s\n", command);
                    }

                    if (ind_data.backhaul_v6_status_valid == TRUE) {
                        memset(command, 0, MAX_COMMAND_STR_LEN);
                        snprintf(command, MAX_COMMAND_STR_LEN, "IPV6 %s Backhaul %s...\n", buffer, (ind_data.backhaul_v6_status) ? "connected" : "disconneted");
                        PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: %s\n", command);
                    }

                    if (ind_data.backhaul_eth_status_valid == TRUE) {
                        memset(command, 0, MAX_COMMAND_STR_LEN);
                        snprintf(command, MAX_COMMAND_STR_LEN, "ETH %s Backhaul %s...\n", buffer, (ind_data.backhaul_eth_status) ? "connected" : "disconneted");
                        PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind: %s\n", command);
                    }
                }
            }
            break;
        }
        case QMI_QCMAP_MSGR_WWAN_ROAMING_STATUS_IND_V01: {
            qcmap_msgr_wwan_roaming_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(qcmap_msgr_wwan_roaming_status_ind_msg_v01));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind Roaming status changed to %d\n", ind_data.wwan_roaming_status);
            PRINT_DEBUG("qcmap_msgr_qmi_qcmap_ind Roaming status changed to %d\n", ind_data.wwan_roaming_status);
            return;
        }

        case QMI_QCMAP_MSGR_V2X_SPS_FLOW_REG_RESULT_IND_V01: {
            qcmap_msgr_v2x_sps_flow_reg_result_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("   Received V2X_SPS_FLOW_REG_RESULT_IND \n");
            SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_data.reg_result.flow_id.req_id);
            SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_data.reg_result.flow_id.sps_id);
            SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_data.reg_result.result);
        } break;

        case QMI_QCMAP_MSGR_V2X_SPS_FLOW_DEREG_RESULT_IND_V01: {
            qcmap_msgr_v2x_sps_flow_dereg_result_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof((ind_data)));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("   Received V2X_SPS_FLOW_DEREG_RESULT_IND \n");
            SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_data.dereg_result.flow_id.req_id);
            SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_data.dereg_result.flow_id.sps_id);
            SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_data.dereg_result.result);
        } break;

        case QMI_QCMAP_MSGR_V2X_SPS_FLOW_UPDATE_RESULT_IND_V01: {
            qcmap_msgr_v2x_sps_flow_update_result_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("   Received V2X_SPS_FLOW_UPDATE_RESULT_IND \n");
            SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_data.update_result.flow_id.req_id);
            SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_data.update_result.flow_id.sps_id);
            SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_data.update_result.result);
        } break;

        case QMI_QCMAP_MSGR_V2X_SERVICE_SUBSCRIBE_RESULT_IND_V01: {
            qcmap_msgr_v2x_service_subscribe_result_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("   Received V2X_SERVICE_SUBSCRIBE_RESULT_IND \n");
            SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_data.req_id);
            SHOW_OPTIONAL_RESPONSE_TO_USER(Result of Wildcard Subscription, ind_data.subscribe_wildcard_result);
            SHOW_OPTIONAL_RESPONSE_LIST_TO_USER(Service Subscription Result List, ind_data.reg_result);
        } break;

        case QMI_QCMAP_MSGR_V2X_SEND_CONFIG_FILE_RESULT_IND_V01: {
            qcmap_msgr_v2x_send_config_file_result_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("   Received V2X_SEND_CONFIG_FILE_RESULT_IND \n");
            SHOW_MANDATORY_RESPONSE_TO_USER(Config File Result, ind_data.result);

        } break;

        case QMI_QCMAP_MSGR_V2X_SPS_SCHEDULING_INFO_IND_V01: {
            qcmap_msgr_v2x_sps_scheduling_info_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("   Received V2X_SPS_SCHEDULING_INFO_IND \n");
            SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_data.info.sps_id);
            SHOW_MANDATORY_RESPONSE_TO_USER(Absolute UTC start(nanoSecs), ind_data.info.utc_time);
            SHOW_MANDATORY_RESPONSE_TO_USER(Periodicity of the grant(milliSecs), ind_data.info.periodicity);
        } break;

        case QMI_QCMAP_MSGR_V2X_SRC_L2_INFO_IND_V01: {
            qcmap_msgr_v2x_src_l2_info_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("   Received V2X_SRC_L2_INFO_IND \n");
            SHOW_MANDATORY_RESPONSE_TO_USER(Source L2 ID, ind_data.src_l2_id);
        } break;

        case QMI_QCMAP_MSGR_V2X_CAPABILITY_INFO_IND_V01: {
            qcmap_msgr_v2x_capability_info_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("   Received V2X_CAPABILITY_INFO_IND \n");
            SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of SPS Flows, ind_data.max_sps_flow_cnt);
            SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of Event Driven Flows, ind_data.max_event_driven_flow_cnt);
            SHOW_OPTIONAL_RESPONSE_TO_USER(WWAN Concurrency Capability, ind_data.is_concurrency_supported);
            SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of SPS Flows, ind_data.pppp_info);    //here
            if (ind_data.pppp_info_valid) {
                PRINT_DEBUG("   Promixmity Service per Packet Priority Information Length=%d\n", ind_data.pppp_info_len);
                if (ind_data.pppp_info_len > 0) {
                    for (int i = 0; i < ind_data.pppp_info_len; i++) {
                        PRINT_DEBUG("      Priority[%d]=%d\n", i, ind_data.pppp_info[i].priority);
                        PRINT_DEBUG("      PDB[%d]=%d\n", i, ind_data.pppp_info[i].pdb);
                    }
                }
            }
            SHOW_OPTIONAL_RESPONSE_TO_USER(Supported Minimum Transmission Power, ind_data.min_tx_pwr);
            SHOW_OPTIONAL_RESPONSE_TO_USER(Supported Maximum Transmission Power, ind_data.max_tx_pwr);
            SHOW_OPTIONAL_RESPONSE_LIST_TO_USER(Supported SPS Periodicity List, ind_data.supported_periodicity_list);
            if (ind_data.tx_pool_id_list_valid) {
                PRINT_DEBUG("   Transmission Pool ID List Length=%d\n", ind_data.tx_pool_id_list_len);
                if (ind_data.tx_pool_id_list_len > 0) {
                    for (int i = 0; i < ind_data.tx_pool_id_list_len; i++) {
                        PRINT_DEBUG("      Pool ID[%d]=%d\n", i, ind_data.tx_pool_id_list[i].pool_id);
                        PRINT_DEBUG("      Min freq[%d] in MHz=%d\n", i, ind_data.tx_pool_id_list[i].min_freq);
                        PRINT_DEBUG("      Max freq[%d] in MHz=%d\n", i, ind_data.tx_pool_id_list[i].max_freq);
                    }
                }
            }
        } break;

        case QMI_QCMAP_MSGR_MODEM_STATUS_IND_V01: {
            qcmap_msgr_modem_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR)

            {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            if (ind_data.service_status == 0)
                PRINT_DEBUG("Received MODEM_STATUS_IND: UP\n");
            else
                PRINT_DEBUG("Received MODEM_STATUS_IND: DOWN\n");
        } break;

        case QCMAP_SERVER_STATUS_IND: {
            PRINT_DEBUG("QCMAP_Server status update on %s: %s\n", (((qcmap_server_status_t*)ind_buf)->server_type) ? "EAP" : "MDM", (((qcmap_server_status_t*)ind_buf)->server_status) ? "DOWN" : "UP");
        } break;

        case QMI_QCMAP_MSGR_MODEM_SERVICE_STATUS_IND_V01: {
            qcmap_msgr_modem_service_status_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));

            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            if (ind_data.subs_id_valid == TRUE) {
                PRINT_DEBUG("Subs_id for MODEM_SERVICE_STATUS_IND: %d\n", ind_data.subs_id);
            }
            if (ind_data.modem_service_status_valid == TRUE) {
                if (ind_data.modem_service_status == 1) {
                    PRINT_DEBUG("Received MODEM_SERVICE_STATUS_IND: UP\n");
                }
                if (ind_data.modem_service_status == 0) {
                    PRINT_DEBUG("Received MODEM_SERVICE_STATUS_IND: DOWN\n");
                }
            }
        } break;

        case QMI_QCMAP_MSGR_MTPE_TEST_RESULT_IND_V01: {
            PRINT_DEBUG("Modem Throughput Test result received\n");

            qcmap_msgr_mtpe_test_result_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            switch (ind_data.status) {
                case QCMAP_MTPE_TEST_SETUP_READY_V01:

                    PRINT_DEBUG("MTPE Test has successfully been configured and is ready to Start\n");

                    break;

                case QCMAP_MTPE_TEST_COMPLETE_V01:

                    PRINT_DEBUG("MTPE Test completed successfully!\n");
                    PRINT_DEBUG("Peak rate calculated: %d(kbps)\n", ind_data.peak_rate);

                    break;

                case QCMAP_MTPE_TEST_ABORTED_V01:

                    PRINT_DEBUG("MTPE Test was aborted before test duration ended\n");
                    if (ind_data.peak_rate_valid) {
                        PRINT_DEBUG("Peak rate calculated: %d(kbps)\n", ind_data.peak_rate);
                    }

                    break;

                case QCMAP_MTPE_TEST_FAILED_V01:
                    PRINT_DEBUG("MTPE Test Failed. Failure reason: 0x%x\n", ind_data.failure_reason);
                    break;

                default:
                    PRINT_DEBUG("Invalid test status received: 0x%x\n", ind_data.status);
                    break;
            }
        } break;
        case QMI_QCMAP_MSGR_GLOBAL_QOS_FLOW_IND_V01: {
            qcmap_msgr_global_qos_flow_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }
            PRINT_DEBUG("Received GLOBAL_QOS_FLOW_IND \n");

            /* Bearer ID valid */
            if (ind_data.bearer_id_valid) {
                PRINT_DEBUG("Received Bearer Id %d \n", ind_data.bearer_id);
            }
            if (ind_data.tx_5g_qci_valid) {
                PRINT_DEBUG("Received Tx 5g qci  %d \n", ind_data.tx_5g_qci);
            }
            if (ind_data.rx_5g_qci_valid) {
                PRINT_DEBUG("Received Rx 5g qci  %d \n", ind_data.rx_5g_qci);
            }
        } break;

        case QMI_QCMAP_MSGR_DDS_RECOMMENDATION_IND_V01: {
            qcmap_msgr_dds_recommendation_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("Received DDS_RECOMMENDATION_IND with recommended dds subid: %d\n", ind_data.recommended_dds);
        } break;

        case QMI_QCMAP_MSGR_SWITCH_DDS_IND_V01: {
            qcmap_msgr_switch_dds_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("Received SWITCH_DDS_IND\n");

            if (ind_data.dds_switch_result == QCMAP_MSGR_DDS_SWITCH_ALLOWED_V01) {
                PRINT_DEBUG("DDS Switch is allowed\n");
            } else if (ind_data.dds_switch_result == QCMAP_MSGR_DDS_SWITCH_NOT_ALLOWED_V01) {
                PRINT_DEBUG("DDS Switch is not allowed\n");
            } else if (ind_data.dds_switch_result == QCMAP_MSGR_DDS_SWITCH_FAILED_V01) {
                PRINT_DEBUG("DDS Switch Failed\n");
            }
        } break;

        case QMI_QCMAP_MSGR_CURRENT_DDS_IND_V01: {
            qcmap_msgr_current_dds_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            PRINT_DEBUG("Received CURRENT_DDS_IND with current dds: %d\n", ind_data.dds);
        } break;

        case QMI_QCMAP_MSGR_MTPE_HISTORY_IND_V01: {
            PRINT_DEBUG("Recieved an MTPE History Indication...\n");

            qcmap_msgr_mtpe_history_ind_msg_v01 ind_data;

            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi_client_message_decode error %d\n",
                    qmi_error);
                break;
            }

            if (ind_data.transaction_id_valid) {
                PRINT_DEBUG("Indication received with transaction ID : %d\n", ind_data.transaction_id);
            } else {
                PRINT_DEBUG("Indication received with invalid transaction ID\n");
                break;
            }

            pthread_mutex_lock(&mtpe_hist_mutex);

            if (!ProcessMTPEHistoryIndication(&ind_data, mtpe_history_entries, &len_mtpe_history, mtpe_txn_id, &mtpe_history_ready_flag)) {
                PRINT_DEBUG("Failed to process an MTPE History Indication\n");
            } else {
                PRINT_DEBUG("Processed an MTPE History Indication...\n");

                // All indications from the QCMAP Server received by the client
                if (mtpe_history_ready_flag) {
                    PRINT_DEBUG("Successfully retrieved requested MTPE history items:\n");
                    for (int i = 0; i < len_mtpe_history; i++) {
                        print_mtpe_entry((mtpe_history_entry*)((char*)mtpe_history_entries + (sizeof(mtpe_history_entry) * i)));
                    }
                }
            }

            pthread_mutex_unlock(&mtpe_hist_mutex);

        } break;

        case QMI_QCMAP_MSGR_THROUGHPUT_STATS_IND_V01: {
            qcmap_msgr_throughput_stats_ind_msg_v01 ind_data;
            qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION, msg_id, ind_buf, ind_buf_len, &ind_data, sizeof(ind_data));
            if (qmi_error != QMI_NO_ERR) {
                PRINT_DEBUG(
                    "qcmap_msgr_qmi_qcmap_ind: "
                    "qmi throughput indication error %d\n",
                    qmi_error);
                break;
            }
            PRINT_DEBUG("The thermal mitigation Indication Received: %d\n", ind_data.throughput, 0, 0);
            PRINT_DEBUG("The thermal mitigation Indication Received: %d\n", ind_data.throughput);

            break;
        }

        default:
            break;
    }

    return;
}

// Process and aggregate the data from incoming MTPE indication(s)
boolean ProcessMTPEHistoryIndication(qcmap_msgr_mtpe_history_ind_msg_v01* ind_data, mtpe_history_entry* in_mtpe_history_entries, uint32_t* len_mtpe_history, uint32 txn_id, boolean* mtpe_history_ready) {
    qmi_error_type_v01* qmi_err_num;
    PRINT_DEBUG("TXN_ID %d (%d)", ind_data->transaction_id, txn_id, 0);
    PRINT_DEBUG("TXN_valid %d, list_valid %d, pending %d\n", ind_data->transaction_id_valid, ind_data->mtpe_history_list_valid, ind_data->num_inds_pending_valid);
    // check if history items are related to the same request
    if (ind_data->transaction_id_valid && (ind_data->transaction_id == txn_id) && ind_data->mtpe_history_list_valid) {
        memcpy(((char*)(in_mtpe_history_entries) + (sizeof(qcmap_msgr_mtpe_history_entry_msg_v01) * (*len_mtpe_history))), ind_data->mtpe_history_list, (ind_data->mtpe_history_list_len * sizeof(qcmap_msgr_mtpe_history_entry_msg_v01)));
        *len_mtpe_history = *len_mtpe_history + ind_data->mtpe_history_list_len;
    } else {
        PRINT_DEBUG("MTPE history items invalid or TXN ID invalid\n", 0, 0, 0);
        *mtpe_history_ready = true;
        return false;
    }
    if (ind_data->num_inds_pending_valid && (ind_data->num_inds_pending == 0)) {
        PRINT_DEBUG("MTPE READY", 0, 0, 0);
        *mtpe_history_ready = true;
    } else {
        *mtpe_history_ready = false;
    }
    return true;
}

boolean convert_backhaul_enum_to_string(qcmap_msgr_backhaul_type_enum_v01 backhaul_type, char* backhaul_type_string) {
    if (backhaul_type_string == NULL) {
        PRINT_DEBUG("NULL Args", 0, 0, 0);
        return false;
    }
    switch (backhaul_type) {
        case QCMAP_MSGR_WWAN_BACKHAUL_V01:
            snprintf(backhaul_type_string, MAX_BACKHAUL_TYPE_LENGTH, "WWAN");
            break;
        case QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01:
            snprintf(backhaul_type_string, MAX_BACKHAUL_TYPE_LENGTH, "USB CRADLE");
            break;
        case QCMAP_MSGR_WLAN_BACKHAUL_V01:
            snprintf(backhaul_type_string, MAX_BACKHAUL_TYPE_LENGTH, "WLAN");
            break;
        case QCMAP_MSGR_ETHERNET_BACKHAUL_V01:
            snprintf(backhaul_type_string, MAX_BACKHAUL_TYPE_LENGTH, "ETHERNET");
            break;
        case QCMAP_MSGR_BT_BACKHAUL_V01:
            snprintf(backhaul_type_string, MAX_BACKHAUL_TYPE_LENGTH, "BT");
            break;
        default:
            PRINT_DEBUG("Invalid Backhaul type\n", 0, 0, 0);
            return false;
            break;
    }
    return true;
}

void DisplayClientInfo(qcmap_msgr_packet_stats_status_ind_msg_v01 ind_data) {
    char tmpIPv4[INET_ADDRSTRLEN];
    in6_addr allipv6[QCMAP_MSGR_MAX_IPV6_ADDR_V01];
    in6_addr tmpipv6;
    uint8 zero_buff[QCMAP_MSGR_IPV6_ADDR_LEN_V01] = {0};
    char ip6_addr_buf[INET6_ADDRSTRLEN];
    uint32_t connDevCount = 0;
    char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01]; /*char array of mac address*/
    memset(tmpIPv4, 0, INET_ADDRSTRLEN);
    uint32_t entries = ind_data.number_of_entries;

    PRINT_DEBUG("\n CLI: ind_type %d, conn_client_num  %d , entry %d\n", ind_data.conn_status, entries);
    if (entries != 0) {
        // Displaying the information in appropriate fashion
        for (connDevCount = 0; connDevCount < entries; connDevCount++) {
            if (connDevCount == QCMAP_MSGR_MAX_CONNECTED_DEVICES_V01) {
                break;
            }
            ds_mac_addr_ntop(ind_data.info[connDevCount].client_mac_addr, mac_addr_str);
            PRINT_DEBUG("MAC Address : %s \n", mac_addr_str);
            if (inet_ntop(AF_INET, (void*)&ind_data.info[connDevCount].ipv4_addr, tmpIPv4, INET_ADDRSTRLEN)) {
                PRINT_DEBUG("IPv4 Address : %s \n", tmpIPv4);
            }
            memset(&tmpipv6, 0, sizeof(tmpipv6));
            memcpy(&tmpipv6.s6_addr, ind_data.info[connDevCount].ll_ipv6_addr, QCMAP_MSGR_IPV6_ADDR_LEN_V01);
            if (inet_ntop(AF_INET6, (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf))) {
                PRINT_DEBUG("Link Local IPv6 Address : %s\n", ip6_addr_buf);
            }

            for (int i = 0; i < QCMAP_MSGR_MAX_IPV6_ADDR_V01; i++) {
                memset(&allipv6[i], 0, sizeof(in6_addr));
                memcpy(&allipv6[i].s6_addr, ind_data.info[connDevCount].ipv6[i].addr, QCMAP_MSGR_IPV6_ADDR_LEN_V01);
                if (!memcmp(&allipv6[i].s6_addr, zero_buff, QCMAP_MSGR_IPV6_ADDR_LEN_V01))
                    break;
                if (inet_ntop(AF_INET6, (void*)&allipv6[i], ip6_addr_buf, sizeof(ip6_addr_buf))) {
                    PRINT_DEBUG("IPv6 Address %d: %s\n", i, ip6_addr_buf);
                }
            }
            switch (ind_data.info[connDevCount].device_type) {
                case QCMAP_MSGR_DEVICE_TYPE_PRIMARY_AP_V01:
                    PRINT_DEBUG("Device Type : Primary AP\n");
                    break;
                case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_V01:
                    PRINT_DEBUG("Device Type :Guest AP1\n");
                    break;
                case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_2_V01:
                    PRINT_DEBUG("Device Type :Guest AP2\n");
                    break;
                case QCMAP_MSGR_DEVICE_TYPE_USB_V01:
                    PRINT_DEBUG("Device Type :USB\n");
                    break;
                case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01:
                    PRINT_DEBUG("Device Type :Ethernet\n");
                    break;
                case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_NIC2_V01:
                    PRINT_DEBUG("Device Type :Ethernet-NIC2\n");
                    break;
                default:
                    PRINT_DEBUG("Device Type : Invalid\n");
                    break;
            }
            PRINT_DEBUG("Host Name : %s\n", ind_data.info[connDevCount].host_name);
            PRINT_DEBUG("rx bytes : %lu\n", ind_data.info[connDevCount].bytes_rx);
            PRINT_DEBUG("tx bytes : %lu\n", ind_data.info[connDevCount].bytes_tx);
            PRINT_DEBUG("Lease Expiry Time (in minutes) : %d\n\n\n", ind_data.info[connDevCount].lease_expiry_time);
        }
    } else {
        PRINT_DEBUG("\n CLI:No Connected Device to this Access Point \n");
    }
}

void qcmap_send_cwmp_notify(void) {
    int cwmp_msgid;
    struct cwmp_message cwmpmsg;

    if ((cwmp_msgid = msgget((key_t)1234, 0)) >= 0)
    {
        cwmpmsg.msg_type = MSG_ACTIVE_NOTIFY;
        msgsnd(cwmp_msgid, (void *)&cwmpmsg, MSG_SIZE, 0);
        PRINT_DEBUG("send cwmp notify\n");
    }
}