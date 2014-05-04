/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <main.h>
#include <ls_app_if.h>
#include <gatt.h>
#include <nvm.h>

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "nvm_access.h"
#include "gap_service.h"

/*============================================================================*
*  Private Function Prototypes
*============================================================================*/
static void readPersistentStore(void);

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Maximum number of timers */
#define MAX_APP_TIMERS                 (6)

/*Number of IRKs that application can store */
#define MAX_NUMBER_IRK_STORED          (1)

/* Time after which HR Measurements will be transmitted to the connected 
 * host.
 */
#define HR_MEAS_TIME                   (7 * SECOND)

/* Magic value to check the sanity of NVM region used by the application */
#define NVM_SANITY_MAGIC               (0xAB04)

/* NVM offset for NVM sanity word */
#define NVM_OFFSET_SANITY_WORD         (0)

/* NVM offset for bonded flag */
#define NVM_OFFSET_BONDED_FLAG         (NVM_OFFSET_SANITY_WORD + 1)

/* NVM offset for bonded device bluetooth address */
#define NVM_OFFSET_BONDED_ADDR         (NVM_OFFSET_BONDED_FLAG + \
                                        sizeof(g_hr_data.bonded))

/* NVM offset for diversifier */
#define NVM_OFFSET_SM_DIV              (NVM_OFFSET_BONDED_ADDR + \
                                        sizeof(g_hr_data.bonded_bd_addr))

/* NVM offset for IRK */
#define NVM_OFFSET_SM_IRK              (NVM_OFFSET_SM_DIV + \
                                        sizeof(g_hr_data.diversifier))

/* Number of words of NVM used by application. Memory used by supported 
 * services is not taken into consideration here.
 */
#define NVM_MAX_APP_MEMORY_WORDS       (NVM_OFFSET_SM_IRK + \
                                        MAX_WORDS_IRK)

/*============================================================================*
 *  Public Data
 *============================================================================*/

/* HR Sensor application data instance */
HR_DATA_T g_hr_data;

/****************************************************************************
NAME
    AppPowerOnReset

DESCRIPTION
    This user application function is called just after a power-on reset
    (including after a firmware panic), or after a wakeup from Hibernate or
    Dormant sleep states.

    At the time this function is called, the last sleep state is not yet
    known.

    NOTE: this function should only contain code to be executed after a
    power-on reset or panic. Code that should also be executed after an
    HCI_RESET should instead be placed in the AppInit() function.

RETURNS

*/
void AppPowerOnReset(void)
{
}

/****************************************************************************
NAME    
    AppInit
    
DESCRIPTION
    This user application function is called after a power-on reset
    (including after a firmware panic), after a wakeup from Hibernate or
    Dormant sleep states, or after an HCI Reset has been requested.

    The last sleep state is provided to the application in the parameter.

    NOTE: In the case of a power-on reset, this function is called
    after app_power_on_reset().
    
RETURNS
    
*/
void AppInit(sleep_state last_sleep_state)
{
    /* Initialise GATT entity */
    GattInit();

    /* Install GATT Server support for the optional Write procedure
     * This is mandatory only if control point characteristic is supported. 
     */
    GattInstallServerWrite();

    /* Don't wakeup on UART RX line */
    SleepWakeOnUartRX(FALSE);

#ifdef NVM_TYPE_EEPROM
    /* Configure the NVM manager to use I2C EEPROM for NVM store */
    NvmConfigureI2cEeprom();
#elif NVM_TYPE_FLASH
    /* Configure the NVM Manager to use SPI flash for NVM store. */
    NvmConfigureSpiFlash();
#endif /* NVM_TYPE_EEPROM */

    Nvm_Disable();

    /* Initialize the gap data. Needs to be done before readPersistentStore */
    GapDataInit();

    /* Read persistent storage */
    readPersistentStore();
}


/****************************************************************************
NAME
    AppProcesSystemEvent

DESCRIPTION
    This user application function is called whenever a system event, such
    as a battery low notification, is received by the system.

RETURNS

*/
void AppProcessSystemEvent(sys_event_id id, void *data)
{
}

/****************************************************************************
NAME
    AppProcessLmEvent

DESCRIPTION
    This user application function is called whenever a LM-specific event is
    received by the system.

RETURNS
    Application should always return TRUE. Refer API Documentation under the module
    named "Application" for more information.
*/
bool AppProcessLmEvent(lm_event_code event_code, LM_EVENT_T *event_data)
{
    return TRUE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      readPersistentStore
 *
 *  DESCRIPTION
 *      This function is used to initialise and read NVM data
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

static void readPersistentStore(void)
{
    /* NVM offset for supported services */
    uint16 nvm_offset = NVM_MAX_APP_MEMORY_WORDS;
    uint16 nvm_sanity = 0xffff;
    bool nvm_start_fresh = FALSE;

    /* Read persistent storage to know if the device was last bonded 
     * to another device 
     */

    /* If the device was bonded, trigger fast undirected advertisements by 
     * setting the white list for bonded host. If the device was not bonded, 
     * trigger undirected advertisements for any host to connect.
     */

    Nvm_Read(&nvm_sanity, 
             sizeof(nvm_sanity), 
             NVM_OFFSET_SANITY_WORD);

    if(nvm_sanity == NVM_SANITY_MAGIC)
    {

        /* Read Bonded Flag from NVM */
        Nvm_Read((uint16*)&g_hr_data.bonded,
                  sizeof(g_hr_data.bonded), 
                  NVM_OFFSET_BONDED_FLAG);

        if(g_hr_data.bonded)
        {

            /* Bonded Host Typed BD Address will only be stored if bonded flag
             * is set to TRUE. Read last bonded device address.
             */
            Nvm_Read((uint16*)&g_hr_data.bonded_bd_addr, 
                       sizeof(TYPED_BD_ADDR_T),
                       NVM_OFFSET_BONDED_ADDR);

            /* If device is bonded and bonded address is resolvable then read 
             * the bonded device's IRK
             */
            if(GattIsAddressResolvableRandom(&g_hr_data.bonded_bd_addr))
            {
                Nvm_Read(g_hr_data.central_device_irk.irk, 
                         MAX_WORDS_IRK,
                         NVM_OFFSET_SM_IRK);
            }

        }
        else /* Case when we have only written NVM_SANITY_MAGIC to NVM but 
              * didn't get bonded to any host in the last powered session
              */
        {
            g_hr_data.bonded = FALSE;
        }

        /* Read the diversifier associated with the presently bonded/last 
         * bonded device.
         */
        Nvm_Read(&g_hr_data.diversifier, 
                 sizeof(g_hr_data.diversifier),
                 NVM_OFFSET_SM_DIV);

        /* If NVM in use, read device name and length from NVM */
        GapReadDataFromNVM(&nvm_offset);

    }
    else /* NVM Sanity check failed means either the device is being brought up 
          * for the first time or memory has got corrupted in which case 
          * discard the data and start fresh.
          */
    {

        nvm_start_fresh = TRUE;

        nvm_sanity = NVM_SANITY_MAGIC;

        /* Write NVM Sanity word to the NVM */
        Nvm_Write(&nvm_sanity, 
                  sizeof(nvm_sanity), 
                  NVM_OFFSET_SANITY_WORD);

        /* The device will not be bonded as it is coming up for the first 
         * time 
         */
        g_hr_data.bonded = FALSE;

        /* Write bonded status to NVM */
        Nvm_Write((uint16*)&g_hr_data.bonded, 
                  sizeof(g_hr_data.bonded), 
                  NVM_OFFSET_BONDED_FLAG);

        /* When the application is coming up for the first time after flashing 
         * the image to it, it will not have bonded to any device. So, no LTK 
         * will be associated with it. Hence, set the diversifier to 0.
         */
        g_hr_data.diversifier = 0;

        /* Write the same to NVM. */
        Nvm_Write(&g_hr_data.diversifier, 
                  sizeof(g_hr_data.diversifier),
                  NVM_OFFSET_SM_DIV);

        /* If fresh NVM, write device name and length to NVM for the 
         * first time.
         */
        GapInitWriteDataToNVM(&nvm_offset);

    }

    /* Read Heart Rate service data from NVM if the devices are bonded and  
     * update the offset with the number of words of NVM required by 
     * this service
     */
    HeartRateReadDataFromNVM(nvm_start_fresh, &nvm_offset);

    /* Read Battery service data from NVM if the devices are bonded and  
     * update the offset with the number of word of NVM required by 
     * this service
     */
    BatteryReadDataFromNVM(&nvm_offset);

}