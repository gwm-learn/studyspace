#ifndef MOBILE_SERVICE_SIGNAL_H
#define MOBILE_SERVICE_SIGNAL_H

int mobile_setup_signal_handlers(void);
int mobile_init_signal_service(void);
void mobile_deinit_signal_service(void);
int mobile_get_global_signal(void);
#endif