#ifndef _QCMAP_WLAN_WKK_H_
#define _QCMAP_WLAN_WKK_H_

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                   _ Q C M A P _ W L A N _ H M T . H

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/**
  @file QCMAP_WLAN_HMT.h
  @brief QCMAP WLAN Common public function declarations.
         As a note, the client app should not perform time intensive tasks in
         callback context. It should be minimal handling and majority of
         tasks should be handled in client thread context
 */

/*===========================================================================
NOTE: The @brief description above does not appear in the PDF.
      The description that displays in the PDF is maintained in the
      xxx_mainpage.dox file. Contact Tech Pubs for assistance.
===========================================================================*/

/*===========================================================================

FILE:  QCMAP_WLAN_HMT.h

SERVICES:
   QCMAP LAN Client Class

===========================================================================*/
/*===========================================================================

Copyright (c) 2022-2023 Qualcomm Technologies, Inc.
All rights reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
===========================================================================*/
/*===========================================================================

                         EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  when       who        what, where, why
  --------   ---        -------------------------------------------------------

===========================================================================*/

/*===========================================================================

Define Structure:
  1. #include
  2. constants
  3. enum constants
  4. strings constants
  5. strings for shell command
  6. strings for path
  7. typedef struct
  8. macro
  9. class definitions
  10. function definitions

===========================================================================*/

#include "QCMAP_WLAN_Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/** @addtogroup 2.constants
@{ */


/** @} */ /* end_addtogroup 2. constants */
/*===========================================================================*/


/*===========================================================================*/
/** @addtogroup 3.enum constants
@{ */


/** @} */ /* end_addtogroup 3.enum constants */
/*===========================================================================*/


/*===========================================================================*/
/** @addtogroup 4.strings constants
@{ */


/** @} */ /* end_addtogroup 4.strings constants */
/*===========================================================================*/



/*===========================================================================*/
/** @addtogroup 5.strings for shell command
@{ */

/** @} */ /* end_addtogroup 5.strings for shell command */
/*===========================================================================*/



/*===========================================================================*/
/** @addtogroup 6.strings for path
@{ */



/** @} */ /* end_addtogroup 6.strings for path */
/*===========================================================================*/


/*===========================================================================*/
/** @addtogroup 7.typedef struct
@{ */


/** @} */ /* end_addtogroup 7.typedef struct */
/*===========================================================================*/


/*===========================================================================*/
/** @addtogroup 8.macro
@{ */

/** @} */ /* end_addtogroup 8.macro */
/*===========================================================================*/


//===================================================================
//              Class Definitions
//===================================================================

class QCMAP_WLAN_WKK:public QCMAP_WLAN_Common
{
public:
  QCMAP_WLAN_WKK(void);
  ~QCMAP_WLAN_WKK();
  boolean
  GetWLANConfigEx
  (
  qcmap_wlan_ex_config *wlan_config,
  qmi_error_type_v01 *qmi_err_num
  );

private:
  boolean FillSpecificWlanIfInfo(qcmap_msgr_wlan_if_info_t *wlan_info_cfg, int wlan_mode);
  int GetWlanIfaceName(char *interface);
  virtual boolean DisAssociateClient_Device(const char* device, const char* mac_addr_str);

public:
/*=====================================================================
  FUNCTION DisAssociateClient
======================================================================*/
/*!
@brief
  - Disassocaite WiFi client with mac addr
@return
  true - Success
  false - Failure

@note
  - Disassocaite WiFi client with mac addr by hostapd_cli disaasocaite command

- Dependencies
  - None

- Side Effects
  - None
*/
/*=========================================================================*/
boolean DisAssociateClient(const char* mac_addr_str);

/*=====================================================================
  FUNCTION RestartTetheredWLANClient
======================================================================*/
/*!
@brief
  - Restart tethered WLAN Client

@return
  true - Success
  false - Failure

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean RestartTetheredWLANClient();

boolean ResetWLANatBootup();

boolean
IsWifiUp
(
   qmi_error_type_v01 *qmi_err_num
);
};
#endif /* _QCMAP_WLAN_WKK_H_ */
