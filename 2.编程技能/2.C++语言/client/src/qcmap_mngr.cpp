#include "qcmap_mngr.h"
#include "qcmap_mngr_config.h"
#include "qcmap_mngr_modem.h"
#include "qcmap_mngr_info.h"
#include "qcmap_mngr_apn.h"
#include "qcmap_mngr_led.h"
#include "qcmap_mngr_lock.h"
#include "qcmap_mngr_wan.h"

void qcmap_init_signal(void) {
    signal(SIGUSR1, qcmap_signal_handler);
    signal(SIGINT, qcmap_signal_handler);
    signal(SIGHUP, qcmap_signal_handler);
    signal(SIGTERM, qcmap_signal_handler);
    signal(SIGKILL, qcmap_signal_handler);
    PRINT_DEBUG("init signal success\n");
}

void qcmap_init_client(void) {
#ifdef FEATURE_EXTERNAL_AP
    qcmap_client = new QCMAP_Client(qcmap_msgr_qmi_qcmap_ind, QCMAP_FUSION_ARCH_V01);
#else
    qcmap_client = new QCMAP_Client(qcmap_msgr_qmi_qcmap_ind);
#endif

    if (qcmap_client->IsReady() == false) {
        PRINT_DEBUG("qcmap client up fail\n");
        qcmap_signal_handler(SIGTERM);
        PRINT_DEBUG("------------------------>>> qcmap_mngr end ------------------------>>>\n");
        exit(1);
    }

    PRINT_DEBUG("init client success\n");
}

void qcmap_get_profile_num(int* profile_num) {
    qcmap_wwan_policy_list_info wwan_policy_list;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

    if (!qcmap_client->GetWWANPolicyListEx(&wwan_policy_list, &qmi_err_num)) {
        PRINT_DEBUG("get profile num fail, error: 0x%x\n", qmi_err_num);
        *profile_num = 1;
        return;
    }

    *profile_num = wwan_policy_list.wwan_policy_len;
}

void qcmap_add_profile_num(int old_num) {
    if (old_num >= g_max_apn_num) {
        PRINT_DEBUG("profile num is [%d], do not need add\n", old_num);
        return;
    }

    for (int i = old_num; i < g_max_apn_num; i++) {
        qcmap_create_profile(old_num + 1);
    }
}

void qcmap_create_profile(int profile_index) {
    qcmap_net_policy_info net_policy;
    profile_handle_type_v01 profile_handle;
    memset(&net_policy, 0, sizeof(net_policy));
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

    net_policy.tech_pref = 0;    //0-ANY, 1-UMTS, 2-CDMA
    net_policy.subscription_id = 1;
    net_policy.profile_id_3gpp = profile_index;
    net_policy.profile_id_3gpp2 = 0;
    net_policy.ip_family = 10;    //IPV4-4 IPV6-6 IPV4V6-10 ETH-12

    //切换到该profile
    if (!qcmap_client->SetWWANProfileHandlePreference(1, &qmi_err_num)) {
        PRINT_DEBUG("change to profile [%d] fail\n", 1);
        return;
    }

    if (!qcmap_client->CreateWWANPolicyEx(net_policy, &profile_handle, &qmi_err_num)) {
        if (qmi_err_num == QMI_ERR_NO_FREE_PROFILE_V01)
            PRINT_DEBUG("Max Profiles reached, Error 0x%x\n", qmi_err_num);
        else if (qmi_err_num == QMI_ERR_INVALID_PROFILE_V01)
            PRINT_DEBUG("Invalid/Duplicate Profile request, Error 0x%x\n", qmi_err_num);
        else
            PRINT_DEBUG("Failed to Create Profile. Error 0x%x\n ", qmi_err_num);
        return;
    }

    PRINT_DEBUG("Create Profile succeeds, profile_handle=%d\n", profile_handle);
}

void qcmap_init_mobileap(void) {
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

    if (!qcmap_client->EnableMobileAP_Ext(&qmi_err_num, indication_mask)) {
        PRINT_DEBUG("mobileap fail, error: 0x%x\n", qmi_err_num);
        qcmap_signal_handler(SIGTERM);
        PRINT_DEBUG("------------------------>>> qcmap_mngr end ------------------------>>>\n");
        exit(1);
    }

    PRINT_DEBUG("mobileap enable success\n");
}

void qcmap_init_config(void) {
    memset(&glb_module_desc, 0, sizeof(glb_module_desc));
    //读取 network wan5gx 配置
    qcmap_get_wan5g_config();
    PRINT_DEBUG("init config success\n");
}

void qcmap_init_profile(void) {
    int profile_num = 0;

    do {
        //读取 profile 个数
        qcmap_get_profile_num(&profile_num);

        //profile 不足则创建
        qcmap_add_profile_num(profile_num);
    } while (profile_num < g_max_apn_num);

    //更新network wan5gx 配置到 profile
    qcmap_update_profile_config();

    //同步profile信息到modem侧
    qcmap_sync_profile_with_modem();

    PRINT_DEBUG("init profile success\n");
}

void qcmap_timer_check_state_machine_running_status(void) {
    PRINT_DEBUG("release start\n");
    //qcmap_release_util();
    qcmap_release_modem();
    qcmap_release_led();
    qcmap_release_config();
    qcmap_release_apn();
    //qcmap_release_info();
    qcmap_release_client();
    qcmap_release_interface();
    qcmap_update_state_machine_status(STATE_MACHINE_NONE);
    usleep(2 * 1000 * 1000);
    PRINT_DEBUG("release finish\n");
}

void qcmap_init_basic(void) {
    int apn_num_t = 0;
    int max_apn_num_t = 0;
    //初始化apnnum
    qcmap_init_apn_num();

    apn_num_t = g_apn_num;
    max_apn_num_t = g_max_apn_num;

    g_apn_num = 1;
    g_max_apn_num = 1;
    work_type = WORK_TYPE_BASIC;

    //重新初始化auto apn 列表
    qcmap_init_auto_apn_list();

    //初始化 profile
    qcmap_init_profile();

    //初始化bridge
    qcmap_init_bridge();

    //使能profile 拨号
    qcmap_dial_profile();

    //初始化默认路由
    qcmap_init_default_route();

    //初始化nat
    qcmap_init_nat();

    //初始化防火墙
    qcmap_init_firewall();

    //更新wan侧信息
    qcmap_wan_status_update(true);

    g_apn_num = apn_num_t;
    g_max_apn_num = max_apn_num_t;
}

void qcmap_init_multi(void) {
    work_type = WORK_TYPE_MULTI;

    //初始化 profile
    qcmap_init_profile();

    //使能profile 拨号
    qcmap_dial_profile();

    //更新wan侧信息
    qcmap_wan_status_update(false);
}

void qcmap_timer_check(void) {
    int ret = 0;
    if (qcmap_get_state_machine_running_status()) { //状态机在运行
        //检查各种配置更新情况并同步信息后重新拨号
        ret |= qcmap_timer_check_config();
        //检查lock配置更新
        ret |= qcmap_timer_check_lock();
        if (ret) { 
            qcmap_update_state_machine_status(STATE_MACHINE_NONE); 
        }
    } else {//状态机未运行
        //检查模组状态，at，cpin，并作对应动作，用状态机方式实现
        qcmap_timer_check_modem();

        if (!qcmap_get_state_machine_running_status()) {
            //获取并更新各种注网信息
            qcmap_timer_check_info();
        }

        if (!qcmap_get_state_machine_running_status()) {
            //检查profile状态
            qcmap_timer_check_profile();
        }

        if (!qcmap_get_state_machine_running_status()) {
            //检查各种配置更新情况并同步信息后重新拨号
            qcmap_timer_check_config();
        }

        if (!qcmap_get_state_machine_running_status()) {
            //检查lock配置更新
            qcmap_timer_check_lock();
        }

        if (!qcmap_get_state_machine_running_status()) {
            //检查wan
            qcmap_timer_check_wan();
        }

        if (!qcmap_get_state_machine_running_status()) {
            ///检查是否更新路由
            qcmap_update_default_route();
        }

        if (!qcmap_get_state_machine_running_status()) {
            ///检查是否更新dns link
            qcmap_update_dns_link();
        }

        if (qcmap_get_state_machine_running_status()) {
            //释放资源
            qcmap_timer_check_state_machine_running_status();
        }
    }

    //检查信号并更新led
    qcmap_timer_check_led();
}

void qcmap_state_machine_running(void) {
    if (!qcmap_get_state_machine_running_status()) {
        return;
    }

    int status = qcmap_get_state_machine_status();

    PRINT_DEBUG("state machine status : [%d]\n", status);
    switch (status)
    {
        case STATE_MACHINE_NONE:
            PRINT_DEBUG("qcmap check at\n");
            if (AT_READY == qcmap_check_at()) {
                work_type = WORK_TYPE_NONE;

                //初始化ims设置
                //qcmap_init_ims(0);

                //同步注网类型到modem侧
                qcmap_sync_nettype_with_modem();

                //初始化lock
                if (qcmap_init_lock()) {
                    // 初始化 modem相关信息到文件
                    qcmap_init_modem_info();
                    qcmap_update_state_machine_status(STATE_MACHINE_AT_READY);
                }
 
                //若启动后lock未执行则需要执行一次cfun 0/1(即第一次启动必须执行一次cfun 0/1)
                qcmap_init_cfun(false);
            }
            break;
        case STATE_MACHINE_AT_READY:
            PRINT_DEBUG("qcmap check sim\n");
            if (SIM_READY == qcmap_check_sim()) {
                qcmap_update_state_machine_status(STATE_MACHINE_SIM_READY);
            }
            break;
        case STATE_MACHINE_SIM_READY:
            static int count = 0;
            PRINT_DEBUG("qcmap check register\n");
            if (NETWORK_REGISTERED == qcmap_check_registration()) {
                qcmap_update_state_machine_status(STATE_MACHINE_NETWORK_REGISTED);
            } else {
                if (SIM_READY != qcmap_check_sim()) {
                    count++;
                    if (count == 2) {
                        count = 0;
                        qcmap_update_state_machine_status(STATE_MACHINE_AT_READY);
                    }
                } else {
                    count = 0;
                }
            }
            break;
        case STATE_MACHINE_NETWORK_REGISTED:
            //初始化 client 实例
            qcmap_init_client();

            //初始化 mobileAP
            qcmap_init_mobileap();

            //初始化basic配置
            qcmap_init_basic();

            //初始化multi配置
            qcmap_init_multi();

            work_type = WORK_TYPE_ALL;

            write_buf_to_file(QCMAP_BOOT_FILE, "1", sizeof("1"));

            qcmap_update_state_machine_status(STATE_MACHINE_NONE);

            qcmap_disable_state_machine();
            break;
        default:
            break;
    }
}

int main(int argc, char** argv) {
    PRINT_DEBUG("------------------------>>> qcmap_mngr start ------------------------>>>\n");
    //初始化信号处理
    qcmap_init_signal();

    //检查处理临时文件
    qcmap_init_info_file();

    //初始化uci 相关配置
    qcmap_init_config();

    //初始化支持band列表
    qcmap_init_bandlist_config();

    //初始化流程标志
    qcmap_enable_state_machine();

    //主循环
    while (true) {
        // 第一次启动以及需要重新配置再拨号
        qcmap_state_machine_running();

        // 定时检查
        qcmap_timer_check();

        if (qcmap_get_state_machine_running_status()) {
            usleep(1 * 1000 * 1000);
        } else {
            usleep(2 * 1000 * 1000);
        }
    }

    write_buf_to_file(QCMAP_BOOT_FILE, "1", sizeof("1"));
    PRINT_DEBUG("------------------------>>> qcmap_mngr end ------------------------>>>\n");
    qcmap_release_led();
    return 0;
}
