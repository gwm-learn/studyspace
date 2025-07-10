#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_TIMERS 5
#define MAX_KEY_LEN 64
#define EMPTY_KEY "EMPTY"
#define DEFAULT_KEY "DEFAULT"

// 简化的定时器状态
typedef enum {
    TIMER_STATE_EMPTY,    // 空状态（未使用）
    TIMER_STATE_WAITING    // 等待超时触发状态
} TimerState;

// 定时器结构体
typedef struct {
    int tfd;               // timerfd 文件描述符
    TimerState state;       // 当前状态
    char key[MAX_KEY_LEN];  // 定时器ID键值
    struct itimerspec spec; // 定时器时间规格
    void (*callback)(const char*); // 回调函数
} Timer;

int timer_manager_init(void);
int find_free_timer_index(void);
bool is_key_unique(const char* key);
int find_timer_index_by_key(const char* key);
int timer_set_parameters(const char* key, long timeout_sec, long timeout_nsec, void (*callback)(const char*));
int timer_update_timeout(const char* key, long new_sec, long new_nsec);
int timer_reset_by_key(const char* key);
void process_timer_events(void);
void print_timer_status(void);
void timer_manager_cleanup(void);
