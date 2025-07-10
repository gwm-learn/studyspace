#include "schedule.h"

static Timer timers[MAX_TIMERS];
static int epoll_fd = -1;

// 初始化定时器管理器
int timer_manager_init(void) {
    // 创建epoll实例
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        printf("epoll_create1 failed\n");
        return -1;
    }

    // 初始化所有定时器为空状态
    for (int i = 0; i < MAX_TIMERS; i++) {
        timers[i].tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timers[i].tfd < 0) {
            printf("timerfd_create failed\n");
            return -1;
        }

        // 添加到epoll
        struct epoll_event ev = {
            .events = EPOLLIN,
            .data.ptr = &timers[i]
        };
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timers[i].tfd, &ev) < 0) {
            printf("epoll_ctl add failed\n");
            close(timers[i].tfd);
            return -1;
        }

        // 设置初始状态
        timers[i].state = TIMER_STATE_EMPTY;
        strncpy(timers[i].key, EMPTY_KEY, MAX_KEY_LEN);
        memset(&timers[i].spec, 0, sizeof(struct itimerspec));
        timers[i].callback = NULL;
    }
    
    return 0;
}

// 查找空闲定时器索引
int find_free_timer_index(void) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].state == TIMER_STATE_EMPTY) {
            return i;
        }
    }
    return -1; // 没有空闲定时器
}

// 检查键值是否唯一（包括默认键值）
bool is_key_unique(const char* key) {
    // 空指针键值视为无效
    if (!key) {
        return false;
    }
    
    // 空键值不需要唯一性检查（将自动转为默认键值）
    if (strlen(key) == 0) {
        return true;
    }
    
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].state == TIMER_STATE_WAITING && 
            strcmp(timers[i].key, key) == 0) {
            return false; // 键值已存在
        }
    }
    return true; // 键值唯一
}

// 根据键值查找定时器索引
int find_timer_index_by_key(const char* key) {
    if (!key) {
        return -1;
    }
    
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].state == TIMER_STATE_WAITING && 
            strcmp(timers[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

// 设置定时器参数并自动启动
int timer_set_parameters(const char* key, long timeout_sec, long timeout_nsec, void (*callback)(const char*)) {
    // 查找空闲定时器
    int index = find_free_timer_index();
    if (index < 0) {
        printf("No free timers available\n");
        return -1;
    }

    Timer* timer = &timers[index];
    
    // 确定实际使用的键值
    char actual_key[MAX_KEY_LEN];
    if (key && strlen(key) > 0) {
        // 检查非空键值是否唯一
        if (!is_key_unique(key)) {
            printf("Key '%s' already exists\n", key);
            return -1;
        }
        strncpy(actual_key, key, MAX_KEY_LEN-1);
        actual_key[MAX_KEY_LEN-1] = '\0';
    } else {
        // 检查默认键值是否唯一
        if (!is_key_unique(DEFAULT_KEY)) {
            printf("Default key '%s' already in use\n", DEFAULT_KEY);
            return -1;
        }
        strncpy(actual_key, DEFAULT_KEY, MAX_KEY_LEN);
    }

    // 设置键值
    strncpy(timer->key, actual_key, MAX_KEY_LEN);
    
    // 设置超时时间
    timer->spec.it_value.tv_sec = timeout_sec;
    timer->spec.it_value.tv_nsec = timeout_nsec;
    
    // 设置回调函数
    timer->callback = callback;
    
    // 启动定时器
    if (timerfd_settime(timer->tfd, 0, &timer->spec, NULL) < 0) {
        printf("timerfd_settime failed\n");
        return -1;
    }
    
    // 更新状态
    timer->state = TIMER_STATE_WAITING;
    
    return index; // 返回分配的定时器索引
}

// 更新定时器超时时间
int timer_update_timeout(const char* key, long new_sec, long new_nsec) {
    int index = -1;

    if (key == NULL) {
        index = find_timer_index_by_key(DEFAULT_KEY);
    } else {
        index = find_timer_index_by_key(key);
    }
    if (index < 0) {
        printf("Timer with key '%s' not found or not in waiting state\n", key);
        return -1;
    }

    Timer* timer = &timers[index];
    
    // 更新超时时间
    timer->spec.it_value.tv_sec = new_sec;
    timer->spec.it_value.tv_nsec = new_nsec;
    
    // 重新设置定时器
    if (timerfd_settime(timer->tfd, 0, &timer->spec, NULL) < 0) {
        printf("timerfd_settime update failed\n");
        return -1;
    }
    
    return 0;
}

// 通过键值重置定时器
int timer_reset_by_key(const char* key) {
    int index = find_timer_index_by_key(key);
    if (index < 0) {
        printf("Timer with key '%s' not found\n", key);
        return -1;
    }
    
    Timer* timer = &timers[index];
    
    // 清除定时器
    struct itimerspec stop_spec = {0};
    if (timerfd_settime(timer->tfd, 0, &stop_spec, NULL) < 0) {
        printf("timerfd_settime reset failed\n");
        return -1;
    }
    
    // 重置状态和参数
    timer->state = TIMER_STATE_EMPTY;
    strncpy(timer->key, EMPTY_KEY, MAX_KEY_LEN);
    memset(&timer->spec, 0, sizeof(struct itimerspec));
    timer->callback = NULL;
    
    return 0;
}

// 处理定时器事件
void process_timer_events(void) {
    char key_copy[MAX_KEY_LEN] = {0};
    struct epoll_event events[MAX_TIMERS];
    int n = epoll_wait(epoll_fd, events, MAX_TIMERS, 100); // 100ms超时
    
    if (n < 0) {
        if (errno != EINTR) {
            printf("epoll_wait failed\n");
        }
        return;
    }
    
    for (int i = 0; i < n; i++) {
        Timer* timer = (Timer*)events[i].data.ptr;
        
        // 只处理等待状态的定时器
        if (timer->state != TIMER_STATE_WAITING) {
            continue;
        }
        
        // 读取事件数据
        uint64_t expirations;
        ssize_t bytes = read(timer->tfd, &expirations, sizeof(expirations));
        
        if (bytes < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printf("read timerfd failed\n");
            }
            continue;
        }
        
        // 确保读取到完整数据
        if (bytes != sizeof(expirations)) {
            printf("Incomplete read from timerfd\n");
            continue;
        }
        
        // 保存键值副本（因为回调后可能重置定时器）
        strncpy(key_copy, timer->key, MAX_KEY_LEN);
        
        // 执行回调
        if (timer->callback) {
            timer->callback(key_copy);
        }
        
        // 通过键值重置定时器
        timer_reset_by_key(key_copy);
    }
}

// 获取定时器状态信息
void print_timer_status(void) {
    printf("\n--- Timer Status ---\n");
    for (int i = 0; i < MAX_TIMERS; i++) {
        const char* state_str = (timers[i].state == TIMER_STATE_EMPTY) ? 
                               "EMPTY" : "WAITING";
        printf("Timer %d: [%s] Key='%s' Timeout=%lds %ldns\n", 
               i, state_str, timers[i].key, 
               timers[i].spec.it_value.tv_sec, 
               timers[i].spec.it_value.tv_nsec);
    }
    printf("\n-------------------\n");
}

// 清理资源
void timer_manager_cleanup(void) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].tfd >= 0) {
            close(timers[i].tfd);
            timers[i].tfd = -1;
        }
    }
    
    if (epoll_fd >= 0) {
        close(epoll_fd);
        epoll_fd = -1;
    }
}

// 示例回调函数
void sample_callback(const char* key) {
    printf("Timer '%s' triggered!\n", key);
}

int main_test(void) {
    if (timer_manager_init() != 0) {
        fprintf(stderr, "Timer manager initialization failed\n");
        return 1;
    }
    
    // 测试1: 创建唯一键值定时器
    printf("Creating BackupTimer...\n");
    int timer1_idx = timer_set_parameters("BackupTimer", 5, 0, sample_callback);
    if (timer1_idx < 0) {
        fprintf(stderr, "Failed to create BackupTimer\n");
    } else {
        printf("BackupTimer created at index %d\n", timer1_idx);
    }
    
    // 测试2: 创建默认键值定时器
    printf("\nCreating default key timer...\n");
    int timer2_idx = timer_set_parameters("", 6, 0, sample_callback);
    if (timer2_idx < 0) {
        fprintf(stderr, "Failed to create default key timer\n");
    } else {
        printf("Default key timer created at index %d\n", timer2_idx);
    }
    
    // 测试3: 尝试创建重复键值
    printf("\nAttempting to create duplicate BackupTimer...\n");
    int timer3_idx = timer_set_parameters("BackupTimer", 7, 0, sample_callback);
    if (timer3_idx < 0) {
        fprintf(stderr, "Failed to create duplicate BackupTimer (expected)\n");
    } else {
        printf("Duplicate BackupTimer created (unexpected!)\n");
    }
    
    // 测试4: 尝试创建第二个默认键值定时器
    printf("\nAttempting to create second default key timer...\n");
    int timer4_idx = timer_set_parameters(NULL, 8, 0, sample_callback);
    if (timer4_idx < 0) {
        fprintf(stderr, "Failed to create second default key timer (expected)\n");
    } else {
        printf("Second default key timer created (unexpected!)\n");
    }
    
    print_timer_status();
    
    // 测试5: 重置定时器后可以重用键值
    if (timer1_idx >= 0) {
        printf("\nResetting BackupTimer...\n");
        if (timer_reset_by_key("BackupTimer") == 0) {
            printf("BackupTimer reset\n");
            
            // 尝试重新创建相同键值
            printf("Recreating BackupTimer...\n");
            int timer5_idx = timer_set_parameters("BackupTimer", 3, 0, sample_callback);
            if (timer5_idx < 0) {
                fprintf(stderr, "Failed to recreate BackupTimer\n");
            } else {
                printf("BackupTimer recreated at index %d\n", timer5_idx);
            }
        }
    }
    
    print_timer_status();
    
    printf("\nRunning timers (press Ctrl+C to exit)...\n");
    
    // 事件处理循环
    while (1) {
        process_timer_events();
        
        // 添加一些延迟以减少CPU使用
        usleep(10000); // 10ms
    }
    
    timer_manager_cleanup();
    return 0;
}