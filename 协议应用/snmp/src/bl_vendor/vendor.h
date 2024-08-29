#ifndef __VENDOR_H__
#define __VENDOR_H__

void init_vendor(void);

Netsnmp_Node_Handler handle_readTest;
Netsnmp_Node_Handler handle_writeTest;
Netsnmp_Node_Handler handle_SwitchMac;

#endif