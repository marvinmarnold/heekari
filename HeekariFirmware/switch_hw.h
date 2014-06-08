#ifndef __SWITCH_HW_H__
#define __SWITCH_HW_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>
#include <bluetooth.h>
#include <timer.h>

 /*============================================================================*
 *  Public data type
 *============================================================================*/

typedef struct
{
    uint16                    last_dimmer_var;
}SWITCH_DIMMER_DATA_T;

/*============================================================================*
 *  Public Data Declarations
 *============================================================================*/

/* Dimmer hardware data instance */
extern SWITCH_DIMMER_DATA_T                   g_dimmer_data;

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* This function is called to initialise Switch hardware */
extern void HrInitSwitchHardware(void);

/* This function initialises Switch hardware data structure */
extern void HrInitSwitchData(void);

/* This function handles PIO Changed event */
extern void HandleSwitchPIOChangedEvent(uint32 pio_changed);

#endif /* __SWITCH_HW_H__ */