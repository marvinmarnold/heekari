/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <pio.h>
#include <pio_ctrlr.h>
#include <timer.h>

/*============================================================================*
*  Local Header Files
*============================================================================*/

#include "switch_hw.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Setup PIOs
 *  PIO3    Buzzer
 */

#define DIMMER_PIO              (3)

#define PIO_BIT_MASK(pio)       (0x01 << (pio))

#define DIMMER_PIO_MASK         (PIO_BIT_MASK(BUZZER_PIO))

/*============================================================================*
 *  Public data
 *============================================================================*/

/* Dimmer hardware data instance */
SWITCH_DIMMER_DATA_T                   g_dimmer_data;

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/

 /*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      HrInitSwitchHardware 
 *
 *  DESCRIPTION
 *      This function is called to initialise switch hardware
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HrInitSwitchHardware(void)
{

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HrHwDataInit
 *
 *  DESCRIPTION
 *      This function initialises switch hardware data structure
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HrInitSwitchData(void)
{

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleSwitchPIOChangedEvent
 *
 *  DESCRIPTION
 *      This function handles Switch PIO Changed event
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HandleSwitchPIOChangedEvent(uint32 pio_changed)
{

}