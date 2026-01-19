#ifndef MOBILE_SERVICE_TIMER_H
#define MOBILE_SERVICE_TIMER_H

int mobile_init_timer_service(void);
void mobile_deinit_timer_service(void);

void mobile_timer_check_modem(void);
void mobile_process_ussd(void);

void mobile_timer_main(void);

#endif