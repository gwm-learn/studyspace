#ifndef _TEST_H_
#define _TEST_H_

void init_test(void);
void init_test_node(void);

Netsnmp_Node_Handler handle_readTest;
Netsnmp_Node_Handler handle_writeTest;
Netsnmp_Node_Handler handle_SwitchMac;

#endif