/******************************************************************************
 *
 *  FILE
 *      switch_service.h
 *
 *  DESCRIPTION
 *      Header definitions for Switch service
 *
 *****************************************************************************/

#ifndef __SWITCH_SERVICE_H__
#define __SWITCH_SERVICE_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>

/* This function is used to initialise switch service data structure.*/
extern void SwitchDataInit(void);

/* This function is used to initialise switch service data structure at 
* chip reset
*/
extern void SwitchInitChipReset(void);

/* This function handles read operation on switch service attributes
 * maintained by the application
 */
// extern void SwitchHandleAccessRead(GATT_ACCESS_IND_T *p_ind);

/* This function is used to check if the handle belongs to the Battery 
 * service
 */
// extern bool SwitchCheckHandleRange(uint16 handle);

#endif /* __SWITCH_SERVICE_H__ */

