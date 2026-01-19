#include <linux/route.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "mobile_module_util.h"
#include "mobile_service_modem.h"

/*
 * 文件名称：mobile_module_util.c
 * 功能描述：
 *     移动管理工具模块，提供UCI配置管理和通用链表操作功能
 *
 * 作者：gaoweiming
 */

/**
 * 获取UCI配置项的值
 * @param key 配置项键名
 * @param value 存储配置值的缓冲区
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_get(char* key, char* value) {
    int ret = -1;

    if (key == NULL || value == NULL) {
        MOBILE_ERROR("Invalid parameters: key=%p, value=%p\n", key, value);
        return -1;
    }

    ret = suci_get(key, value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to get UCI config: key=%s, ret=%d\n", key, ret);
    }

    return ret;
}

/**
 * 获取UCI配置项的整数值
 * @param key 配置项键名
 * @param value 要获取的整数值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_get_int(char* key, int* value) {
    char str_value[128] = {0};
    int ret = -1;

    if (key == NULL) {
        MOBILE_ERROR("Invalid parameter: key is NULL\n");
        return -1;
    }

    if (value == NULL) {
        MOBILE_ERROR("Invalid parameter: value is NULL\n");
        return -1;
    }

    ret = suci_get(key, str_value);
    if (0 != ret) {
        MOBILE_DEBUG("Failed to get UCI int config: key=%s, ret=%d\n", key, ret);
        return -1;
    } else {
        char* endptr;
        long result = strtol(str_value, &endptr, 10);
        if (endptr == str_value || *endptr != '\0') {
            MOBILE_ERROR("Invalid integer value: key=%s, value=%s\n", key, str_value);
            return -1;
        }
        if (result < INT_MIN || result > INT_MAX) {
            MOBILE_ERROR("Integer value out of range: key=%s, value=%ld\n", key, result);
            return -1;
        }
        *value = (int)result;
        return 0;
    }
}

/**
 * 获取UCI配置项的无符号整数值
 * @param key 配置项键名
 * @param value 要获取的无符号整数值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_get_uint(char* key, unsigned int* value) {
    char str_value[128] = {0};
    int ret = -1;

    if (key == NULL) {
        MOBILE_ERROR("Invalid parameter: key is NULL\n");
        return -1;
    }

    if (value == NULL) {
        MOBILE_ERROR("Invalid parameter: value is NULL\n");
        return -1;
    }

    ret = suci_get(key, str_value);
    if (0 != ret) {
        MOBILE_DEBUG("Failed to get UCI uint config: key=%s, ret=%d\n", key, ret);
        return -1;
    } else {
        char* endptr;
        unsigned long result = strtoul(str_value, &endptr, 10);
        if (endptr == str_value || *endptr != '\0') {
            MOBILE_ERROR("Invalid unsigned integer value: key=%s, value=%s\n", key, str_value);
            return -1;
        }
        if (result > UINT_MAX) {
            MOBILE_ERROR("Unsigned integer value out of range: key=%s, value=%lu\n", key, result);
            return -1;
        }
        *value = (unsigned int)result;
        return 0;
    }
}


/**
 * 获取UCI配置选项的值
 * @param package_name 包名
 * @param section_name 节名
 * @param option_name 选项名
 * @param option_value 存储选项值的缓冲区
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_get_option(char* package_name, char* section_name, char* option_name, char* option_value) {
    char tmp_str[128] = {0};  // 增加缓冲区大小
    int ret;

    if (package_name == NULL || section_name == NULL || option_value == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section=%p, option=%p, value=%p\n",
                    package_name, section_name, option_value);
        return -1;
    }

    if (option_name == NULL)
        ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s", package_name, section_name);
    else
        ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s.%s", package_name, section_name, option_name);
    if (ret < 0 || ret >= (int)sizeof(tmp_str)) {
        MOBILE_ERROR("Buffer overflow when building UCI option string\n");
        return -1;
    }

    ret = suci_get(tmp_str, option_value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to get UCI option: %s, ret=%d\n", tmp_str, ret);
    }

    return ret;
}

/**
 * 获取UCI配置选项的值
 * @param package_name 包名
 * @param section_name 节名
 * @param option_name 选项名
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_get_option_int(char* package_name, char* section_name, char* option_name, int* value) {
    char tmp_str[128] = {0};  // 增加缓冲区大小
    char str_value[128] = {0};
    int ret = -1;

    if (package_name == NULL || section_name == NULL || option_name == NULL || value == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section=%p, option=%p, value=%p\n",
                    package_name, section_name, option_name, value);
        return -1;
    }

    ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s.%s", package_name, section_name, option_name);
    if (ret < 0 || ret >= (int)sizeof(tmp_str)) {
        MOBILE_ERROR("Buffer overflow when building UCI option string\n");
        return -1;
    }

    ret = suci_get(tmp_str, str_value);
    if (0 != ret) {
        MOBILE_DEBUG("Failed to get UCI option int: %s, ret=%d\n", tmp_str, ret);
        return -1;
    } else {
        char* endptr;
        long result = strtol(str_value, &endptr, 10);
        if (endptr == str_value || *endptr != '\0') {
            MOBILE_ERROR("Invalid integer value: option=%s, value=%s\n", tmp_str, str_value);
            return -1;
        }
        if (result < INT_MIN || result > INT_MAX) {
            MOBILE_ERROR("Integer value out of range: option=%s, value=%ld\n", tmp_str, result);
            return -1;
        }
        *value = (int)result;
        return 0;
    }
}

/**
 * 设置UCI配置项的值
 * @param key 配置项键名
 * @param value 要设置的配置值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_set(char* key, char* value) {
    int ret = -1;

    if (key == NULL || value == NULL) {
        MOBILE_ERROR("Invalid parameters: key=%p, value=%p\n", key, value);
        return -1;
    }

    ret = suci_set(key, value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to set UCI config: key=%s, value=%s, ret=%d\n", key, value, ret);
    }

    return ret;
}

/**
 * 设置UCI配置项的整数值
 * @param key 配置项键名
 * @param value 要设置的整数值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_set_int(char* key, int value) {
    int ret = -1;
    char str_value[64] = {0};

    if (key == NULL) {
        MOBILE_ERROR("Invalid parameter: key is NULL\n");
        return -1;
    }

    if (snprintf(str_value, sizeof(str_value), "%d", value) >= (int)sizeof(str_value)) {
        MOBILE_ERROR("Value too large for buffer: value=%d\n", value);
        return -1;
    }

    ret = suci_set(key, str_value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to set UCI int config: key=%s, value=%d, ret=%d\n", key, value, ret);
    }

    return ret;
}

/**
 * 设置UCI配置项的无符号整数值
 * @param key 配置项键名
 * @param value 要设置的无符号整数值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_set_uint(char* key, unsigned int value) {
    int ret = -1;
    char str_value[64] = {0};

    if (key == NULL) {
        MOBILE_ERROR("Invalid parameter: key is NULL\n");
        return -1;
    }

    if (snprintf(str_value, sizeof(str_value), "%u", value) >= (int)sizeof(str_value)) {
        MOBILE_ERROR("Value too large for buffer: value=%u\n", value);
        return -1;
    }

    ret = suci_set(key, str_value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to set UCI uint config: key=%s, value=%u, ret=%d\n", key, value, ret);
    }

    return ret;
}


/**
 * 设置UCI配置选项的值
 * @param package_name 包名
 * @param section_name 节名
 * @param option_name 选项名（可为NULL）
 * @param option_value 选项值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_set_option(char* package_name, char* section_name, char* option_name, char* option_value) {
    char tmp_str[128] = {0};  // 增加缓冲区大小
    int ret;

    if (package_name == NULL || section_name == NULL || option_value == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section=%p, value=%p\n",
                    package_name, section_name, option_value);
        return -1;
    }

    if (option_name == NULL) {
        ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s", package_name, section_name);
    } else {
        ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s.%s", package_name, section_name, option_name);
    }

    if (ret < 0 || ret >= (int)sizeof(tmp_str)) {
        MOBILE_ERROR("Buffer overflow when building UCI option string\n");
        return -1;
    }

    ret = suci_set(tmp_str, option_value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to set UCI option: %s=%s, ret=%d\n", tmp_str, option_value, ret);
    }

    return ret;
}

/**
 * 设置UCI配置选项的整数值
 * @param package_name 包名
 * @param section_name 节名
 * @param option_name 选项名
 * @param option_value 整数值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_set_option_int(char* package_name, char* section_name, char* option_name, int option_value) {
    char tmp_str[128] = {0};  // 增加缓冲区大小
    char str_value[64] = {0};
    int ret;

    if (package_name == NULL || section_name == NULL || option_name == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section=%p, option=%p\n",
                    package_name, section_name, option_name);
        return -1;
    }

    ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s.%s", package_name, section_name, option_name);
    if (ret < 0 || ret >= (int)sizeof(tmp_str)) {
        MOBILE_ERROR("Buffer overflow when building UCI option string\n");
        return -1;
    }

    ret = snprintf(str_value, sizeof(str_value), "%d", option_value);
    if (ret < 0 || ret >= (int)sizeof(str_value)) {
        MOBILE_ERROR("Buffer overflow when converting integer value: %d\n", option_value);
        return -1;
    }

    ret = suci_set(tmp_str, str_value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to set UCI option int: %s=%s, ret=%d\n", tmp_str, str_value, ret);
    }

    return ret;
}

/**
 * 删除UCI配置选项
 * @param package_name 包名
 * @param section_name 节名
 * @param option_value 选项值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_del_option(char* package_name, char* section_name, char* option_value) {
    char tmp_str[128] = {0};  // 增加缓冲区大小
    int ret;

    if (package_name == NULL || section_name == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section=%p\n", package_name, section_name);
        return -1;
    }

    ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s", package_name, section_name);
    if (ret < 0 || ret >= (int)sizeof(tmp_str)) {
        MOBILE_ERROR("Buffer overflow when building UCI option string\n");
        return -1;
    }

    ret = suci_delete(tmp_str, option_value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to delete UCI option: %s, value=%s, ret=%d\n", tmp_str, option_value ? option_value : "NULL", ret);
    }

    return ret;
}

/**
 * 删除UCI配置项
 * @param key 配置项键名
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_del(char* key) {
    int ret;

    if (key == NULL) {
        MOBILE_ERROR("Invalid parameter: key is NULL\n");
        return -1;
    }

    ret = suci_delete(key, NULL);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to delete UCI key: %s, ret=%d\n", key, ret);
    }

    return ret;
}

/**
 * 添加UCI配置节
 * @param package_name 包名
 * @param section_name 节名
 * @param option_name 选项名
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_add(char* package_name, char* section_name, char* option_name) {
    int ret;

    if (package_name == NULL || section_name == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section=%p\n", package_name, section_name);
        return -1;
    }

    ret = suci_add(package_name, section_name, option_name);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to add UCI section: package=%s, section=%s, option=%s, ret=%d\n",
                    package_name, section_name, option_name ? option_name : "NULL", ret);
    }

    return ret;
}

/**
 * 向UCI配置列表添加值
 * @param package_name 包名
 * @param section_name 节名
 * @param option_name 选项名
 * @param option_value 要添加的列表值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_add_list(char* package_name, char* section_name, char* option_name, char* option_value) {
    char tmp_str[128] = {0};
    int ret;

    if (package_name == NULL || section_name == NULL || option_name == NULL || option_value == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section=%p, option=%p, value=%p\n",
                    package_name, section_name, option_name, option_value);
        return -1;
    }

    ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s.%s", package_name, section_name, option_name);
    if (ret < 0 || ret >= (int)sizeof(tmp_str)) {
        MOBILE_ERROR("Buffer overflow when building UCI option string\n");
        return -1;
    }

    ret = suci_add_list(tmp_str, option_value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to add UCI list: %s=%s, ret=%d\n", tmp_str, option_value, ret);
    }

    return ret;
}

/**
 * 向UCI配置列表删除值
 * @param package_name 包名
 * @param section_name 节名
 * @param option_name 选项名
 * @param option_value 要添加的列表值
 * @return 成功返回0，失败返回-1
 */
int mobile_uci_del_list(char* package_name, char* section_name, char* option_name, char* option_value) {
    int ret;
    char tmp_str[128] = {0};

    if (package_name == NULL || section_name == NULL || option_name == NULL || option_value == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section=%p, option=%p, value=%p\n",
                    package_name, section_name, option_name, option_value);
        return -1;
    }
    ret = snprintf(tmp_str, sizeof(tmp_str), "%s.%s.%s", package_name, section_name, option_name);
    if (ret < 0 || ret >= (int)sizeof(tmp_str)) {
        MOBILE_ERROR("Buffer overflow when building UCI option string\n");
        return -1;
    }

    ret = suci_del_list(tmp_str, option_value);
    if (ret != 0) {
        MOBILE_DEBUG("Failed to del UCI list: %s=%s, ret=%d\n", tmp_str, option_value, ret);
    }

    return ret;
}

/**
 * 获取UCI包中指定类型节的数量
 * @param package 包名
 * @param section_type 节类型
 * @return 节的数量，失败返回0
 */
int mobile_uci_get_section_number(char* package, char* section_type) {
    FILE* fp = NULL;
    char cmd[256] = {0};  // 增加缓冲区大小
    char buf[16] = {0};   // 增加缓冲区大小
    int ret = 0;

    if (package == NULL || section_type == NULL) {
        MOBILE_ERROR("Invalid parameters: package=%p, section_type=%p\n", package, section_type);
        return 0;
    }

    if (snprintf(cmd, sizeof(cmd), "uci show %s|grep =%s|wc -l", package, section_type) >= (int)sizeof(cmd)) {
        MOBILE_ERROR("Command buffer overflow: package=%s, section_type=%s\n", package, section_type);
        return 0;
    }

    fp = popen(cmd, "r");
    if (!fp) {
        MOBILE_ERROR("Failed to execute command: %s\n", cmd);
        return 0;
    }

    if (fgets(buf, sizeof(buf), fp) != NULL) {
        char* endptr;
        long result = strtol(buf, &endptr, 10);
        if (endptr == buf || *endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
            MOBILE_ERROR("Invalid section count: %s\n", buf);
            ret = 0;
        } else if (result < 0 || result > INT_MAX) {
            MOBILE_ERROR("Section count out of range: %ld\n", result);
            ret = 0;
        } else {
            ret = (int)result;
        }
    }

    if (pclose(fp) == -1) {
        MOBILE_ERROR("Failed to close pipe for command: %s\n", cmd);
    }

    return ret;
}

/**
 * 释放整个通用链表
 * @param listhead 链表头指针的地址
 */
void mobile_free_whole_comm_list(void** listhead) {
    if (listhead && *listhead) {
        comm_list_head_t* p_curr = ((comm_list_head_t*)(*listhead));
        comm_list_head_t* p_next = p_curr->next;

        //reset listhead first
        *listhead = NULL;
        while (p_curr) {
            free(p_curr);
            p_curr = p_next;
            if (p_next) {
                p_next = p_next->next;
            }
        }
    }
}

/**
 * 在通用链表中分配内存
 * @param listhead 链表头指针的地址
 * @param size 要分配的内存大小
 * @return 成功返回分配的内存指针，失败返回NULL
 */
void* mobile_malloc_in_comm_list(void** listhead, int size) {
    if (listhead && size > sizeof(void*)) {
        comm_list_head_t* p_curr = (comm_list_head_t*)malloc(size);
        if (p_curr) {
            memset(p_curr, 0x0, size);

            if (*listhead) {
                //insert to head
                p_curr->next = (comm_list_head_t*)(*listhead);
                *listhead = p_curr;
            } else {
                *listhead = p_curr;
            }
        }

        return p_curr;
    }

    return NULL;
}

/**
 * 在通用链表尾部分配内存
 * @param listhead 链表头指针的地址
 * @param size 要分配的内存大小
 * @return 成功返回分配的内存指针，失败返回NULL
 */
void* mobile_malloc_in_comm_list_tail(void **listhead, int size) {
    if (listhead && size > sizeof(void*)) {
        comm_list_head_t* p_curr = (comm_list_head_t*)malloc(size);
        if (p_curr) {
            memset(p_curr, 0x0, size);
            p_curr->next = NULL;

            if (*listhead) {
                // 找到链表尾部
                comm_list_head_t* p_tail = (comm_list_head_t*)(*listhead);
                while (p_tail->next) {
                    p_tail = p_tail->next;
                }
                // 插入到尾部
                p_tail->next = p_curr;
            } else {
                *listhead = p_curr;
            }
        }

        return p_curr;
    }

    return NULL;
}

/**
 * 将缓冲区内容写入文件
 * 用于将数据缓冲区写入指定文件，支持创建新文件或覆盖现有文件
 *
 * @param filename 目标文件名，不能为空
 * @param buf 要写入的数据缓冲区，不能为空
 * @param len 要写入的数据长度，必须大于0
 * @return 成功返回1，失败返回0
 */
int mobile_write_buff_to_file(const char* filename, const char* buf, int len) {
    FILE* in = NULL;
    int ret = 0;
    size_t written = 0;

    if (filename == NULL || buf == NULL) {
        MOBILE_ERROR("Invalid parameters: filename=%p, buf=%p\n", filename, buf);
        return 0;
    }

    if (len <= 0) {
        MOBILE_ERROR("Invalid buffer length: %d (filename:%s)\n", len, filename);
        return 0;
    }

    in = fopen(filename, "w+");
    if (in == NULL) {
        MOBILE_ERROR("Failed to open file for writing: %s, error: %s\n", filename, strerror(errno));
        return 0;
    }

    written = fwrite(buf, 1, len, in);
    if (written != (size_t)len) {
        MOBILE_ERROR("Failed to write complete data to file: %s, written=%zu, expected=%d\n",
                    filename, written, len);
        fclose(in);
        return 0;
    }

    if (fclose(in) != 0) {
        MOBILE_ERROR("Failed to close file: %s, error: %s\n", filename, strerror(errno));
        return 0;
    }

    ret = 1;
    return ret;
}

/**
 * 从指定文件中读取第一行到数据缓冲区
 *
 * @param filename 目标文件名，不能为空
 * @param line 要写入的数据缓冲区，不能为空
 * @param size 缓冲区大小，必须大于0
 * @return 成功返回0，失败返回-1
 */
int mobile_read_first_line_from_file(char* filename, char* line, int size) {
    FILE* fp = NULL;

    if (filename == NULL || line == NULL) {
        MOBILE_ERROR("Invalid parameters: filename=%p, line=%p\n", filename, line);
        return -1;
    }

    if (size <= 0) {
        MOBILE_ERROR("Invalid buffer size: %d\n", size);
        return -1;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        MOBILE_ERROR("Failed to open file for reading: %s, error: %s\n", filename, strerror(errno));
        return -1;
    }

    memset(line, 0, size);

    if (fgets(line, size, fp)) {
        /* Terminate CR/LF */
        int iLen = strlen(line);
        if (iLen > 0) {
            if (line[iLen - 1] == '\n' || line[iLen - 1] == '\r') {
                line[iLen - 1] = '\0';
                // 检查是否还有回车符
                iLen = strlen(line);
                if (iLen > 0 && (line[iLen - 1] == '\n' || line[iLen - 1] == '\r')) {
                    line[iLen - 1] = '\0';
                }
            }
        }
    } else {
        if (ferror(fp)) {
            MOBILE_ERROR("Error reading from file: %s\n", filename);
        }
        // 文件为空是正常情况，不记录错误
    }

    if (fclose(fp) != 0) {
        MOBILE_ERROR("Failed to close file: %s, error: %s\n", filename, strerror(errno));
        return -1;
    }

    return 0;
}

/**
 * @brief 执行系统命令并等待子进程返回
 * @param command 要执行的命令字符串
 * @param print_flag 是否打印命令执行信息 (1:打印, 0:不打印)
 * @return 成功返回子进程退出状态，失败返回-1
 */
int mobile_system_ex(char* command, int print_flag) {
    int pid = 0, status = 0;

    if (!command) {
        MOBILE_ERROR("system_ex: Null Command, Error!");
        return -1;
    }

    MOBILE_DEBUG("command:%s\n", command);

    pid = fork();
    if (pid == -1) {
        MOBILE_ERROR("system_ex fork fail");
        return -1;
    }

    if (pid == 0) {
        char* argv[4];
        sigset_t sigset;
        int sig = 0;

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
        if (print_flag) {
            MOBILE_INFO("[system]: %s\r\n", command);
        }
        execv("/bin/sh", argv);
        exit(127);
    }

    /* wait for child process return */
    do {
        if (waitpid(pid, &status, 0) == -1) {
            if (errno != EINTR && errno != ECHILD) {
                MOBILE_ERROR("system_ex waitpid fail, errno->%s", strerror(errno));
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

/**
 * @brief 执行AT脚本并获取返回结果
 * @param script 要执行的脚本命令
 * @param buf 存储返回结果的缓冲区
 * @param len 缓冲区长度
 * @return 成功返回读取的字节数，失败返回-1
 */
int mobile_at_script_and_get_ret(char* script, char* buf, int len) {
    FILE* fp = NULL;
    int ret = -1;
    char cmd[1024 + 1] = {0};
    size_t cmd_len;

    if (script == NULL) {
        MOBILE_ERROR("Invalid parameter: script is NULL\n");
        return -1;
    }

    cmd_len = strlen(script);
    if (cmd_len == 0 || cmd_len > (1024 - strlen(" 2>/dev/null"))) {
        MOBILE_ERROR("Script name too long or empty: length=%zu\n", cmd_len);
        return -1;
    }

    if (snprintf(cmd, sizeof(cmd), "%s 2>/dev/null", script) >= (int)sizeof(cmd)) {
        MOBILE_ERROR("Command buffer overflow: script=%s\n", script);
        return -1;
    }

    fp = popen(cmd, "r");
    if (fp == NULL) {
        MOBILE_ERROR("Failed to execute command: %s, error: %s\n", cmd, strerror(errno));
        return -1;
    }

    if (len > 0 && buf != NULL) {
        ret = fread(buf, 1, len, fp);
        if (ret < 0) {
            MOBILE_ERROR("Failed to read from command output: %s\n", cmd);
            ret = -1;
        } else if (ret == len) {
            MOBILE_DEBUG("Buffer full when reading command output: %s\n", cmd);
            // 缓冲区已满，但这不是错误
        }
    } else if (len == 0 || buf == NULL) {
        // 只是执行命令，不需要读取输出
        ret = 0;
    } else {
        MOBILE_ERROR("Invalid buffer parameters: buf=%p, len=%d\n", buf, len);
        ret = -1;
    }

    pclose(fp);

    return ret;
}

/**
 * @brief 执行AT命令
 * @param cmd AT命令字符串
 * @param rbuf 存储返回结果的缓冲区（可为NULL）
 * @param len 缓冲区长度
 * @return 0 - 执行成功, -1 - 执行失败
 */
int mobile_at_cmd(const char* cmd, char* rbuf, int len) {
    char buf[1024] = {0};
    int ret;

    if (cmd == NULL) {
        MOBILE_ERROR("Invalid parameter: cmd is NULL\n");
        return -1;
    }

    if (strlen(cmd) > (sizeof(buf) - strlen("at-mngr   2>/dev/null"))) {
        MOBILE_ERROR("AT command too long: %s\n", cmd);
        return -1;
    }

    ret = snprintf(buf, sizeof(buf) - 1, "at-mngr  %s 2>/dev/null", cmd);
    if (ret < 0 || ret >= (int)sizeof(buf)) {
        MOBILE_ERROR("Command buffer overflow: cmd=%s\n", cmd);
        return -1;
    }

    if (rbuf && len > 0) {
        ret = mobile_at_script_and_get_ret(buf, rbuf, len);
        //MOBILE_DEBUG("%s %s\n", buf, rbuf ? rbuf : "NULL");
    } else {
        ret = mobile_system_ex(buf, 0);
    }
    return (ret >= 0) ? 0 : -1;
}

/**
 * @brief 检查是否为有效char
 * @param str 输入字符串
 * @return 0 - 无效, 1 - 有效
 */
int mobile_check_valid_char(char* str) {
    char validchar[] = "0123456789ABCDEFabcdef\n\r";

    for (int i = 0; i < strlen(str); i++) {
        if (strchr(validchar, str[i]) == NULL) {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief 获取系统启动时间（毫秒）
 *
 * 使用单调时钟(CLOCK_MONOTONIC)获取系统启动后的运行时间，不受系统时间调整影响。
 * 单调时钟从系统启动开始计时，不会因系统时间调整而回退或跳跃。
 *
 * @return 系统启动后的毫秒数，如果获取时间失败则返回0
 */
unsigned int mobile_get_uptime_in_ms(void) {
    struct timespec currTime = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &currTime) != 0) {
        MOBILE_ERROR("bad news, get time err!\n");
    }

    return (unsigned int)(currTime.tv_sec * 1000U + currTime.tv_nsec / 1000000);
}

/**
 * 获取网络接口的IPv4地址
 *
 * @param dev_name 网络接口名称
 * @param ip 输出参数，存储获取的IP地址
 * @return 成功返回1，失败返回0
 */
unsigned char mobile_get_if_ipv4_addr(const char* dev_name, char* ip) {
    int skfd;
    struct ifreq intf;
    char* ptr;
    unsigned char ret = 0;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return ret;
    }

    strcpy(intf.ifr_name, dev_name);

    if (ioctl(skfd, SIOCGIFADDR, &intf) != -1) {
        if ((ptr = inet_ntoa(((struct sockaddr_in*)(&(intf.ifr_addr)))->sin_addr)) != NULL) {
            strcpy(ip, ptr);
            ret = 1;
        }
    }
    close(skfd);

    return ret;
}

/**
 * 验证IPv4地址格式是否有效
 *
 * @param ip 要验证的IP地址字符串
 * @return 有效返回1，无效返回0
 */
unsigned char mobile_is_valid_ipv4_address(const char* ip) {
    int section = 0;    // 每一节的十进制值
    int dot = 0;        // 点号计数器
    int last = -1;      // 每一个字符的前一个字符
    while (*ip) {
        if (*ip == '.') {
            dot++;
            if (dot > 3) {
                return 0;
            }
            if (section >= 0 && section <= 255) {
                section = 0;
            } else {
                return 0;
            }
        } else if (*ip >= '0' && *ip <= '9') {
            section = section * 10 + *ip - '0';
            if (last == '0') {
                return 0;
            }
        } else {
            return 0;
        }
        last = *ip;
        ip++;
    }

    if (section >= 0 && section <= 255) {
        if (3 == dot) {
            section = 0;
            return 1;
        }
    }
    return 1;
}

/**
 * 从路由表中获取默认网关的接口名称
 *
 * @param if_name 输出参数，存储获取的接口名称
 * @return 成功返回1，失败返回0
 */
unsigned char mobile_get_default_gateway_if_name_in_route_table(char* if_name) {
    char col[16][32];
    char line[256];
    struct in_addr addr;
    int count = 0;
    int flag = 0;
    FILE* fs_route = fopen("/proc/net/route", "r");
    unsigned char found = 0;

    if (fs_route != NULL) {
        while (!found && fgets(line, sizeof(line), fs_route)) {
            /* 跳过标题行 */
            if (count++ < 1) {
                continue;
            }

            sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s", col[0], col[1], col[2], col[3], col[4], col[5], col[6],
                   col[7], col[8], col[9], col[10]);
            flag = strtol(col[3], (char**)NULL, 16);

            if ((flag & (RTF_UP)) == RTF_UP) {
                addr.s_addr = strtoul(col[1], (char**)NULL, 16);
                if (!strcmp("0.0.0.0", (char*)inet_ntoa(addr))) {
                    strcpy(if_name, col[0]);
                    found = 1;
                }
            }
        }

        fclose(fs_route);
    }

    return found;
}

/**
 * 获取指定网络接口的默认网关IP地址
 * 从/var/gateway目录下的接口文件中读取网关IP地址
 *
 * @param ifname 网络接口名称，不能为空
 * @param default_gw 输出参数，存储获取的默认网关IP地址
 * @param max_size 输出缓冲区最大大小
 * @return 成功返回0，失败返回-1
 */
int mobile_get_ifname_default_gw(const char* ifname, char* default_gw, int max_size) {
    char gateway_file[64] = {0};
    const char* var_gateway_dir = "/var/gateway";

    if (ifname == NULL || default_gw == NULL) {
        MOBILE_ERROR("Invalid parameters: ifname=%p, default_gw=%p\n", ifname, default_gw);
        return -1;
    }

    if (ifname[0] == '\0') {
        MOBILE_ERROR("Invalid parameter: ifname is empty\n");
        return -1;
    }

    if (max_size <= 0) {
        MOBILE_ERROR("Invalid buffer size: %d\n", max_size);
        return -1;
    }

    // 初始化输出缓冲区
    default_gw[0] = '\0';

    // 构建网关文件路径
    if (snprintf(gateway_file, sizeof(gateway_file), "%s/%s", var_gateway_dir, ifname) >= (int)sizeof(gateway_file)) {
        MOBILE_ERROR("Gateway file path too long: ifname=%s\n", ifname);
        return -1;
    }

    // 从文件中读取网关IP地址
    if (mobile_read_first_line_from_file(gateway_file, default_gw, max_size) != 0) {
        MOBILE_DEBUG("Failed to read default gateway from file: %s\n", gateway_file);
        return -1;
    }

    MOBILE_DEBUG("Interface %s default gateway: %s\n", ifname, default_gw);
    return (default_gw[0] != '\0') ? 0 : -1;
}

/**
 * 将十六进制字符转换为十进制值
 * 
 * @param hex_char 十六进制字符
 * @return 十进制值，无效字符返回0
 */
int mobile_hex_char_to_dec(char hex_char) {
    if (hex_char >= '0' && hex_char <= '9') {
        return hex_char - '0';
    } else if (hex_char >= 'A' && hex_char <= 'F') {
        return hex_char - 'A' + 10;
    } else if (hex_char >= 'a' && hex_char <= 'f') {
        return hex_char - 'a' + 10;
    }
    return 0;
}

/**
 * 将十六进制字符串转换为二进制字符串
 * 
 * @param hex_str 十六进制字符串
 * @param bin_str 输出二进制字符串缓冲区
 * @param bin_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 */
int mobile_hex_to_bin(const char* hex_str, char* bin_str, size_t bin_size) {
    size_t hex_len = strlen(hex_str);
    size_t bin_len = hex_len * 4;
    
    if (bin_size < bin_len + 1) {
        MOBILE_ERROR("Binary buffer too small: need %zu, got %zu\n", bin_len + 1, bin_size);
        return -1;
    }
    
    memset(bin_str, 0, bin_size);
    
    for (size_t i = 0; i < hex_len; i++) {
        char hex_char = hex_str[i];
        int dec = mobile_hex_char_to_dec(hex_char);
        
        // 转换为4位二进制
        for (int j = 3; j >= 0; j--) {
            bin_str[i * 4 + (3 - j)] = (dec & (1 << j)) ? '1' : '0';
        }
    }
    
    return 0;
}

/**
 * @brief 解析并清理字段值
 * 
 * 从输入行中提取字段值，并清理逗号和换行符
 * 
 * @param line 输入行
 * @param field_name 字段名
 * @param output 输出缓冲区
 * @param output_size 输出缓冲区大小
 * @return int 成功返回0，失败返回-1
 */
static int mobile_parse_field_value(const char* line, const char* field_name, char* output, size_t output_size) {
    char* ptr = NULL;
    
    if (line == NULL || output == NULL || output_size == 0) {
        return -1;
    }
    
    // 使用sscanf提取字段值
    if (sscanf(line, "%*s %s", output) != 1) {
        return -1;
    }
    
    // 清理逗号
    if ((ptr = strstr(output, ",")) != NULL) {
        *ptr = '\0';
    }
    
    // 清理换行符
    if ((ptr = strstr(output, "\n")) != NULL) {
        *ptr = '\0';
    }
    return 0;
}

/**
 * @brief 解析IPv4地址信息
 * 
 * 从文件流中读取并解析IPv4地址和子网掩码
 * 
 * @param fd 文件流指针
 * @param wandeviceinfo 接口信息结构体指针
 * @return int 成功返回0，失败返回-1
 */
static int mobile_parse_ipv4_address(FILE* fd, inf_status_t* wandeviceinfo) {
    char line[128] = {0};
    
    if (fd == NULL || wandeviceinfo == NULL) {
        return -1;
    }
    
    while (fgets(line, sizeof(line) - 1, fd)) {
        // 检查是否到达数组结束
        if (strstr(line, "],") != NULL) {
            break;
        }
        
        // 解析地址
        if (strstr(line, "address:") != NULL) {
            mobile_parse_field_value(line, "ipv4_address", wandeviceinfo->ipv4_address, 
                                    sizeof(wandeviceinfo->ipv4_address));
        }
        
        // 解析子网掩码
        if (strstr(line, "mask:") != NULL) {
            if (mobile_parse_field_value(line, "mask", wandeviceinfo->mask, 
                                        sizeof(wandeviceinfo->mask)) == 0) {
                break; // 找到掩码后退出
            }
        }
    }
    
    return 0;
}

/**
 * @brief 解析IPv6地址信息
 * 
 * 从文件流中读取并解析IPv6地址和子网掩码
 * 
 * @param fd 文件流指针
 * @param wandeviceinfo 接口信息结构体指针
 * @param is_prefix 是否为IPv6前缀地址
 * @return int 成功返回0，失败返回-1
 */
static int mobile_parse_ipv6_address(FILE* fd, inf_status_t* wandeviceinfo, bool is_prefix) {
    char line[128] = {0};
    
    if (fd == NULL || wandeviceinfo == NULL) {
        return -1;
    }
    
    while (fgets(line, sizeof(line) - 1, fd)) {
        // 检查是否到达数组结束
        if (strstr(line, "],") != NULL) {
            break;
        }
        
        // 解析地址
        if (strstr(line, "address:") != NULL) {
            if (is_prefix) {
                mobile_parse_field_value(line, "ipv6_prefix_address", 
                                       wandeviceinfo->ipv6_prefix_address, 
                                       sizeof(wandeviceinfo->ipv6_prefix_address));
            } else {
                mobile_parse_field_value(line, "ipv6_address", 
                                       wandeviceinfo->ipv6_address, 
                                       sizeof(wandeviceinfo->ipv6_address));
            }
        }
        
        // 解析子网掩码
        if (strstr(line, "mask:") != NULL) {
            if (is_prefix) {
                if (mobile_parse_field_value(line, "ipv6_prefix_mask", 
                                           wandeviceinfo->ipv6_prefix_mask, 
                                           sizeof(wandeviceinfo->ipv6_prefix_mask)) == 0) {
                    break;
                }
            } else {
                if (mobile_parse_field_value(line, "ipv6_mask", 
                                           wandeviceinfo->ipv6_mask, 
                                           sizeof(wandeviceinfo->ipv6_mask)) == 0) {
                    break;
                }
            }
        }
    }
    
    return 0;
}

/**
 * @brief 解析路由信息
 * 
 * 从文件流中读取并解析下一跳网关地址
 * 
 * @param fd 文件流指针
 * @param wandeviceinfo 接口信息结构体指针
 * @return int 成功返回0，失败返回-1
 */
static int mobile_parse_route_info(FILE* fd, inf_status_t* wandeviceinfo) {
    char line[128] = {0};
    
    if (fd == NULL || wandeviceinfo == NULL) {
        return -1;
    }
    
    while (fgets(line, sizeof(line) - 1, fd)) {
        // 检查是否到达数组结束
        if (strstr(line, "],") != NULL) {
            break;
        }
        
        // 解析下一跳地址
        if (strstr(line, "nexthop:") != NULL) {
            if (mobile_parse_field_value(line, "nexthop", wandeviceinfo->nexthop, 
                                        sizeof(wandeviceinfo->nexthop)) == 0) {
                break;
            }
        }
    }
    
    return 0;
}

/**
 * @brief 解析DNS服务器信息
 * 
 * 从文件流中读取并解析DNS服务器地址列表
 * 
 * @param fd 文件流指针
 * @param wandeviceinfo 接口信息结构体指针
 * @return int 成功返回0，失败返回-1
 */
static int mobile_parse_dns_servers(FILE* fd, inf_status_t* wandeviceinfo) {
    char line[128] = {0};
    char dnsip[512] = {0};
    char dnsbuf[512] = {0};
    char* ptr = NULL;
    
    if (fd == NULL || wandeviceinfo == NULL) {
        return -1;
    }
    
    while (fgets(line, sizeof(line) - 1, fd)) {
        // 检查是否到达数组结束
        if (strstr(line, "],") != NULL) {
            break;
        }
        
        memset(dnsip, 0, sizeof(dnsip));
        memset(dnsbuf, 0, sizeof(dnsbuf));
        
        // 提取DNS IP地址
        ptr = strrchr(line, '\t');
        if (ptr) {
            snprintf(dnsip, sizeof(dnsip), "%s", ptr + 1);
        } else {
            snprintf(dnsip, sizeof(dnsip), "%s", line);
        }
        
        // 清理换行符和逗号
        if ((ptr = strstr(dnsip, "\n")) != NULL) {
            *ptr = '\0';
        }
        if ((ptr = strstr(dnsip, ",")) != NULL) {
            *ptr = '\0';
        }
        
        // 验证并保存DNS地址
        if (strcmp(dnsip, "") != 0 && strlen(dnsip) >= 7) {
            if (wandeviceinfo->dns[0] == '\0') {
                snprintf(wandeviceinfo->dns, sizeof(wandeviceinfo->dns), "%s", dnsip);
            } else {
                snprintf(dnsbuf, sizeof(dnsbuf), "%s,%s", wandeviceinfo->dns, dnsip);
                snprintf(wandeviceinfo->dns, sizeof(wandeviceinfo->dns), "%s", dnsbuf);
            }
        }
    }
    
    return 0;
}

/**
 * @brief 处理单行网络接口信息
 * 
 * 根据行内容分发到相应的解析函数
 * 
 * @param line 输入行
 * @param fd 文件流指针
 * @param wandeviceinfo 接口信息结构体指针
 * @param read_end 读取结束标志指针
 * @return int 成功返回0，失败返回-1
 */
static int mobile_process_interface_line(const char* line, FILE* fd, inf_status_t* wandeviceinfo, bool* read_end) {
    if (line == NULL || fd == NULL || wandeviceinfo == NULL || read_end == NULL) {
        return -1;
    }
    
    // 检查接口状态
    if (strstr(line, "up: true") != NULL) {
        wandeviceinfo->status = 1;
        return 0;
    }
    
    // 检查运行时间
    if (strstr(line, "uptime:") != NULL) {
        mobile_parse_field_value(line, "uptime", wandeviceinfo->uptime, sizeof(wandeviceinfo->uptime));
        return 0;
    }
    
    // 检查L3设备
    if (strstr(line, "l3_device:") != NULL) {
        mobile_parse_field_value(line, "l3_device", wandeviceinfo->l3_device, sizeof(wandeviceinfo->l3_device));
        return 0;
    }
    
    // 检查协议
    if (strstr(line, "proto:") != NULL) {
        mobile_parse_field_value(line, "proto", wandeviceinfo->proto, sizeof(wandeviceinfo->proto));
        return 0;
    }
    
    // 检查设备
    if (strstr(line, "device:") != NULL) {
        mobile_parse_field_value(line, "device", wandeviceinfo->device, sizeof(wandeviceinfo->device));
        return 0;
    }
    
    // 解析IPv4地址信息
    if (strstr(line, "ipv4-address:") != NULL) {
        mobile_parse_ipv4_address(fd, wandeviceinfo);
        return 0;
    }
    
    // 解析IPv6地址信息
    if (strstr(line, "ipv6-address:") != NULL) {
        mobile_parse_ipv6_address(fd, wandeviceinfo, false);
        return 0;
    }
    
    // 解析IPv6前缀地址信息
    if (strstr(line, "ipv6-prefix:") != NULL) {
        mobile_parse_ipv6_address(fd, wandeviceinfo, true);
        return 0;
    }
    
    // 解析路由信息
    if (strstr(line, "route:") != NULL) {
        mobile_parse_route_info(fd, wandeviceinfo);
        return 0;
    }
    
    // 解析DNS服务器信息
    if (strstr(line, "dns-server:") != NULL) {
        mobile_parse_dns_servers(fd, wandeviceinfo);
        return 0;
    }
    
    // 检查是否到达非活动部分
    if (strstr(line, "inactive: {") != NULL) {
        *read_end = true;
        return 0;
    }
    
    return 0;
}

/**
 * @brief 获取网络接口信息（优化版本）
 * 
 * 通过执行ubus命令获取指定网络接口的状态信息，包括：
 * - 接口状态（up/down）
 * - 运行时间
 * - L3设备名称
 * - 协议类型
 * - 设备名称
 * - IPv4地址和子网掩码
 * - IPv6地址和子网掩码
 * - IPv6前缀地址和掩码
 * - 路由下一跳地址
 * - DNS服务器列表
 * 
 * 函数特点：
 * - 使用结构化方法，将复杂逻辑拆分为多个子函数
 * - 每个子函数有明确的单一职责
 * - 添加了详细的注释和错误处理
 * - 使用状态机模式处理不同的解析场景
 * 
 * @param inf 接口名称（如"wan5g01"）
 * @param wandeviceinfo 接口信息结构体指针，用于存储解析结果
 * @return void
 */
void mobile_get_interface_info(char* inf, inf_status_t* wandeviceinfo) {
    bool read_end = false;
    FILE* fd = NULL;
    char line[128] = {0};
    char cmd[128] = {0};
    
    // 参数验证
    if (inf == NULL || wandeviceinfo == NULL) {
        MOBILE_ERROR("Invalid parameters: inf or wandeviceinfo is NULL");
        return;
    }
    
    // 初始化输出结构体
    memset(wandeviceinfo, 0, sizeof(inf_status_t));
    
    // 构建ubus命令
    snprintf(cmd, sizeof(cmd), "ubus call network.interface.%s status | sed 's/\"//g'", inf);
    
    // 执行命令并打开管道
    fd = popen(cmd, "r");
    if (fd == NULL) {
        MOBILE_ERROR("Failed to execute command: %s", cmd);
        return;
    }
    
    // 逐行读取并解析输出
    while (fgets(line, sizeof(line) - 1, fd)) {
        // 如果已经到达非活动部分，跳过后续行
        if (read_end) {
            continue;
        }
        
        // 处理当前行
        mobile_process_interface_line(line, fd, wandeviceinfo, &read_end);
    }
    
    // 关闭管道
    pclose(fd);
}

/**
 * @brief 检查接口是否工作
 * @return up返回true，down返回false
 */
bool mobile_check_interface_up(char *interface) {
    if (interface == NULL)
        return false;

    inf_status_t inf_status;
    memset(&inf_status, 0, sizeof(inf_status));
    mobile_get_interface_info(interface, &inf_status);
    if (inf_status.status)
        return true;
    else
        return false;
}

/**
 * @brief 检查网络接口是否存在
 *
 * 通过执行ubus list network.interface.*命令检查指定的网络接口是否存在
 * 函数会执行ubus list network.interface.*命令，然后在输出中查找匹配的接口名
 *
 * @param interface 要检查的接口名称（如"lan", "wan", "wan5g", "wan5g01"等）
 * @return true - 接口存在, false - 接口不存在
 */
bool mobile_check_interface_exists(const char* interface) {
    FILE* fp = NULL;
    char cmd[128] = {0};
    char line[256] = {0};
    char interface_pattern[64] = {0};
    bool found = false;
    
    // 参数验证
    if (interface == NULL || interface[0] == '\0') {
        MOBILE_ERROR("Invalid parameter: interface is NULL or empty\n");
        return false;
    }
    
    // 构建接口匹配模式
    snprintf(interface_pattern, sizeof(interface_pattern), "network.interface.%s", interface);
    
    // 构建ubus命令
    snprintf(cmd, sizeof(cmd), "ubus list network.interface.*");
    
    // 执行命令并打开管道
    fp = popen(cmd, "r");
    if (fp == NULL) {
        MOBILE_ERROR("Failed to execute command: %s\n", cmd);
        return false;
    }
    
    // 逐行读取输出并查找匹配的接口
    while (fgets(line, sizeof(line) - 1, fp)) {
        // 去除行尾的换行符
        char* newline = strchr(line, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }
        
        // 检查是否匹配目标接口
        if (strcmp(line, interface_pattern) == 0) {
            found = true;
            break;
        }
    }
    
    // 关闭管道
    pclose(fp);
    
    MOBILE_DEBUG("Interface %s %s\n", interface, found ? "exists" : "does not exist");
    return found;
}