#ifndef _QCMAP_WLAN_HMT_H_
#define _QCMAP_WLAN_HMT_H_

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

#define FILE_WLAN_CONFIG_HMT_SCRIPT       "/etc/data/wlanConfig_hmt.sh"

/** WLAN device0 */
#define WLAN_DEVICE0 "wlan0"

/** WLAN device1 */
#define WLAN_DEVICE1 "wlan1"

/** WLAN device2 */
#define WLAN_DEVICE2 "wlan2"

/** WLAN device3 */
#define WLAN_DEVICE3 "wlan3"


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
class QCMAP_WLAN_HMT:public QCMAP_WLAN_Common
{
public:

  QCMAP_WLAN_HMT(void);
  ~QCMAP_WLAN_HMT();

private:
  boolean FillSpecificWlanIfInfo(qcmap_msgr_wlan_if_info_t *wlan_info_cfg, int wlan_mode);
  virtual boolean DisAssociateClient_Device(const char* device, const char* mac_addr_str);

public:
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
#endif /* _QCMAP_WLAN_HMT_H_ */
