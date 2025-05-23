/*
 * Note: this file originally auto-generated by mib2c using
 *        $
*/

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <signal.h>
#include <string.h>
#include "ipc_client.h"
#include "util.h"
#include "vendor.h"
#include "system_info.h"
#include "mac_table.h"
#include "vlan_table.h"

/** Initializes the vendor module */
void init_vendor(void)
{
    snmp_log(LOG_DEBUG, "init_vendor init\n");

    init_system_info_node();
    init_mac_table_node();
    init_vlan_table_node();

    signal(SIGUSR1, stopSnmpdHandler);

    if ((cj_plat_shm_init() >= 0) && (cj_client_init() >= 0))
        snmp_ipc_startup();
}