/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2014
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 * FILE
 *      user_config.h
 *
 * DESCRIPTION
 *      This file contains definitions which will enable customization of the
 *      application.
 *
 ******************************************************************************/

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

/*=============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Macro to enable GATT OTA SERVICE */
/* Over-the-Air Update is supported only on EEPROM */
#ifdef NVM_TYPE_EEPROM

#define ENABLE_GATT_OTA_SERVICE

#endif /* NVM_TYPE_EEPROM */

/* Select target hardware. By default the application builds for the controls
 * on the IOT Demo Board. Uncomment the GUNILAMP to build the application for
 * Gunilamp
 */
/* #define GUNILAMP */

/* Enable support for Colour Temperature */
/* #define COLOUR_TEMP_ENABLED */

/* Enable application debug logging on UART */
/* #define DEBUG_ENABLE */

/* SW2 button on IOT board will be used for Association removal
 * and restart when USE_ASSOCIATION_REMOVAL_KEY is defined.
 */
#if (!defined(GUNILAMP) && !defined(DEBUG_ENABLE))
#define USE_ASSOCIATION_REMOVAL_KEY
#endif

/* Enable battery model support */
/* #define ENABLE_BATTERY_MODEL */

/* Enable firmware model support */
/* #define ENABLE_FIRMWARE_MODEL */


#endif /* __USER_CONFIG_H__ */

