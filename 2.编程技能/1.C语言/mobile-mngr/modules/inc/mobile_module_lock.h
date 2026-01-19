#ifndef MOBILE_MODULE_LOCK_H
#define MOBILE_MODULE_LOCK_H

/**
 * @brief  支持的band配置
 * 
 */
typedef struct {
    unsigned int vid;
    unsigned int pid;
    const char* module_name;        /**< 模块名称 */
    const char* band_list_3g;       /**< 支持的3g band */
    const char* band_list_4g;       /**< 支持的4g band */
    const char* band_list_5g_sa;    /**< 支持的5g sa band */
    const char* band_list_5g_nsa;   /**< 支持的5g nsa band */
} support_band_list_t;

int mobile_init_support_band_list(unsigned int pid, unsigned int vid, support_band_list_t **support_band_list);

#endif // MOBILE_MODULE_LOCK_H