#ifndef MOBILE_SERVICE_FAILOVER_H
#define MOBILE_SERVICE_FAILOVER_H

#include <pthread.h>

#define MAX_PING_ADDRESS_NUM 2
#define PING_TEMP_FILE "/var/tmp/ping"

typedef struct {
    char eth_wan_gw_if_name[32];
    char active_gw_if_name[32];
    char ip_address1[16];
    char ip_address2[16];
    unsigned char enable;
    unsigned char icmp_check_enable;
    unsigned int retry_times;
    unsigned int period_time;
} failover_st;

typedef struct {
    pthread_t thread_id;
    volatile int running;
    pthread_mutex_t running_lock;
} ping_check_thread_t;

int mobile_init_failover_service(void);
void mobile_deinit_failover_service(void);
int mobile_init_failover_config(void);
int mobile_init_ping_check_task(void);
void mobile_deinit_ping_check_task(void);
void mobile_set_ping_check_thread_running(int running);
int mobile_get_ping_check_thread_running(void);

unsigned int mobile_ping_check_loop(void);
int mobile_ping_test(char* ip_addr, char* if_name);
unsigned char mobile_check_eth_net_is_link(char* active_gw_if_name);
unsigned char mobile_check_eth_net_is_no_link(char* active_gw_if_name);
void mobile_add_route_for_ping_addresses(void);
int mobile_get_ifname_gw_ip(char* wan_interface, char* if_name, char* gw);
void mobile_switch_default_route_by_ping(char* def_route, char* to_route);

#endif