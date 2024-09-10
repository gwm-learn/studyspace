#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <string.h>
#include "ipc_client.h"
#include "util.h"
#include "test.h"

void init_test(void)
{
    // do nothing, init in init_test_node
}

void init_test_node(void)
{
    const oid readTest_oid[] = { 1,3,6,1,4,1,36000,1,1 };
    const oid writeTest_oid[] = { 1,3,6,1,4,1,36000,1,2 };
    const oid SwitchMac_oid[] = { 1,3,6,1,4,1,36000,1,3 };

    snmp_log(LOG_DEBUG, "init_test_node init\n");

    netsnmp_register_scalar(netsnmp_create_handler_registration("readTest", handle_readTest, readTest_oid, OID_LENGTH(readTest_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("writeTest", handle_writeTest, writeTest_oid, OID_LENGTH(writeTest_oid), HANDLER_CAN_RWRITE));
    netsnmp_register_scalar(netsnmp_create_handler_registration("SwitchMac", handle_SwitchMac, SwitchMac_oid, OID_LENGTH(SwitchMac_oid), HANDLER_CAN_RWRITE));
}

int handle_readTest(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    int value = 36000;
    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_readTest MODE_GET\n");
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &value, sizeof(value));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_readTest\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_writeTest(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    int ret;
    static char buf[1024] = "writeTest";

    switch(reqinfo->mode) {
        case MODE_SET_UNDO:
        case MODE_SET_FREE:
        case MODE_SET_COMMIT:
        case MODE_SET_RESERVE2:
            break;
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_writeTest MODE_GET\n");
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, buf, strlen(buf));
            break;
        case MODE_SET_RESERVE1:
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_OCTET_STR);
            if (ret != SNMP_ERR_NOERROR)
                netsnmp_set_request_error(reqinfo, requests, ret);
            break;
        case MODE_SET_ACTION:
            snmp_log(LOG_DEBUG, "handle_writeTest MODE_SET_ACTION\n");
            if(requests->requestvb->val_len > 0 && requests->requestvb->val_len < 1023) {
                memcpy(buf, requests->requestvb->val.string, requests->requestvb->val_len);
                buf[requests->requestvb->val_len] = '\0';
            }else{
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_BADVALUE);
            }
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_writeTest\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }
    return SNMP_ERR_NOERROR;
}

int handle_SwitchMac(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    int ret;
    char mac[16] = {0};

    cj_ctl_msg_t msg;
    cj_ctl_msg_t recv_msg;

    static netsnmp_variable_list var_obj;
    static netsnmp_variable_list var_trap;

    const oid objid_snmptrap[] = { 1,3,6,1,6,3,1,1,4,1,0 };     /* snmpTrapOID.0 */
    const oid demo_trap[] = { 1,3,6,1,4,1,2021,13,990 };  /*demo-trap */
    const oid SwitchMac_oid[] = { 1,3,6,1,4,1,36000,1,3 };

    switch(reqinfo->mode) {
        case MODE_SET_UNDO:
        case MODE_SET_FREE:
        case MODE_SET_RESERVE2:
            break;
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_SwitchMac MODE_GET\n");
            msg.msg_type  = CJ_MSG_TYPE_SNMP;
            msg.protorl   = CJ_CLIENT_SNMP;
            msg.sub_type.snmp_type = CJ_MSG_SNMP_SUB_TYPE_GET_MAC;
            if (snmp_get_msg(&msg, sizeof(msg), (char *)&recv_msg, sizeof(cj_ctl_msg_t)) < 0){
                snmp_log(LOG_DEBUG, "handle_SwitchMac snmp_get_msg fail\n");
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
                break;
            }
            mac_array_str(recv_msg.msg.mac, mac);
            snmp_log(LOG_DEBUG, "handle_SwitchMac mac str:%s hex:%02x%02x%02x%02x%02x%02x\n", mac, \
            recv_msg.msg.mac[0],recv_msg.msg.mac[1],recv_msg.msg.mac[2],recv_msg.msg.mac[3],recv_msg.msg.mac[4],recv_msg.msg.mac[5]);
            snmp_set_var_typed_value( requests->requestvb, ASN_OCTET_STR, mac, strlen(mac));
            break;
        case MODE_SET_RESERVE1:
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_OCTET_STR);
            if (ret != SNMP_ERR_NOERROR)
                netsnmp_set_request_error(reqinfo, requests, ret);
            break;
        case MODE_SET_ACTION:
            snmp_log(LOG_DEBUG, "handle_SwitchMac MODE_SET_ACTION\n");
            snmp_log(LOG_DEBUG, "handle_SwitchMac requests->requestvb->val_len is %d\n", requests->requestvb->val_len);
            if(requests->requestvb->val_len == 12 && mac_str_check(requests->requestvb->val.string)){
                msg.msg_type  = CJ_MSG_TYPE_SNMP;
                msg.protorl   = CJ_CLIENT_SNMP;
                msg.sub_type.snmp_type = CJ_MSG_SNMP_SUB_TYPE_SET_MAC;
                mac_str_array(requests->requestvb->val.string, msg.msg.mac);
                if (snmp_set_msg(&msg, sizeof(msg)) < 0){
                    snmp_log(LOG_DEBUG, "handle_SwitchMac snmp_set_msg fail\n");
                    netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
                    break;
                }
                snmp_log(LOG_DEBUG, "handle_SwitchMac mac str:%s hex:%02x%02x%02x%02x%02x%02x\n", requests->requestvb->val.string, \
                msg.msg.mac[0],msg.msg.mac[1],msg.msg.mac[2],msg.msg.mac[3],msg.msg.mac[4],msg.msg.mac[5]);
            }else{
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_BADVALUE);
            }
            break;
        case MODE_SET_COMMIT:
            /*
             * send trap test
            */
            snmp_log(LOG_DEBUG, "handle_SwitchMac MODE_SET_COMMIT send trap test\n");

            var_trap.next_variable = &var_obj;
            var_trap.name = objid_snmptrap;
            var_trap.name_length = sizeof(objid_snmptrap) / sizeof(oid);
            var_trap.type = ASN_OBJECT_ID;
            var_trap.val.objid = demo_trap;
            var_trap.val_len = sizeof(demo_trap);

            var_obj.next_variable = NULL;
            var_obj.name = SwitchMac_oid;
            var_obj.name_length = sizeof(SwitchMac_oid) / sizeof(oid);
            var_obj.type = ASN_OCTET_STR;
            var_obj.val.string = (u_char *)requests->requestvb->val.string;
            var_obj.val_len = requests->requestvb->val_len;

            snmp_log(LOG_DEBUG, "handle_SwitchMac sending the v2 trap\n");
            send_v2trap(&var_trap);
            snmp_log(LOG_DEBUG, "handle_SwitchMac v2 trap sent\n");
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_writeTest\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }
    return SNMP_ERR_NOERROR;
}