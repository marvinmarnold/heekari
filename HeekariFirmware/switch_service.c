/******************************************************************************
 *
 *  FILE
 *      switch_service.c
 *
 *  DESCRIPTION
 *      This file defines routines for using Switch service.
 *
 ****************************************************************************/
/*============================================================================*
 *  Local Header Files
 *===========================================================================*/

#include "app_gatt.h"
#include "switch_service.h"

/*============================================================================*
 *  Private Data Types
 *===========================================================================*/

/* Switch service data type */
typedef struct
{
    /* Switch intensity in percent */
    uint8   intensity;

    /* Client configurate for Switch intensity characteristic */
    gatt_client_config switch_client_config;

    /* NVM Offset at which Switch data is stored */
    uint16 nvm_offset;

} SWITCH_DATA_T;

/*============================================================================*
 *  Private Data
 *===========================================================================*/

/* Switch service data instance */
static SWITCH_DATA_T g_switch_data;

/*============================================================================*
 *  Private Definitions
 *===========================================================================*/

/* Switch Level Full in percentage */
#define SWITCH_INTENSITY_ON                              (100)
#define SWITCH_INTENSITY_OFF                             (0)

/* Number of words of NVM memory used by Switch service */
#define SWITCH_SERVICE_NVM_MEMORY_WORDS              (1)

/* The offset of data being stored in NVM for Switch service. This offset is 
 * added to Switch service offset to NVM region (see g_switch_data.nvm_offset) 
 * to get the absolute offset at which this data is stored in NVM
 */
#define SWITCH_NVM_LEVEL_CLIENT_CONFIG_OFFSET        (0)

/*============================================================================*
 *   Private Function Prototypes
 *===========================================================================*/

/*static uint8 readSwitchIntensity(void);*/

/*============================================================================*
 *  Public Function Implementations
 *===========================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      SwitchDataInit
 *
 *  DESCRIPTION
 *      This function is used to initialise Switch service data 
 *      structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void SwitchDataInit(void)
{
    if(!AppIsDeviceBonded())
    {
        /* Initialise Switch level client configuration characterisitic
         * descriptor value only if device is not bonded
         */
        g_switch_data.switch_client_config = gatt_client_config_none;
    }

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      SwitchInitChipReset
 *
 *  DESCRIPTION
 *      This function is used to initialise Switch service data 
 *      structure at chip reset
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void SwitchInitChipReset(void)
{
    /* Initialise Switch level to 0 percent so that the Switch level 
     * notification (if configured) is sent when the value is read for 
     * the first time after power cycle.
     */
    g_switch_data.intensity = SWITCH_INTENSITY_OFF;
}