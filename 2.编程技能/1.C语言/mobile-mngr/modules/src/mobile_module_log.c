#include <stdarg.h>

#include "mobile_module_util.h"
#include "mobile_module_log.h"

/*
 * 文件名称：mobile_module_log.c
 * 功能描述：
 *     log基础模块，提供log基础设置功能
 *
 * 作者：gaoweiming
 */

/* 日志模块全局变量 */
static mobile_log_context_t g_log_ctx = {
    .level = MOBILE_LOG_LEVEL_INFO,
    .file = "/tmp/mobile/log",
    .max_file_size = 10 * 1024 * 1024, /* 默认10MB */
    .fp = NULL
};

/**
 * 初始化日志配置
 * @param level 日志等级
 * @param log_file 日志文件完整路径
 * @param max_size_mb 日志文件大小限制（MB为单位）
 * @return 成功返回0，失败返回-1
 */
int mobile_log_init(mobile_log_level_t level, const char* log_file, unsigned int max_size_mb)
{
    if (log_file == NULL) {
        printf("[ERROR] Log file path is NULL\n");
        return -1;
    }

    /* 设置日志等级 */
    g_log_ctx.level = level;

    /* 设置日志文件路径 */
    if (strlen(log_file) >= sizeof(g_log_ctx.file)) {
        printf("[ERROR] Log file path too long: %s\n", log_file);
        return -1;
    }
    strcpy(g_log_ctx.file, log_file);

    /* 设置文件大小限制 */
    g_log_ctx.max_file_size = max_size_mb * 1024 * 1024;

    /* 创建日志目录 */
    char* last_slash = strrchr(g_log_ctx.file, '/');
    if (last_slash != NULL) {
        char dir_path[PATH_MAX] = {0};
        size_t dir_len = last_slash - g_log_ctx.file;
        if (dir_len >= sizeof(dir_path)) {
            printf("[ERROR] Directory path too long\n");
            return -1;
        }
        strncpy(dir_path, g_log_ctx.file, dir_len);
        dir_path[dir_len] = '\0';

        /* 创建目录 */
        char cmd[PATH_MAX + 32] = {0};
        if (snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir_path) >= (int)sizeof(cmd)) {
            printf("[ERROR] Command buffer overflow\n");
            return -1;
        }
        system(cmd);
    }

    printf("[INFO] Log initialized: level=%d, file=%s, max_size=%uMB\n",
           level, log_file, max_size_mb);
    
    return 0;
}

/**
 * 检查并处理日志文件大小限制
 */
static void mobile_log_check_file_size(void)
{
    if (g_log_ctx.fp == NULL) {
        return;
    }

    /* 获取当前文件大小 */
    long current_size = ftell(g_log_ctx.fp);
    if (current_size < 0) {
        return;
    }

    /* 如果文件大小超过限制，则清空文件 */
    if ((unsigned long)current_size >= g_log_ctx.max_file_size) {
        fclose(g_log_ctx.fp);
        g_log_ctx.fp = NULL;
        
        /* 重新以写入模式打开文件，会清空文件内容 */
        g_log_ctx.fp = fopen(g_log_ctx.file, "w");
        if (g_log_ctx.fp == NULL) {
            printf("[ERROR] Failed to reopen log file: %s\n", g_log_ctx.file);
        } else {
            /* 重新设置为追加模式 */
            fclose(g_log_ctx.fp);
            g_log_ctx.fp = fopen(g_log_ctx.file, "a");
            if (g_log_ctx.fp == NULL) {
                printf("[ERROR] Failed to reopen log file in append mode: %s\n", g_log_ctx.file);
            }
        }
    }
}

/**
 * 写入日志到文件
 * @param level 日志等级
 * @param func 函数名
 * @param line 行号
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void mobile_log_write(mobile_log_level_t level, const char* func, int line, const char* fmt, ...)
{
    /* 检查日志等级是否满足输出条件 */
    /* 当全局日志等级为NONE时，不输出任何日志 */
    if (g_log_ctx.level == MOBILE_LOG_LEVEL_NONE) {
        return;
    }
    
    /* 当全局日志等级不为NONE时，ERROR等级总是输出 */
    if (level == MOBILE_LOG_LEVEL_ERROR ||
        level == MOBILE_LOG_LEVEL_WARN) {
        /* ERROR等级在所有非NONE等级都输出 */
    } else if (level > g_log_ctx.level) {
        /* 其他等级按正常规则过滤 */
        return;
    }

    /* 打开日志文件（如果尚未打开） */
    if (g_log_ctx.fp == NULL) {
        g_log_ctx.fp = fopen(g_log_ctx.file, "a");
        if (g_log_ctx.fp == NULL) {
            printf("[ERROR] Failed to open log file: %s\n", g_log_ctx.file);
            return;
        }
    }

    /* 检查文件大小 */
    mobile_log_check_file_size();

    /* 获取当前时间 */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[64] = {0};
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    /* 格式化日志等级字符串 */
    const char* level_str = "";
    switch (level) {
        case MOBILE_LOG_LEVEL_INFO:
            level_str = "INFO";
            break;
        case MOBILE_LOG_LEVEL_DEBUG:
            level_str = "DEBUG";
            break;
        case MOBILE_LOG_LEVEL_WARN:
            level_str = "WARN";
            break;
        case MOBILE_LOG_LEVEL_ERROR:
            level_str = "ERROR";
            break;
        default:
            level_str = "UNKNOWN";
            break;
    }

    /* 写入日志头信息 */
    if (level == MOBILE_LOG_LEVEL_INFO) {
        fprintf(g_log_ctx.fp, "[%s] [%s] ", time_str, level_str);
    } else {
        fprintf(g_log_ctx.fp, "[%s] [%s] %s:%d ", time_str, level_str, func, line);
    }

    /* 写入日志内容 */
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log_ctx.fp, fmt, args);
    va_end(args);

    /* 确保换行 */
    fprintf(g_log_ctx.fp, "\n");
    fflush(g_log_ctx.fp);

    /* 同时输出到控制台（用于调试） */
    if (level == MOBILE_LOG_LEVEL_INFO) {
        printf("[%s] ", level_str);
    } else {
        printf("[%s] %s:%d ", level_str, func, line);
    }
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
