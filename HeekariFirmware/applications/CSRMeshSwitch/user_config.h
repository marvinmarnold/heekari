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

/* Enable battery model support */
/* #define ENABLE_BATTERY_MODEL */

/* Enable application debug logging on UART */
/* #define DEBUG_ENABLE */

#endif /* __USER_CONFIG_H__ */
