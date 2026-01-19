#ifndef MOBILE_SERVICE_DIAL_H
#define MOBILE_SERVICE_DIAL_H

typedef enum {
    CFUN_0 = 0,
    CFUN_1 = 1,
    CFUN_4 = 4,
} cfun_status;

/**
 * @brief CFUN set state machine states
 */
typedef enum {
    CFUN_STATE_INIT = 0,      // Initial state
    CFUN_STATE_SET_VALUE = 1,  // Set specified value state
    CFUN_STATE_SET_CFUN1 = 2,  // Set CFUN=1 state
} cfun_state_t;

int mobile_init_dial_service(void);
void mobile_deinit_dial_service(void);

void mobile_init_cfun(cfun_status status);
void mobile_execute_cfun_sequence(int value);
bool mobile_update_cfun(int value);
bool mobile_check_cfun(int expected_value);

void mobile_network_dial_ex(const char *interface_name, bool enable, int vlanid);
void mobile_network_dedial_ex(const char *interface_name, bool enable, int vlanid);
void mobile_network_dial_common(const char *interface_name, bool enable, int vlanid);
void mobile_network_dial_basic(void);
void mobile_network_dial_multi_single(module_mutil_config_t *current_config);
void mobile_network_dial(void);

#endif