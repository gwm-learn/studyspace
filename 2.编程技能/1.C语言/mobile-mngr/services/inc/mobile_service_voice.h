#ifndef MOBILE_SERVICE_VOICE_H
#define MOBILE_SERVICE_VOICE_H

int mobile_init_voice_service(void);
void mobile_deinit_voice_service(void);
int mobile_init_voice_config(void);

void mobile_update_ims_status(void);
void mobile_update_ims_status_ext(void);
void mobile_network_slic(int reset);

int mobile_voice_is_supported(void);
void mobile_voice_configure_network(void);
void mobile_voice_launch_process(void);

#endif