/*******************************************************************************
 *  Copyright (C) Cambridge Silicon Radio Limited 2013
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 *  FILE
 *      gatt_service.h
 *
 *  DESCRIPTION
 *      Header definitions for GATT service
 *
 *****************************************************************************/

#ifndef __GATT_SERVICE_H__
#define __GATT_SERVICE_H__

/*=============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "user_config.h"

#ifdef ENABLE_GATT_OTA_SERVICE
/*=============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <bt_event_types.h>

/*=============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* This function reads the GATT service data from NVM. */
extern void GattReadDataFromNVM(uint16 *p_offset);

/* This function should be called when a bonded host connects. */
extern void GattOnConnection(void);

/* This function allows other modules to read whether the bonded device
 * has requested indications on the Service Changed characteristic. */
extern bool GattServiceChangedIndActive(void);

/* This function should be called when the device is switched into 
 * over-the-air update mode.
 */
extern void GattOnOtaSwitch(void);

/* This function handles READ operations on GATT service attributes. */
extern void GattHandleAccessRead(GATT_ACCESS_IND_T *p_ind);

/* This function handles READ operations on GATT service attributes. */
extern void GattHandleAccessWrite(GATT_ACCESS_IND_T *p_ind);

/* Determine whether a given handle is one handled by the GAP module. */
extern bool GattCheckHandleRange(uint16 handle);

#endif /* ENABLE_GATT_OTA_SERVICE */

#endif /* __GATT_SERVICE_H__ */
