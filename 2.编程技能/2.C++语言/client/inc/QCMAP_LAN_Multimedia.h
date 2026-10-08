#ifndef _QCMAP_LAN_MULTIMEDIA_H_
#define _QCMAP_LAN_MULTIMEDIA_H_

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

              _ Q C M A P _ L A N _ M U L T I M E D I A . H

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/**
  @file QCMAP_LAN_Multimedia.h
  @brief QCMAP LAN Multimedia public function declarations.
 */

/*===========================================================================
NOTE: The @brief description above does not appear in the PDF.
      The description that displays in the PDF is maintained in the
      xxx_mainpage.dox file. Contact Tech Pubs for assistance.
===========================================================================*/

/*===========================================================================

FILE:  QCMAP_LAN_Multimedia.h

SERVICES:
   QCMAP LAN Multimedia Class

===========================================================================*/
/*===========================================================================

Copyright (c) 2023 Qualcomm Technologies, Inc.
All rights reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
===========================================================================*/
/*===========================================================================

                         EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  when       who        what, where, why
  --------   ---        -------------------------------------------------------
  06/06/23   yunhcao    Added MINIUPNPD API Support in Openwrt
===========================================================================*/

#include "QCMAP_LAN_Util.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

/** @addtogroup qcmap_lan_strings
@{ */

/** Enable miniupnpd macro */
#define ENABLE_MINIPUNPD "enable_miniupnpd"

/** Disable miniupnpd macro */
#define DISABLE_MINIPUNPD "disable_miniupnpd"

/** Set notify_interval macro */
#define SET_NOTIFY_INTERVAL "set_notify_interval"

/** Enable mdns macro */
#define ENABLE_MDNS "enable_mdns"

/** Disable miniupnpd macro */
#define DISABLE_MDNS "disable_mdns"


/** @} */ /* end_addtogroup qcmap_lan_strings */



/** @addtogroup qcmap_lan_file_path
@{ */

/** Shell script of miniupnpd */
#define MINIUPNPD_CONFIG_FILE "/etc/data/miniupnpd.sh"

#define ACTIVE_MINIUPNPD_CONFIG_PATH "/var/etc/miniupnpd.conf"

/** Shell script of mdns */
#define MDNS_CONFIG_FILE "/etc/data/mdns.sh"


/** @} */ /* end_addtogroup qcmap_lan_file_path */



//===================================================================
//              Class Definitions
//===================================================================

/**  @ingroup qcmap_lan_class */
class QCMAP_LAN_Multimedia
{

public:
/*===========================================================================
FUNCTION QCMAP_LAN_Multimedia()
===========================================================================*/
/** @ingroup qcmap_lan_class

   Constructor for the LAN Multimedia library QCMAP_LAN_Multimedia class.

   This constructor initializes the LAN Multimedia.

   @return
   None.
*/
/*=========================================================================*/
QCMAP_LAN_Multimedia(  void);

/*===========================================================================
  FUNCTION EnableUPNP
  ===========================================================================*/
/*!
  @brief
  Starts the UPNP daemon

  @return
  true  - on Success
  false - on Failure

  @note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean EnableUPNP(qmi_error_type_v01 *qmi_err_num);

/*===========================================================================
  FUNCTION DisableUPNP
  ===========================================================================*/
/*!
  @brief
  Stops the UPNP daemon

  @return
  true  - on Success
  false - on Failure

  @note
  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean DisableUPNP(qmi_error_type_v01 *qmi_err_num);

/*===========================================================================
  FUNCTION GetUPNPStatus
  ===========================================================================*/
/*!
  @brief
  Returns the status of UPNP

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean GetUPNPStatus
(
  qcmap_msgr_upnp_mode_enum_v01 *enable_UPNP,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION SetUPNPNotifyInterval
  ===========================================================================*/
/*!
  @brief
  Changes the UPnP notify interval

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean SetUPNPNotifyInterval
(
  int upnp_notify_int,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetUPnPNotifyInterval
  ===========================================================================*/
/*!
  @brief
  Returns the UPnP notify interval

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean GetUPnPNotifyInterval
(
  int *upnp_notify_int,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION Enable M-DNS
  ===========================================================================*/
/*!
  @brief
  Starts the M-DNS daemon

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean EnableMDNS
(
  qmi_error_type_v01 *qmi_err_num
);


/*===========================================================================
  FUNCTION DisableMDNS
  ===========================================================================*/
/*!
  @brief
  Stops the M-DNS daemon

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean DisableMDNS
(
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetMDNSStatus
  ===========================================================================*/
/*!
  @brief
  Returns the status of MDNS

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean GetMDNSStatus
(
  qcmap_msgr_mdns_mode_enum_v01  *mdns_state,
  qmi_error_type_v01 *qmi_err_num
);

};

#endif /* _QCMAP_LAN_MULTIMEDIA_H_ */
