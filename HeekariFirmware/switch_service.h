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

#endif /* __SWITCH_SERVICE_H__ */

