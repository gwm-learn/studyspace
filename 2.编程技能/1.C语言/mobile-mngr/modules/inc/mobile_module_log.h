#ifndef MOBILE_MODULE_LOG_H
#define MOBILE_MODULE_LOG_H

#include <limits.h>
#include <stdio.h>

/* 日志等级枚举 */
typedef enum {
    MOBILE_LOG_LEVEL_NONE = 0,    /* 不输出任何日志 */
    MOBILE_LOG_LEVEL_INFO,        /* 信息级别 */
    MOBILE_LOG_LEVEL_DEBUG,       /* 调试级别 */
    MOBILE_LOG_LEVEL_WARN,        /* 警告级别 */
    MOBILE_LOG_LEVEL_ERROR        /* 错误级别 */
} mobile_log_level_t;

/* 日志模块上下文结构体 */
typedef struct {
    mobile_log_level_t level;      /* 当前日志等级 */
    char file[PATH_MAX];           /* 日志文件路径 */
    unsigned int max_file_size;    /* 最大文件大小（字节） */
    FILE* fp;                      /* 日志文件指针 */
} mobile_log_context_t;

/* 调试宏定义 */
#define MOBILE_INFO(fmt, ...) \
    do { \
        mobile_log_write(MOBILE_LOG_LEVEL_INFO, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

#define MOBILE_DEBUG(fmt, ...) \
    do { \
        mobile_log_write(MOBILE_LOG_LEVEL_DEBUG, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

#define MOBILE_WARN(fmt, ...) \
    do { \
        mobile_log_write(MOBILE_LOG_LEVEL_WARN, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

#define MOBILE_ERROR(fmt, ...) \
    do { \
        mobile_log_write(MOBILE_LOG_LEVEL_ERROR, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

/* 日志配置函数声明 */
int mobile_log_init(mobile_log_level_t level, const char* log_file, unsigned int max_size_mb);
void mobile_log_write(mobile_log_level_t level, const char* func, int line, const char* fmt, ...);

#endif