/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2014
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 *  FILE
 *      battery_hw.h
 *
 *  DESCRIPTION
 *      Header definitions for battery access routines
 *
 *****************************************************************************/

#ifndef __BATTERY_HW_H__
#define __BATTERY_HW_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>
#include <bt_event_types.h>

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* This function reads the battery level */
extern uint8 ReadBatteryLevel(void);

#endif /* __BATTERY_HW_H__ */

