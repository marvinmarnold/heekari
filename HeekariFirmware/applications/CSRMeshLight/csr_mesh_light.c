/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2014
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 *  FILE
 *      csr_mesh_light.c
 *
 *  DESCRIPTION
 *      This file implements the CSR Mesh light application.
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <main.h>
#include <ls_app_if.h>
#include <gatt.h>
#include <timer.h>
#include <uart.h>
#include <pio.h>
#include <nvm.h>
#include <security.h>
#include <gatt_prim.h>
#include <mem.h>
#include <panic.h>
#include <config_store.h>
#include <random.h>
#include <buf_utils.h>

/*============================================================================*
 *  CSR Mesh Header Files
 *============================================================================*/
#include <csr_mesh.h>
#include <attention_model.h>
#include <light_model.h>
#include <power_model.h>
#include <bearer_model.h>
#include <ping_model.h>
#include <debug.h>

#ifdef ENABLE_FIRMWARE_MODEL
#include <firmware_model.h>
#endif
#ifdef ENABLE_BATTERY_MODEL
#include <battery_model.h>
#endif

/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"
#include "nvm_access.h"
#include "csr_mesh_light.h"
#include "app_debug.h"
#include "app_gatt.h"
#include "csr_mesh_light_hw.h"
#include "gap_service.h"
#include "app_gatt_db.h"
#include "mesh_control_service.h"
#include "csr_mesh_light_gatt.h"

#ifdef ENABLE_GATT_OTA_SERVICE
#include "csr_ota.h"
#include "csr_ota_service.h"
#include "gatt_service.h"
#endif /* ENABLE_GATT_OTA_SERVICE */

#ifdef USE_ASSOCIATION_REMOVAL_KEY
#include "iot_hw.h"
#endif /* USE_ASSOCIATION_REMOVAL_KEY */
#include "battery_hw.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Application version */
#define APP_MAJOR_VERSION               1
#define APP_MINOR_VERSION               0

/* Association Removal Button Press Duration */
#define LONG_KEYPRESS_TIME             (2 * SECOND)

/* CSRmesh Device UUID size */
#define DEVICE_UUID_SIZE_WORDS          8

/* CSRmesh Authorization Code Size in Words */
#define DEVICE_AUTHCODE_SIZE_IN_WORDS  (4)

/* Number of model groups supported */
#define MAX_MODEL_GROUPS               (4)

/* CS Key for user flags */
#define CSKEY_INDEX_USER_FLAGS         (CSR_MESH_CS_USERKEY_INDEX_ADV_TIME + 1)

/* Used for generating Random UUID */
#define RANDOM_UUID_ENABLE_MASK        (0x0001)

/* Used for permanently Enabling/Disabling Relay */
#define RELAY_ENABLE_MASK              (0x0002)

/* Used for permanently Enabling/Disabling Bridge */
#define BRIDGE_ENABLE_MASK             (0x0004)

/* NVM Data Write defer Duration */
#define NVM_WRITE_DEFER_DURATION       (5 * SECOND)

/* Maximum number of timers */
#define MAX_APP_TIMERS                 (8 + MAX_CSR_MESH_TIMERS)

/* Advertisement Timer for sending device identification */
#define DEVICE_ID_ADVERT_TIME          (5 * SECOND)

/* Slave device is not allowed to transmit another Connection Parameter
 * Update request till time TGAP(conn_param_timeout). Refer to section 9.3.9.2,
 * Vol 3, Part C of the Core 4.0 BT spec. The application should retry the
 * 'connection parameter update' procedure after time TGAP(conn_param_timeout)
 * which is 30 seconds.
 */
#define GAP_CONN_PARAM_TIMEOUT         (30 * SECOND)


/* TGAP(conn_pause_peripheral) defined in Core Specification Addendum 3 Revision
 * 2. A Peripheral device should not perform a Connection Parameter Update proc-
 * -edure within TGAP(conn_pause_peripheral) after establishing a connection.
 */
#define TGAP_CPP_PERIOD                (1 * SECOND)

/* TGAP(conn_pause_central) defined in Core Specification Addendum 3 Revision 2.
 * After the Peripheral device has no further pending actions to perform and the
 * Central device has not initiated any other actions within TGAP(conn_pause_ce-
 * -ntral), then the Peripheral device may perform a Connection Parameter Update
 * procedure.
 */
#define TGAP_CPC_PERIOD                (1 * SECOND)

/* Magic value to check the sanity of NVM region used by the application */
#define NVM_SANITY_MAGIC               (0xAB18)

/*Number of IRKs that application can store */
#define MAX_NUMBER_IRK_STORED          (1)

/* Application NVM version. This version is used to keep the compatibility of
 * NVM contents with the application version. This value needs to be modified
 * only if the new version of the application has a different NVM structure
 * than the previous version (such as number of groups supported) that can
 * shift the offsets of the currently stored parameters.
 * If the application NVM version has changed, it could still read the values
 * from the old Offsets and store into new offsets.
 * This application currently erases all the NVM values if the NVM version has
 * changed.
 */
#define APP_NVM_VERSION                2

/* NVM offset for the application NVM version */
#define NVM_OFFSET_APP_NVM_VERSION     (0)

/* NVM offset for NVM sanity word */
#define NVM_OFFSET_SANITY_WORD         (NVM_OFFSET_APP_NVM_VERSION + 1)

/* NVM offset for bonded flag */
#define NVM_OFFSET_BONDED_FLAG         (NVM_OFFSET_SANITY_WORD + 1)

/* NVM offset for bonded device bluetooth address */
#define NVM_OFFSET_BONDED_ADDR         (NVM_OFFSET_BONDED_FLAG + \
                                        sizeof(bool))

/* NVM offset for diversifier */
#define NVM_OFFSET_SM_DIV              (NVM_OFFSET_BONDED_ADDR + \
                                        sizeof(TYPED_BD_ADDR_T))

/* NVM offset for IRK */
#define NVM_OFFSET_SM_IRK              (NVM_OFFSET_SM_DIV + \
                                        sizeof(uint16))

/* Number of words of NVM used by application. Memory used by supported
 * services is not taken into consideration here.
 */
#define NVM_OFFSET_ASSOCIATION_STATE   (NVM_OFFSET_SM_IRK + \
                                        MAX_WORDS_IRK)

/* NVM offset for CSRmesh device UUID */
#define NVM_OFFSET_DEVICE_UUID         (NVM_OFFSET_ASSOCIATION_STATE + 1)

/* NVM Offset for Authorization Code */
#define NVM_OFFSET_DEVICE_AUTHCODE     (NVM_OFFSET_DEVICE_UUID + \
                                        DEVICE_UUID_SIZE_WORDS)

#define NVM_OFFSET_LIGHT_MODEL_GROUPS  (NVM_OFFSET_DEVICE_AUTHCODE + \
                                        DEVICE_AUTHCODE_SIZE_IN_WORDS)

#define NVM_OFFSET_POWER_MODEL_GROUPS  (NVM_OFFSET_LIGHT_MODEL_GROUPS + \
                                        sizeof(light_model_groups))

#define NVM_OFFSET_ATTENTION_MODEL_GROUPS \
                                       (NVM_OFFSET_POWER_MODEL_GROUPS+ \
                                        sizeof(power_model_groups))

/* NVM Offset for RGB data */
#define NVM_RGB_DATA_OFFSET            (NVM_OFFSET_ATTENTION_MODEL_GROUPS + \
                                        sizeof(attention_model_groups))

/* Size of RGB Data in Words */
#define NVM_RGB_DATA_SIZE              (2)

/* NVM Offset for Bearer Model Data. */
#define NVM_BEARER_DATA_OFFSET         (NVM_RGB_DATA_OFFSET + \
                                            NVM_RGB_DATA_SIZE)

/* NVM offset Application Data */
#define NVM_MAX_APP_MEMORY_WORDS       (NVM_BEARER_DATA_OFFSET + \
                                            sizeof(BEARER_MODEL_STATE_DATA_T))

/*============================================================================*
 *  Private Data
 *============================================================================*/
/* The UUID is in big endian but the uint16 data type holds the data in little
 * endian so swap every 2 bytes
 */

/* Default CSRmesh device UUID:
 * 0xe411-2cb1-4250-e311-1896-3fce-0855-d9ac
 */
uint16 light_uuid[DEVICE_UUID_SIZE_WORDS] =
          {0x11e4,0xb12c,0x5042,0x11e3, 0x9618,0xce3f,0x5508,0xacd9};

/* CSRmesh Device Authorization Code */
uint16 lightAuthCode[DEVICE_AUTHCODE_SIZE_IN_WORDS] =
                    {0x3412, 0x7856, 0x3412, 0x7856};

/* CSRmesh light application specific data */
CSRMESH_LIGHT_DATA_T g_lightapp_data;

/* Declare space for application timers. */
static uint16 app_timers[SIZEOF_APP_TIMER * MAX_APP_TIMERS];

/* Declare space for Model Groups */
static uint16 light_model_groups[MAX_MODEL_GROUPS];
static uint16 attention_model_groups[MAX_MODEL_GROUPS];
static uint16 power_model_groups[MAX_MODEL_GROUPS];

#ifdef USE_ASSOCIATION_REMOVAL_KEY
/* Association Button Press Timer */
static timer_id long_keypress_tid;
#endif /* USE_ASSOCIATION_REMOVAL_KEY */

/* Attention timer id */
static timer_id attn_tid = TIMER_INVALID;

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/
/* UART Receive callback */
#ifdef DEBUG_ENABLE
static uint16 UartDataRxCallback ( void* p_data, uint16 data_count,
        uint16* p_num_additional_words );
#endif /* DEBUG_ENABLE */

#ifdef USE_ASSOCIATION_REMOVAL_KEY
static void handlePIOEvent(pio_changed_data *data);
#endif /* USE_ASSOCIATION_REMOVAL_KEY */

/* Advert time out handler */
static void smLightDeviceIdAdvertTimeoutHandler(timer_id tid);

/* This function reads the persistent store. */
static void readPersistentStore(void);

/*============================================================================*
 *  Private Function Definitions
 *============================================================================*/

/*-----------------------------------------------------------------------------*
 *  NAME
 *      smLightDeviceIdAdvertTimeoutHandler
 *
 *  DESCRIPTION
 *      This function handles the Device ID advertise timer event.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void smLightDeviceIdAdvertTimeoutHandler(timer_id tid)
{
    if(tid == g_lightapp_data.mesh_device_id_advert_tid)
    {
        if(g_lightapp_data.assoc_state == app_state_not_associated)
        {
            if (g_lightapp_data.state != app_state_fast_advertising)
            {
                /* Send the device ID advertisements */
                CsrMeshAssociateToANetwork();
            }

            g_lightapp_data.mesh_device_id_advert_tid = TimerCreate(
                                         DEVICE_ID_ADVERT_TIME, TRUE,
                                         smLightDeviceIdAdvertTimeoutHandler);
        }
        else
        {
            /* Device is now associated so no need to start the timer again */
            g_lightapp_data.mesh_device_id_advert_tid = TIMER_INVALID;
        }
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      smLightDataNVMWriteTimerHandler
 *
 *  DESCRIPTION
 *      This function handles NVM Write Timer time-out.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void smLightDataNVMWriteTimerHandler(timer_id tid)
{
    uint32 rd_data = 0;
    uint32 wr_data = 0;

    if (tid == g_lightapp_data.nvm_tid)
    {
        g_lightapp_data.nvm_tid = TIMER_INVALID;

        /* Read RGB and Power Data from NVM */
        NvmRead((uint16 *)&rd_data, sizeof(uint32),
                 NVM_RGB_DATA_OFFSET);

        /* Pack Data for writing to NVM */
        wr_data = ((uint32) g_lightapp_data.power.power_state << 24) |
                  ((uint32) g_lightapp_data.light_state.blue  << 16) |
                  ((uint32) g_lightapp_data.light_state.green <<  8) |
                  g_lightapp_data.light_state.red;

        /* If data on NVM is not equal to current state, write current state
         * to NVM.
         */
        if (rd_data != wr_data)
        {
            NvmWrite((uint16 *)&wr_data, sizeof(uint32),NVM_RGB_DATA_OFFSET);
        }
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      attnTimerHandler
 *
 *  DESCRIPTION
 *      This function handles Attention time-out.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void attnTimerHandler(timer_id tid)
{
    if (attn_tid == tid)
    {
        attn_tid = TIMER_INVALID;

        /* Restore Light State */
        LightHardwareSetColor(g_lightapp_data.light_state.red,
                              g_lightapp_data.light_state.green,
                              g_lightapp_data.light_state.blue);

        LightHardwarePowerControl(g_lightapp_data.power.power_state);
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      smAppInitiateAssociation
 *
 *  DESCRIPTION
 *      This function starts timer to send CSRmesh Association Messages
 *      and also gives visual indication that light is not associated.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void smAppInitiateAssociation(void)
{
    /* Blink light to indicate that it is not associated */
    LightHardwareSetBlink(0, 0, 127, 32, 32);

    /* Start a timer to send Device ID messages periodically to get
     * associated to a network
     */
    g_lightapp_data.mesh_device_id_advert_tid =
                    TimerCreate(DEVICE_ID_ADVERT_TIME,
                                TRUE,
                                smLightDeviceIdAdvertTimeoutHandler);
}


/*-----------------------------------------------------------------------------*
 *  NAME
 *      togglePowerState
 *
 *  DESCRIPTION
 *      This function toggles the power state.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void togglePowerState(void)
{
    POWER_STATE_T curr_state = g_lightapp_data.power.power_state;

    switch (curr_state)
    {
        case POWER_STATE_ON:
            g_lightapp_data.power.power_state = POWER_STATE_OFF;
        break;

        case POWER_STATE_OFF:
            g_lightapp_data.power.power_state = POWER_STATE_ON;
        break;

        case POWER_STATE_ON_FROM_STANDBY:
            g_lightapp_data.power.power_state = POWER_STATE_STANDBY;
        break;

        case POWER_STATE_STANDBY:
            g_lightapp_data.power.power_state = POWER_STATE_ON_FROM_STANDBY;
        break;

        default:
        break;
    }
}


#ifdef USE_ASSOCIATION_REMOVAL_KEY
/*-----------------------------------------------------------------------------*
 *  NAME
 *      longKeyPressTimeoutHandler
 *
 *  DESCRIPTION
 *      This function handles the long key press timer event.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void longKeyPressTimeoutHandler(timer_id tid)
{
    if (long_keypress_tid == tid)
    {
        long_keypress_tid = TIMER_INVALID;
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      handlePIOEvent
 *
 *  DESCRIPTION
 *      This function handles the PIO Events.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
void handlePIOEvent(pio_changed_data *data)
{
    /* If Switch-2 related event, then process further. Otherwise ignore */
    if (data->pio_cause & SW2_MASK)
    {
        /* Button Pressed */
        if (!(data->pio_state & SW2_MASK))
        {
            TimerDelete(long_keypress_tid);

            long_keypress_tid = TimerCreate(LONG_KEYPRESS_TIME, TRUE,
                                            longKeyPressTimeoutHandler);
        }
        else /* Button Released */
        {
            /* Button released after long press */
            if (TIMER_INVALID == long_keypress_tid)
            {
                if (app_state_not_associated != g_lightapp_data.assoc_state)
                {
                    /* Reset Association Information */
                    CsrMeshReset();

                    /* Set state to un-associated */
                    g_lightapp_data.assoc_state = app_state_not_associated;

                    /* Write association state to NVM */
                    NvmWrite((uint16 *)&g_lightapp_data.assoc_state,
                             sizeof(g_lightapp_data.assoc_state),
                             NVM_OFFSET_ASSOCIATION_STATE);
                }
            }
            else /* Button released after a short press */
            {
                if (app_state_not_associated == g_lightapp_data.assoc_state)
                {
                    /* Delete Long Key Press Timer */
                    TimerDelete(long_keypress_tid);

                    /* Start Association to Csr Mesh */
                    smAppInitiateAssociation();
                }
            }
        }
    }
}
#endif /* USE_ASSOCIATION_REMOVAL_KEY */

/*----------------------------------------------------------------------------*
 *  NAME
 *      requestConnParamUpdate
 *
 *  DESCRIPTION
 *      This function is used to send L2CAP_CONNECTION_PARAMETER_UPDATE_REQUEST
 *      to the remote device when an earlier sent request had failed.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void requestConnParamUpdate(timer_id tid)
{
    /* Application specific preferred parameters */
    ble_con_params app_pref_conn_param;

    if(g_lightapp_data.gatt_data.con_param_update_tid == tid)
    {

        g_lightapp_data.gatt_data.con_param_update_tid = TIMER_INVALID;
        g_lightapp_data.gatt_data.cpu_timer_value = 0;

        /*Handling signal as per current state */
        switch(g_lightapp_data.state)
        {

            case app_state_connected:
            {
                /* Increment the count for Connection Parameter Update
                 * requests
                 */
                ++ g_lightapp_data.gatt_data.num_conn_update_req;

                /* If it is first or second request, preferred connection
                 * parameters should be request
                 */
                if(g_lightapp_data.gatt_data.num_conn_update_req == 1 ||
                   g_lightapp_data.gatt_data.num_conn_update_req == 2)
                {
                    app_pref_conn_param.con_max_interval =
                                                PREFERRED_MAX_CON_INTERVAL;
                    app_pref_conn_param.con_min_interval =
                                                PREFERRED_MIN_CON_INTERVAL;
                    app_pref_conn_param.con_slave_latency =
                                                PREFERRED_SLAVE_LATENCY;
                    app_pref_conn_param.con_super_timeout =
                                                PREFERRED_SUPERVISION_TIMEOUT;
                }
                /* If it is 3rd or 4th request, APPLE compliant parameters
                 * should be requested.
                 */
                else if(g_lightapp_data.gatt_data.num_conn_update_req == 3 ||
                        g_lightapp_data.gatt_data.num_conn_update_req == 4)
                {
                    app_pref_conn_param.con_max_interval =
                                                APPLE_MAX_CON_INTERVAL;
                    app_pref_conn_param.con_min_interval =
                                                APPLE_MIN_CON_INTERVAL;
                    app_pref_conn_param.con_slave_latency =
                                                APPLE_SLAVE_LATENCY;
                    app_pref_conn_param.con_super_timeout =
                                                APPLE_SUPERVISION_TIMEOUT;
                }

                /* Send Connection Parameter Update request using application
                 * specific preferred connection parameters
                 */

                if(LsConnectionParamUpdateReq(
                                 &g_lightapp_data.gatt_data.con_bd_addr,
                                 &app_pref_conn_param) != ls_err_none)
                {
                    ReportPanic(app_panic_con_param_update);
                }
            }
            break;

            default:
                /* Ignore in other states */
            break;
        }

    } /* Else ignore the timer */

}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      handleGapCppTimerExpiry
 *
 *  DESCRIPTION
 *      This function handles the expiry of TGAP(conn_pause_peripheral) timer.
 *      It starts the TGAP(conn_pause_central) timer, during which, if no activ-
 *      -ity is detected from the central device, a connection parameter update
 *      request is sent.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void handleGapCppTimerExpiry(timer_id tid)
{
    if(g_lightapp_data.gatt_data.con_param_update_tid == tid)
    {
        g_lightapp_data.gatt_data.con_param_update_tid =
                           TimerCreate(TGAP_CPC_PERIOD, TRUE,
                                       requestConnParamUpdate);
        g_lightapp_data.gatt_data.cpu_timer_value = TGAP_CPC_PERIOD;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      smLightDataInit
 *
 *  DESCRIPTION
 *      This function is called to initialise Csr mesh light application
 *      data structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void smLightDataInit(void)
{
    /* Reset/Delete all the timers */
    TimerDelete(g_lightapp_data.gatt_data.app_tid);
    g_lightapp_data.gatt_data.app_tid = TIMER_INVALID;

    TimerDelete(g_lightapp_data.gatt_data.con_param_update_tid);
    g_lightapp_data.gatt_data.con_param_update_tid = TIMER_INVALID;
    g_lightapp_data.gatt_data.cpu_timer_value = 0;

    g_lightapp_data.gatt_data.st_ucid = GATT_INVALID_UCID;
    g_lightapp_data.gatt_data.enable_white_list = FALSE;
    g_lightapp_data.gatt_data.advert_timer_value = 0;

    /* Reset the connection parameter variables. */
    g_lightapp_data.gatt_data.conn_interval = 0;
    g_lightapp_data.gatt_data.conn_latency = 0;
    g_lightapp_data.gatt_data.conn_timeout = 0;

    /* Initialise GAP Data structure */
    GapDataInit();

#ifdef ENABLE_GATT_OTA_SERVICE
    /* Initialise the CSR OTA Service Data */
    OtaDataInit();
#endif /* ENABLE_GATT_OTA_SERVICE */
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      readPersistentStore
 *
 *  DESCRIPTION
 *      This function is used to initialize and read NVM data
 *
 *  RETURNS/MODIFIES
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void readPersistentStore(void)
{
    /* NVM offset for supported services */
    uint16 nvm_offset = 0;
    uint16 nvm_sanity = 0xffff;
    bool nvm_start_fresh = FALSE;
    uint16 app_nvm_version = 0;
    uint32 temp = 0;

    nvm_offset = NVM_MAX_APP_MEMORY_WORDS + CSR_MESH_NVM_SIZE;

    /* Read the Application NVM version */
    Nvm_Read(&app_nvm_version, 1, NVM_OFFSET_APP_NVM_VERSION);

    if( app_nvm_version != APP_NVM_VERSION )
    {
        /* The NVM structure could have changed
         * with a new version of the application, due to NVM values getting
         * removed or added. Currently this application clears all the application
         * and CSRmesh NVM values and writes to NVM
         */
#ifdef NVM_TYPE_EEPROM
        uint16 i;
        uint16 eeprom_erase = 0xFFFF;
        /* Erase a block in EEPROM to remove all sanity words */
        for (i = 0; i < 128; i++)
        {
            Nvm_Write(&eeprom_erase, 0x1, i);
        }
#elif NVM_TYPE_FLASH
        NvmErase(FALSE);
#endif /* NVM_TYPE_EEPROM */

        /* Save new version of the NVM */
        app_nvm_version = APP_NVM_VERSION;
        NvmWrite(&app_nvm_version, 1, NVM_OFFSET_APP_NVM_VERSION);
    }

    /* Read the sanity word */
    Nvm_Read(&nvm_sanity, sizeof(nvm_sanity),
             NVM_OFFSET_SANITY_WORD);

    if(nvm_sanity == NVM_SANITY_MAGIC)
    {
        /* Read Bonded Flag from NVM */
        Nvm_Read((uint16*)&g_lightapp_data.gatt_data.bonded,
                  sizeof(g_lightapp_data.gatt_data.bonded),
                  NVM_OFFSET_BONDED_FLAG);

        if(g_lightapp_data.gatt_data.bonded)
        {
            /* Bonded Host Typed BD Address will only be stored if bonded flag
             * is set to TRUE. Read last bonded device address.
             */
            Nvm_Read((uint16*)&g_lightapp_data.gatt_data.bonded_bd_addr,
                     sizeof(TYPED_BD_ADDR_T),
                     NVM_OFFSET_BONDED_ADDR);

            /* If device is bonded and bonded address is resolvable then read
             * the bonded device's IRK
             */
            if(GattIsAddressResolvableRandom(
                                &g_lightapp_data.gatt_data.bonded_bd_addr))
            {
                Nvm_Read(g_lightapp_data.gatt_data.central_device_irk.irk,
                         MAX_WORDS_IRK,
                         NVM_OFFSET_SM_IRK);
            }
        }
        else /* Case when we have only written NVM_SANITY_MAGIC to NVM but
              * didn't get bonded to any host in the last powered session
              */
        {
            g_lightapp_data.gatt_data.bonded = FALSE;
        }

        /* Read the diversifier associated with the presently bonded/last
         * bonded device.
         */
        Nvm_Read(&g_lightapp_data.gatt_data.diversifier,
                 sizeof(g_lightapp_data.gatt_data.diversifier),
                 NVM_OFFSET_SM_DIV);

        /* Read association state from NVM */
        Nvm_Read((uint16 *)&g_lightapp_data.assoc_state,
                sizeof(g_lightapp_data.assoc_state),
                NVM_OFFSET_ASSOCIATION_STATE);

        /* Read RGB and Power Data from NVM */
        Nvm_Read((uint16 *)&temp, sizeof(uint32),
                 NVM_RGB_DATA_OFFSET);

        /* Read assigned Groups IDs for Light model from NVM */
        Nvm_Read((uint16 *)light_model_groups, sizeof(light_model_groups),
                                                 NVM_OFFSET_LIGHT_MODEL_GROUPS);

        /* Read assigned Groups IDs for Power model from NVM */
        Nvm_Read((uint16 *)power_model_groups, sizeof(power_model_groups),
                                                 NVM_OFFSET_LIGHT_MODEL_GROUPS);

        /* Read assigned Groups IDs for Attention model from NVM */
        Nvm_Read((uint16 *)attention_model_groups,
                  sizeof(attention_model_groups),NVM_OFFSET_LIGHT_MODEL_GROUPS);

        /* Unpack data in to the global variables */
        g_lightapp_data.light_state.red   = temp & 0xFF;
        temp >>= 8;
        g_lightapp_data.light_state.green = temp & 0xFF;
        temp >>= 8;
        g_lightapp_data.light_state.blue  = temp & 0xFF;
        temp >>= 8;
        g_lightapp_data.power.power_state = temp & 0xFF;
        g_lightapp_data.light_state.power = g_lightapp_data.power.power_state;

        /* Read Bearer Model Data from NVM */
        Nvm_Read((uint16 *)&g_lightapp_data.bearer_data,
                 sizeof(BEARER_MODEL_STATE_DATA_T), NVM_BEARER_DATA_OFFSET);

        /* If NVM in use, read device name and length from NVM */
        GapReadDataFromNVM(&nvm_offset);
    }
    else /* NVM Sanity check failed means either the device is being brought up
          * for the first time or memory has got corrupted in which case
          * discard the data and start fresh.
          */
    {
        /* Read Configuration flags from User CS Key */
        uint16 cskey_flags = CSReadUserKey(CSKEY_INDEX_USER_FLAGS);

        nvm_start_fresh = TRUE;

        nvm_sanity = NVM_SANITY_MAGIC;

        /* Write NVM Sanity word to the NVM */
        Nvm_Write(&nvm_sanity, sizeof(nvm_sanity),
                  NVM_OFFSET_SANITY_WORD);

        if(cskey_flags & RANDOM_UUID_ENABLE_MASK)
        {
            /* The flag is set so generate a random UUID NVM */
            uint8 i;
            for( i = 0 ; i < 8 ; i++)
            {
                light_uuid[i] = Random16();
            }
        }

        /* Write to NVM */
        Nvm_Write(light_uuid, DEVICE_UUID_SIZE_WORDS,
                  NVM_OFFSET_DEVICE_UUID);

        /* Write Authorization Code to NVM */
        Nvm_Write(lightAuthCode, DEVICE_AUTHCODE_SIZE_IN_WORDS,
                  NVM_OFFSET_DEVICE_AUTHCODE);

        /* The device will not be bonded as it is coming up for the first
         * time
         */
        g_lightapp_data.gatt_data.bonded = FALSE;

        /* Write bonded status to NVM */
        Nvm_Write((uint16*)&g_lightapp_data.gatt_data.bonded,
                  sizeof(g_lightapp_data.gatt_data.bonded),
                  NVM_OFFSET_BONDED_FLAG);

        /* When the application is coming up for the first time after flashing
         * the image to it, it will not have bonded to any device. So, no LTK
         * will be associated with it. Hence, set the diversifier to 0.
         */
        g_lightapp_data.gatt_data.diversifier = 0;

        /* Write the same to NVM. */
        Nvm_Write(&g_lightapp_data.gatt_data.diversifier,
                  sizeof(g_lightapp_data.gatt_data.diversifier),
                  NVM_OFFSET_SM_DIV);

        /* The device will not be associated as it is coming up for the
         * first time
         */
        g_lightapp_data.assoc_state = app_state_not_associated;

        /* Write association state to NVM */
        Nvm_Write((uint16 *)&g_lightapp_data.assoc_state,
                 sizeof(g_lightapp_data.assoc_state),
                 NVM_OFFSET_ASSOCIATION_STATE);


        /* Write RGB Data and Power to NVM.
         * Data is stored in the following format.
         * HIGH WORD: MSB: POWER LSB: BLUE.
         * LOW  WORD: MSB: GREEN LSB: RED.
         */
        g_lightapp_data.light_state.red   = 0xFF;
        g_lightapp_data.light_state.green = 0xFF;
        g_lightapp_data.light_state.blue  = 0xFF;
        g_lightapp_data.light_state.power = POWER_STATE_OFF;
        g_lightapp_data.power.power_state = POWER_STATE_OFF;

        temp = ((uint32) g_lightapp_data.power.power_state << 24) |
               ((uint32) g_lightapp_data.light_state.blue  << 16) |
               ((uint32) g_lightapp_data.light_state.green <<  8) |
               g_lightapp_data.light_state.red;

        Nvm_Write((uint16 *)&temp, sizeof(uint32),
                 NVM_RGB_DATA_OFFSET);

        /* Update Bearer Model Data from CSKey flags for the first time. */
        g_lightapp_data.bearer_data.promiscuous       = FALSE;
        g_lightapp_data.bearer_data.bearerEnabled     = BLE_BEARER_MASK;
        g_lightapp_data.bearer_data.bearerRelayActive = 0x0000;

        if (cskey_flags & RELAY_ENABLE_MASK)
        {
            g_lightapp_data.bearer_data.bearerRelayActive |= BLE_BEARER_MASK;
        }

        if (cskey_flags & BRIDGE_ENABLE_MASK)
        {
            g_lightapp_data.bearer_data.bearerEnabled     |= BLE_GATT_SERVER_BEARER_MASK;
            g_lightapp_data.bearer_data.bearerRelayActive |= BLE_GATT_SERVER_BEARER_MASK;
        }

        /* Update Bearer Model Data to NVM */
        Nvm_Write((uint16 *)&g_lightapp_data.bearer_data,
                  sizeof(BEARER_MODEL_STATE_DATA_T), NVM_BEARER_DATA_OFFSET);

        /* If fresh NVM, write device name and length to NVM for the
         * first time.
         */
        GapInitWriteDataToNVM(&nvm_offset);
    }

#ifdef ENABLE_GATT_OTA_SERVICE
    /* Read GATT data from NVM */
    GattReadDataFromNVM(&nvm_offset);
#endif /* ENABLE_GATT_OTA_SERVICE */

    /* Read the UUID from NVM */
    Nvm_Read(light_uuid, DEVICE_UUID_SIZE_WORDS, NVM_OFFSET_DEVICE_UUID);

    /* Read Authorization Code from NVM */
    Nvm_Read(lightAuthCode, DEVICE_AUTHCODE_SIZE_IN_WORDS,
             NVM_OFFSET_DEVICE_AUTHCODE);

    /* Read Mesh Control Service data from NVM if the devices are bonded and
     * update the offset with the number of words of NVM required by
     * this service
     */
    MeshControlReadDataFromNVM(&nvm_offset);
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      UartDataRxCallback
 *
 *  DESCRIPTION
 *      This callback is issued when data is received over UART. Application
 *      may ignore the data, if not required. For more information refer to
 *      the API documentation for the type "uart_data_out_fn"
 *
 *  RETURNS
 *      The number of words processed, return data_count if all of the received
 *      data had been processed (or if application don't care about the data)
 *
 *----------------------------------------------------------------------------*/
#ifdef DEBUG_ENABLE
static uint16 UartDataRxCallback ( void* p_data, uint16 data_count,
        uint16* p_num_additional_words )
{

    /* Application needs 1 additional data to be received */
    *p_num_additional_words = 1;

    return data_count;
}
#endif /* DEBUG_ENABLE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      appInitExit
 *
 *  DESCRIPTION
 *      This function is called upon exiting from app_state_init state. The
 *      application starts advertising after exiting this state.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void appInitExit(void)
{
        if(g_lightapp_data.gatt_data.bonded &&
            (!GattIsAddressResolvableRandom(
                                &g_lightapp_data.gatt_data.bonded_bd_addr)))
        {
            /* If the device is bonded and bonded device address is not
             * resolvable random, configure White list with the Bonded
             * host address
             */
            if(LsAddWhiteListDevice(
                    &g_lightapp_data.gatt_data.bonded_bd_addr)!= ls_err_none)
            {
                ReportPanic(app_panic_add_whitelist);
            }
        }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appAdvertisingExit
 *
 *  DESCRIPTION
 *      This function is called while exiting app_state_fast_advertising and
 *      app_state_slow_advertising states.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void appAdvertisingExit(void)
{
        /* Cancel advertisement timer */
        TimerDelete(g_lightapp_data.gatt_data.app_tid);
        g_lightapp_data.gatt_data.app_tid = TIMER_INVALID;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalSmKeysInd
 *
 *  DESCRIPTION
 *      This function handles the signal SM_KEYS_IND and copies IRK from it
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalSmKeysInd(SM_KEYS_IND_T *p_event_data)
{
    /*Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_connected:
        {
            /* If keys are present, save them */
            if((p_event_data->keys)->keys_present & (1 << SM_KEY_TYPE_DIV))
            {
                /* Store the diversifier which will be used for accepting/
                 * rejecting the encryption requests.
                 */
                g_lightapp_data.gatt_data.diversifier =
                                                    (p_event_data->keys)->div;

                /* Write the new diversifier to NVM */
                Nvm_Write(&g_lightapp_data.gatt_data.diversifier,
                          sizeof(g_lightapp_data.gatt_data.diversifier),
                          NVM_OFFSET_SM_DIV);
            }

            /* Store IRK if the connected host is using random resolvable
             * address. IRK is used afterwards to validate the identity of
             * connected host
             */
            if(GattIsAddressResolvableRandom(
                                &g_lightapp_data.gatt_data.con_bd_addr) &&
               ((p_event_data->keys)->keys_present & (1 << SM_KEY_TYPE_ID)))
            {
                MemCopy(g_lightapp_data.gatt_data.central_device_irk.irk,
                        (p_event_data->keys)->irk,
                        MAX_WORDS_IRK);

                /* If bonded device address is resolvable random
                 * then store IRK to NVM
                 */
                Nvm_Write(g_lightapp_data.gatt_data.central_device_irk.irk,
                          MAX_WORDS_IRK,
                          NVM_OFFSET_SM_IRK);
            }

        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalSmDivApproveInd
 *
 *  DESCRIPTION
 *      This function handles the signal SM_DIV_APPROVE_IND.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void handleSignalSmDivApproveInd(SM_DIV_APPROVE_IND_T *p_event_data)
{
    /* Handling signal as per current state */
    switch(g_lightapp_data.state)
    {

        /* Request for approval from application comes only when pairing is not
         * in progress
         */
        case app_state_connected:
        {
            sm_div_verdict approve_div = SM_DIV_REVOKED;

            /* Check whether the application is still bonded (bonded flag gets
             * reset upon 'connect' button press by the user). Then check
             * whether the diversifier is the same as the one stored by the
             * application
             */
            if(AppIsDeviceBonded())
            {
                if(g_lightapp_data.gatt_data.diversifier == p_event_data->div)
                {
                    approve_div = SM_DIV_APPROVED;
                }
            }

            SMDivApproval(p_event_data->cid, approve_div);
        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;

    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalGattAddDBCfm
 *
 *  DESCRIPTION
 *      This function handles the signal GATT_ADD_DB_CFM
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalGattAddDBCfm(GATT_ADD_DB_CFM_T *p_event_data)
{
    switch(g_lightapp_data.state)
    {
        case app_state_init:
        {
            if(p_event_data->result == sys_status_success)
            {
                /* Always do slow adverts on GATT Connection */
                AppSetState(app_state_fast_advertising);
            }
            else
            {
                /* Don't expect this to happen */
                ReportPanic(app_panic_db_registration);
            }
        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalGattCancelConnectCfm
 *
 *  DESCRIPTION
 *      This function handles the signal GATT_CANCEL_CONNECT_CFM
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalGattCancelConnectCfm(void)
{
    /*Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_fast_advertising:
        {
            /* Do nothing here */
        }
        break;

        case app_state_slow_advertising:
            /* There is no idle state, the device
             * will advertise for ever
             */
        break;

        case app_state_connected:
            /* The CSRmesh could have been sending data on
             * advertisements so do not panic
             */
        break;

        case app_state_idle:
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}

/*---------------------------------------------------------------------------
 *
 *  NAME
 *      handleSignalLmEvConnectionComplete
 *
 *  DESCRIPTION
 *      This function handles the signal LM_EV_CONNECTION_COMPLETE.
 *
 *  RETURNS
 *      Nothing.
 *

*----------------------------------------------------------------------------*/
static void handleSignalLmEvConnectionComplete(
                                     LM_EV_CONNECTION_COMPLETE_T *p_event_data)
{
    /* Store the connection parameters. */
    g_lightapp_data.gatt_data.conn_interval = p_event_data->data.conn_interval;
    g_lightapp_data.gatt_data.conn_latency  = p_event_data->data.conn_latency;
    g_lightapp_data.gatt_data.conn_timeout  =
                                        p_event_data->data.supervision_timeout;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalGattConnectCfm
 *
 *  DESCRIPTION
 *      This function handles the signal GATT_CONNECT_CFM
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalGattConnectCfm(GATT_CONNECT_CFM_T* p_event_data)
{
    /*Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_fast_advertising:
        case app_state_slow_advertising:
        {
            if(p_event_data->result == sys_status_success)
            {
                /* Store received UCID */
                g_lightapp_data.gatt_data.st_ucid = p_event_data->cid;

                /* Store connected BD Address */
                g_lightapp_data.gatt_data.con_bd_addr = p_event_data->bd_addr;

                if((g_lightapp_data.gatt_data.bonded) &&
                   (GattIsAddressResolvableRandom(
                            &g_lightapp_data.gatt_data.bonded_bd_addr)) &&
                   (SMPrivacyMatchAddress(&p_event_data->bd_addr,
                           g_lightapp_data.gatt_data.central_device_irk.irk,
                           MAX_NUMBER_IRK_STORED, MAX_WORDS_IRK) < 0))
                {
                    /* Application was bonded to a remote device using
                     * resolvable random address and application has failed to
                     * resolve the remote device address to which we just
                     * connected So disconnect and start advertising again
                     */

                    /* Disconnect if we are connected */
                    AppSetState(app_state_disconnecting);
                }
                else
                {
                    /* Enter connected state
                     * - If the device is not bonded OR
                     * - If the device is bonded and the connected host doesn't
                     *   support Resolvable Random address OR
                     * - If the device is bonded and connected host supports
                     *   Resolvable Random address and the address gets resolved
                     *   using the store IRK key
                     */
                    AppSetState(app_state_connected);

                    /* Inform CSRmesh that we are connected now */
                    CsrMeshHandleDataInConnection(
                                    g_lightapp_data.gatt_data.st_ucid,
                                    g_lightapp_data.gatt_data.conn_interval);

                    /* Start Listening in CSRmesh */
                    CsrMeshStart();
#ifdef ENABLE_GATT_OTA_SERVICE
                    /* If we are bonded to this host, then it may be appropriate
                     * to indicate that the database is not now what it had
                     * previously.
                     */
                    if(g_lightapp_data.gatt_data.bonded &&
                       (!MemCmp(&g_lightapp_data.gatt_data.bonded_bd_addr,
                                &g_lightapp_data.gatt_data.con_bd_addr,
                          sizeof(g_lightapp_data.gatt_data.bonded_bd_addr))))
                    {
                        GattOnConnection();
                    }
#endif /* ENABLE_GATT_OTA_SERVICE */
                    /* Since CSRmesh application does not mandate encryption
                     * requirement on its characteristics, the remote master may
                     * or may not encrypt the link. Start a timer  here to give
                     * remote master some time to encrypt the link and on expiry
                     * of that timer, send a connection parameter update request
                     * to remote side.
                     */

                    /* Don't request security as this causes connection issues
                     * with Android 4.4
                     *
                     * SMRequestSecurityLevel(&g_lightapp_data.gatt_data.con_bd_addr);
                     */

                    /* If the current connection parameters being used don't
                     * comply with the application's preferred connection
                     * parameters and the timer is not running and, start timer
                     * to trigger Connection Parameter Update procedure
                     */
                    if((g_lightapp_data.gatt_data.con_param_update_tid ==
                                                            TIMER_INVALID) &&
                       (g_lightapp_data.gatt_data.conn_interval <
                                                 PREFERRED_MIN_CON_INTERVAL ||
                        g_lightapp_data.gatt_data.conn_interval >
                                                 PREFERRED_MAX_CON_INTERVAL
#if PREFERRED_SLAVE_LATENCY
                        || g_lightapp_data.gatt_data.conn_latency <
                                                 PREFERRED_SLAVE_LATENCY
#endif
                       )
                      )
                    {
                        /* Set the num of conn update attempts to zero */
                        g_lightapp_data.gatt_data.num_conn_update_req = 0;

                        /* The application first starts a timer of
                         * TGAP_CPP_PERIOD. During this time, the application
                         * waits for the peer device to do the database
                         * discovery procedure. After expiry of this timer, the
                         * application starts one more timer of period
                         * TGAP_CPC_PERIOD. If the application receives any
                         * GATT_ACCESS_IND during this time, it assumes that
                         * the peer device is still doing device database
                         * discovery procedure or some other configuration and
                         * it should not update the parameters, so it restarts
                         * the TGAP_CPC_PERIOD timer. If this timer expires, the
                         * application assumes that database discovery procedure
                         * is complete and it initiates the connection parameter
                         * update procedure.
                         */
                        g_lightapp_data.gatt_data.con_param_update_tid =
                                          TimerCreate(TGAP_CPP_PERIOD, TRUE,
                                                      handleGapCppTimerExpiry);
                        g_lightapp_data.gatt_data.cpu_timer_value =
                                                            TGAP_CPP_PERIOD;
                    }
                      /* Else at the expiry of timer Connection parameter
                       * update procedure will get triggered
                       */
                }
            }
            else
            {
                /* We don't use slow advertising. So, switch device
                 * to fast adverts.
                 */
                AppSetState(app_state_fast_advertising);
            }
        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalSmSimplePairingCompleteInd
 *
 *  DESCRIPTION
 *      This function handles the signal SM_SIMPLE_PAIRING_COMPLETE_IND
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalSmSimplePairingCompleteInd(
                                 SM_SIMPLE_PAIRING_COMPLETE_IND_T *p_event_data)
{
    /*Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_connected:
        {
            if(p_event_data->status == sys_status_success)
            {
                /* Store bonded host information to NVM. This includes
                 * application and services specific information
                 */
                g_lightapp_data.gatt_data.bonded = TRUE;
                g_lightapp_data.gatt_data.bonded_bd_addr =
                                                   p_event_data->bd_addr;

                /* Store bonded host typed BD address to NVM */

                /* Write one word bonded flag */
                Nvm_Write((uint16*)&g_lightapp_data.gatt_data.bonded,
                          sizeof(g_lightapp_data.gatt_data.bonded),
                          NVM_OFFSET_BONDED_FLAG);

                /* Write typed BD address of bonded host */
                Nvm_Write((uint16*)&g_lightapp_data.gatt_data.bonded_bd_addr,
                          sizeof(TYPED_BD_ADDR_T),
                          NVM_OFFSET_BONDED_ADDR);

                /* Configure white list with the Bonded host address only
                 * if the connected host doesn't support random resolvable
                 * address
                 */
                if(!GattIsAddressResolvableRandom(
                    &g_lightapp_data.gatt_data.bonded_bd_addr))
                {
                    /* It is important to note that this application
                     * doesn't support reconnection address. In future, if
                     * the application is enhanced to support Reconnection
                     * Address, make sure that we don't add reconnection
                     * address to white list
                     */
                    if(LsAddWhiteListDevice(
                             &g_lightapp_data.gatt_data.bonded_bd_addr) !=
                                                               ls_err_none)
                    {
                        ReportPanic(app_panic_add_whitelist);
                    }
                }

                /* If the devices are bonded then send notification to all
                 * registered services for the same so that they can store
                 * required data to NVM.
                 */

            }
            else
            {
                /* Pairing has failed.
                 * 1. If pairing has failed due to repeated attempts, the
                 *    application should immediately disconnect the link.
                 * 2. The application was bonded and pairing has failed.
                 *    Since the application was using whitelist, so the remote
                 *    device has same address as our bonded device address.
                 *    The remote connected device may be a genuine one but
                 *    instead of using old keys, wanted to use new keys. We
                 *    don't allow bonding again if we are already bonded but we
                 *    will give some time to the connected device to encrypt the
                 *    link using the old keys. if the remote device encrypts the
                 *    link in that time, it's good. Otherwise we will disconnect
                 *    the link.
                 */
                if(p_event_data->status == sm_status_repeated_attempts)
                {
                    AppSetState(app_state_disconnecting);
                }
            }
        }
        break;

        default:
            /* Firmware may send this signal after disconnection. So don't
             * panic but ignore this signal.
             */
        break;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalLsConnParamUpdateCfm
 *
 *  DESCRIPTION
 *      This function handles the signal LS_CONNECTION_PARAM_UPDATE_CFM.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalLsConnParamUpdateCfm(
                            LS_CONNECTION_PARAM_UPDATE_CFM_T *p_event_data)
{
    /*Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_connected:
        {
            /* Received in response to the L2CAP_CONNECTION_PARAMETER_UPDATE
              * request sent from the slave after encryption is enabled. If
              * the request has failed, the device should again send the same
              * request only after Tgap(conn_param_timeout). Refer
              * Bluetooth 4.0 spec Vol 3 Part C, Section 9.3.9 and profile spec.
              */
            if ((p_event_data->status != ls_err_none) &&
                (g_lightapp_data.gatt_data.num_conn_update_req <
                                        MAX_NUM_CONN_PARAM_UPDATE_REQS))
            {
                /* Delete timer if running */
                TimerDelete(g_lightapp_data.gatt_data.con_param_update_tid);

                g_lightapp_data.gatt_data.con_param_update_tid =
                                 TimerCreate(GAP_CONN_PARAM_TIMEOUT,
                                             TRUE, requestConnParamUpdate);
                g_lightapp_data.gatt_data.cpu_timer_value =
                                             GAP_CONN_PARAM_TIMEOUT;
            }
        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}


/*-----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalSmPairingAuthInd
 *
 *  DESCRIPTION
 *      This function handles the signal SM_PAIRING_AUTH_IND. This message will
 *      only be received when the peer device is initiating 'Just Works'
 8      pairing.
 *
 *  RETURNS/MODIFIES
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void handleSignalSmPairingAuthInd(SM_PAIRING_AUTH_IND_T *p_event_data)
{
    /* Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_connected:
        {
            /* Always authorise a new pairing request, as there is no way
             * to delete an existing bonding. In case host loses the bonding
             * with the app it will be allowed to connect with a new pairing
             */
            SMPairingAuthRsp(p_event_data->data, TRUE);
        }
        break;

        default:
            ReportPanic(app_panic_invalid_state);
        break;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalLmConnectionUpdate
 *
 *  DESCRIPTION
 *      This function handles the signal LM_EV_CONNECTION_UPDATE.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalLmConnectionUpdate(
                                   LM_EV_CONNECTION_UPDATE_T* p_event_data)
{
    switch(g_lightapp_data.state)
    {
        case app_state_connected:
        case app_state_disconnecting:
        {
            /* Store the new connection parameters. */
            g_lightapp_data.gatt_data.conn_interval =
                                            p_event_data->data.conn_interval;
            g_lightapp_data.gatt_data.conn_latency =
                                            p_event_data->data.conn_latency;
            g_lightapp_data.gatt_data.conn_timeout =
                                        p_event_data->data.supervision_timeout;

            CsrMeshHandleDataInConnection(g_lightapp_data.gatt_data.st_ucid,
                                       g_lightapp_data.gatt_data.conn_interval);

            DEBUG_STR("Parameter Update Complete: ");
            DEBUG_U16(g_lightapp_data.gatt_data.conn_interval);
            DEBUG_STR("\r\n");
        }
        break;

        default:
            /* Connection parameter update indication received in unexpected
             * application state.
             */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalLsConnParamUpdateInd
 *
 *  DESCRIPTION
 *      This function handles the signal LS_CONNECTION_PARAM_UPDATE_IND.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalLsConnParamUpdateInd(
                                 LS_CONNECTION_PARAM_UPDATE_IND_T *p_event_data)
{
    /*Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_connected:
        {
            /* Delete timer if running */
            TimerDelete(g_lightapp_data.gatt_data.con_param_update_tid);
            g_lightapp_data.gatt_data.con_param_update_tid = TIMER_INVALID;
            g_lightapp_data.gatt_data.cpu_timer_value = 0;

            /* The application had already received the new connection
             * parameters while handling event LM_EV_CONNECTION_UPDATE.
             * Check if new parameters comply with application preferred
             * parameters. If not, application shall trigger Connection
             * parameter update procedure
             */

            if(g_lightapp_data.gatt_data.conn_interval <
                                                PREFERRED_MIN_CON_INTERVAL ||
               g_lightapp_data.gatt_data.conn_interval >
                                                PREFERRED_MAX_CON_INTERVAL
#if PREFERRED_SLAVE_LATENCY
               || g_lightapp_data.gatt_data.conn_latency <
                                                PREFERRED_SLAVE_LATENCY
#endif
              )
            {
                /* Set the num of conn update attempts to zero */
                g_lightapp_data.gatt_data.num_conn_update_req = 0;

                /* Start timer to trigger Connection Parameter Update
                 * procedure
                 */
                g_lightapp_data.gatt_data.con_param_update_tid =
                                TimerCreate(GAP_CONN_PARAM_TIMEOUT,
                                            TRUE, requestConnParamUpdate);
                g_lightapp_data.gatt_data.cpu_timer_value =
                                                        GAP_CONN_PARAM_TIMEOUT;
            }
        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalGattAccessInd
 *
 *  DESCRIPTION
 *      This function handles GATT_ACCESS_IND message for attributes
 *      maintained by the application.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalGattAccessInd(GATT_ACCESS_IND_T *p_event_data)
{

    /*Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_connected:
        {
            /* GATT_ACCESS_IND indicates that the central device is still disco-
             * -vering services. So, restart the connection parameter update
             * timer
             */
             if(g_lightapp_data.gatt_data.cpu_timer_value == TGAP_CPC_PERIOD &&
                g_lightapp_data.gatt_data.con_param_update_tid != TIMER_INVALID)
             {
                TimerDelete(g_lightapp_data.gatt_data.con_param_update_tid);
                g_lightapp_data.gatt_data.con_param_update_tid =
                                    TimerCreate(TGAP_CPC_PERIOD,
                                                TRUE, requestConnParamUpdate);
             }

            /* Received GATT ACCESS IND with write access */
            if(p_event_data->flags & ATT_ACCESS_WRITE)
            {
                /* If only ATT_ACCESS_PERMISSION flag is enabled, then the
                 * firmware is asking the app for permission to go along with
                 * prepare write request from the peer. Allow it.
                 */
                if(((p_event_data->flags) &
                   (ATT_ACCESS_PERMISSION | ATT_ACCESS_WRITE_COMPLETE))
                                                    == ATT_ACCESS_PERMISSION)
                {
                    GattAccessRsp(p_event_data->cid, p_event_data->handle,
                                  sys_status_success, 0, NULL);
                }
                else
                {
                    HandleAccessWrite(p_event_data);
                }
            }
            else if(p_event_data->flags & ATT_ACCESS_WRITE_COMPLETE)
            {
                GattAccessRsp(p_event_data->cid, p_event_data->handle,
                                          sys_status_success, 0, NULL);
            }
            /* Received GATT ACCESS IND with read access */
            else if(p_event_data->flags ==
                                    (ATT_ACCESS_READ | ATT_ACCESS_PERMISSION))
            {
                HandleAccessRead(p_event_data);
            }
            else
            {
                GattAccessRsp(p_event_data->cid, p_event_data->handle,
                              gatt_status_request_not_supported,
                              0, NULL);
            }
        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      handleSignalLmDisconnectComplete
 *
 *  DESCRIPTION
 *      This function handles LM Disconnect Complete event which is received
 *      at the completion of disconnect procedure triggered either by the
 *      device or remote host or because of link loss
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleSignalLmDisconnectComplete(
                HCI_EV_DATA_DISCONNECT_COMPLETE_T *p_event_data)
{

    /* Reset the connection parameter variables. */
    g_lightapp_data.gatt_data.conn_interval = 0;
    g_lightapp_data.gatt_data.conn_latency = 0;
    g_lightapp_data.gatt_data.conn_timeout = 0;

    CsrMeshHandleDataInConnection(GATT_INVALID_UCID, 0);
#ifdef ENABLE_GATT_OTA_SERVICE
    if(OtaResetRequired())
    {
        OtaReset();
    }
#endif /* ENABLE_GATT_OTA_SERVICE */
    /* LM_EV_DISCONNECT_COMPLETE event can have following disconnect
     * reasons:
     *
     * HCI_ERROR_CONN_TIMEOUT - Link Loss case
     * HCI_ERROR_CONN_TERM_LOCAL_HOST - Disconnect triggered by device
     * HCI_ERROR_OETC_* - Other end (i.e., remote host) terminated connection
     */

    /*Handling signal as per current state */
    switch(g_lightapp_data.state)
    {
        case app_state_connected:
        case app_state_disconnecting:
        {
            /* Connection is terminated either due to Link Loss or
             * the local host terminated connection. In either case
             * initialize the app data and go to fast advertising.
             */
            smLightDataInit();
            AppSetState(app_state_fast_advertising);
        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleCsrMeshGroupSetMsg
 *
 *  DESCRIPTION
 *      This function handles the CSRmesh Group Assignment message. Stores
 *      the group_id at the given index for the model
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleCsrMeshGroupSetMsg(uint8 *msg, uint16 len)
{
    CSR_MESH_MODEL_TYPE_T model = msg[0];
    uint8 index = msg[1];
    uint16 group_id = msg[3] + (msg[4] << 8);

    switch(model)
    {
        case CSR_MESH_LIGHT_MODEL:
        {
            /* Store Group ID */
            light_model_groups[index] = group_id;

            /* Save to NVM */
            Nvm_Write(&light_model_groups[index],
                     sizeof(uint16),
                     NVM_OFFSET_LIGHT_MODEL_GROUPS + index);
        }
        break;

        case CSR_MESH_POWER_MODEL:
        {
            power_model_groups[index] = group_id;

            /* Save to NVM */
            Nvm_Write(&power_model_groups[index],
                     sizeof(uint16),
                     NVM_OFFSET_POWER_MODEL_GROUPS + index);
        }
        break;

        case CSR_MESH_ATTENTION_MODEL:
        {
            attention_model_groups[index] = group_id;

            /* Save to NVM */
            Nvm_Write(&attention_model_groups[index],
                     sizeof(uint16),
                     NVM_OFFSET_ATTENTION_MODEL_GROUPS + index);
        }
        break;

        default:
        break;
    }
}

/*============================================================================*
 *  Public Function Definitions
 *============================================================================*/
#ifdef NVM_TYPE_FLASH
/*----------------------------------------------------------------------------*
 *  NAME
 *      WriteApplicationAndServiceDataToNVM
 *
 *  DESCRIPTION
 *      This function writes the application data to NVM. This function should
 *      be called on getting nvm_status_needs_erase
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void WriteApplicationAndServiceDataToNVM(void)
{
    uint16 nvm_sanity = 0xffff;
    nvm_sanity = NVM_SANITY_MAGIC;

    /* Write NVM sanity word to the NVM */
    Nvm_Write(&nvm_sanity, sizeof(nvm_sanity), NVM_OFFSET_SANITY_WORD);

    /* Write Bonded flag to NVM. */
    Nvm_Write((uint16*)&g_lightapp_data.gatt_data.bonded,
               sizeof(g_lightapp_data.gatt_data.bonded),
               NVM_OFFSET_BONDED_FLAG);

    /* Write Bonded address to NVM. */
    Nvm_Write((uint16*)&g_lightapp_data.gatt_data.bonded_bd_addr,
              sizeof(TYPED_BD_ADDR_T),
              NVM_OFFSET_BONDED_ADDR);

    /* Write the diversifier to NVM */
    Nvm_Write(&g_lightapp_data.gatt_data.diversifier,
              sizeof(g_lightapp_data.gatt_data.diversifier),
              NVM_OFFSET_SM_DIV);

    /* Store the IRK to NVM */
    Nvm_Write(g_lightapp_data.gatt_data.central_device_irk.irk,
             MAX_WORDS_IRK,
              NVM_OFFSET_SM_IRK);

    /* Store the Association State */
    Nvm_Write(g_lightapp_data.assoc_state,
             sizeof(g_lightapp_data.assoc_state),
              NVM_OFFSET_ASSOCIATION_STATE);

    /* Write GAP service data into NVM */
    WriteGapServiceDataInNVM();

}
#endif /* NVM_TYPE_FLASH */

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppSetState
 *
 *  DESCRIPTION
 *      This function is used to set the state of the application.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void AppSetState(app_state new_state)
{
    /* Check if the new state to be set is not the same as the present state
     * of the application.
     */
    app_state old_state = g_lightapp_data.state;

    if (old_state != new_state)
    {
        /* Handle exiting old state */
        switch (old_state)
        {
            case app_state_init:
                appInitExit();
            break;

            case app_state_disconnecting:
                /* Common things to do whenever application exits
                 * app_state_disconnecting state.
                 */

                /* Initialise CSRmesh light data and services
                 * data structure while exiting Disconnecting state.
                 */
                smLightDataInit();
            break;

            case app_state_fast_advertising:
            case app_state_slow_advertising:
                /* Common things to do whenever application exits
                 * APP_*_ADVERTISING state.
                 */
                /* Stop on-going advertisements */
                GattStopAdverts();
                appAdvertisingExit();
            break;

            case app_state_connected:
                /* Do nothing here */
            break;

            case app_state_idle:
            {
            }
            break;

            default:
                /* Nothing to do */
            break;
        }

        /* Set new state */
        g_lightapp_data.state = new_state;

        /* Handle entering new state */
        switch (new_state)
        {
            case app_state_fast_advertising:
            {
                GattTriggerFastAdverts();
            }
            break;

            case app_state_slow_advertising:
            {
                /* Print an error message if we are in
                 * Slow Advertising state by mistake.
                 */
                DEBUG_STR("\r\nERROR: in SLOW ADVERTISING\r\n");
            }
            break;

            case app_state_idle:
            {
                CsrMeshStart();
            }
            break;

            case app_state_connected:
            {
                DEBUG_STR("Connected\r\n");
                /* Common things to do whenever application enters
                 * app_state_connected state.
                 */

                /* Stop on-going advertisements */
                GattStopAdverts();
            }
            break;

            case app_state_disconnecting:
                GattDisconnectReq(g_lightapp_data.gatt_data.st_ucid);
            break;

            default:
            break;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppIsDeviceBonded
 *
 *  DESCRIPTION
 *      This function returns the status whether the connected device is
 *      bonded or not.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern bool AppIsDeviceBonded(void)
{
    bool bonded = FALSE;

    if(g_lightapp_data.gatt_data.bonded)
    {
        if(g_lightapp_data.state == app_state_connected)
        {
            if(!MemCmp(&g_lightapp_data.gatt_data.bonded_bd_addr,
                       &g_lightapp_data.gatt_data.con_bd_addr,
                       sizeof(TYPED_BD_ADDR_T)))
            {
                bonded = TRUE;
            }
        }
        else
        {
            bonded = TRUE;
        }
    }

    return bonded;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      ReportPanic
 *
 *  DESCRIPTION
 *      This function calls firmware panic routine and gives a single point
 *      of debugging any application level panics
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void ReportPanic(app_panic_code panic_code)
{
    /* Raise panic */
    Panic(panic_code);
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      AppPowerOnReset
 *
 *  DESCRIPTION
 *      This user application function is called just after a power-on reset
 *      (including after a firmware panic), or after a wakeup from Hibernate or
 *      Dormant sleep states.
 *
 *      At the time this function is called, the last sleep state is not yet
 *      known.
 *
 *      NOTE: this function should only contain code to be executed after a
 *      power-on reset or panic. Code that should also be executed after an
 *      HCI_RESET should instead be placed in the AppInit() function.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
void AppPowerOnReset(void)
{
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      AppInit
 *
 *  DESCRIPTION
 *      This user application function is called after a power-on reset
 *      (including after a firmware panic), after a wakeup from Hibernate or
 *      Dormant sleep states, or after an HCI Reset has been requested.
 *
 *      The last sleep state is provided to the application in the parameter.
 *
 *      NOTE: In the case of a power-on reset, this function is called
 *      after app_power_on_reset().
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
void AppInit(sleep_state last_sleep_state)
{
    uint16 gatt_db_length = 0;
    uint16 *p_gatt_db_pointer = NULL;
    bool light_poweron = FALSE;

#ifdef USE_STATIC_RANDOM_ADDRESS
    /* Use static random address for the CSRmesh switch. */
    GapSetStaticAddress();
#endif

    /* Initialise the application timers */
    TimerInit(MAX_APP_TIMERS, (void*)app_timers);
    g_lightapp_data.nvm_tid = TIMER_INVALID;

#ifdef DEBUG_ENABLE
    /* Initialize UART and configure with
     * default baud rate and port configuration.
     */
    DebugInit(UART_BUF_SIZE_BYTES_32, UartDataRxCallback, NULL);

    /* UART Rx threshold is set to 1,
     * so that every byte received will trigger the rx callback.
     */
    UartRead(1, 0);
#endif /* DEBUG_ENABLE */

    DEBUG_STR("\r\nLight Application\r\n");

    /* Initialise GATT entity */
    GattInit();

    /* Install GATT Server support for the optional Write procedure
     * This is mandatory only if control point characteristic is supported.
     */
    GattInstallServerWriteLongReliable();

    /* Don't wakeup on UART RX line */
    SleepWakeOnUartRX(FALSE);

#ifdef NVM_TYPE_EEPROM
    /* Configure the NVM manager to use I2C EEPROM for NVM store */
    NvmConfigureI2cEeprom();
#elif NVM_TYPE_FLASH
    /* Configure the NVM Manager to use SPI flash for NVM store. */
    NvmConfigureSpiFlash();
#endif /* NVM_TYPE_EEPROM */

    NvmDisable();
    /* Initialize the GATT and GAP data.
     * Needs to be done before readPersistentStore
     */
    smLightDataInit();

    /* Read persistent storage.
     * Call this before CsrMeshInit.
     */
    readPersistentStore();

    /* Set the CsrMesh NVM start offset.
     * Note: This function must be called before the CsrMeshInit()
     */
    CsrMeshNvmSetStartOffset(NVM_MAX_APP_MEMORY_WORDS);

    /* Initialise the CSRmesh */
    CsrMeshInit();

    /* Enable Relay on Light */
    if (g_lightapp_data.bearer_data.bearerRelayActive & BLE_BEARER_MASK)
    {
        CsrMeshRelayEnable(TRUE);
    }

    /* Enable Notifications for raw messages */
    CsrMeshEnableRawMsgEvent(TRUE);

    /* Initialize the light model */
    LightModelInit(light_model_groups, MAX_MODEL_GROUPS);

    /* Initialize the power model */
    PowerModelInit(power_model_groups, MAX_MODEL_GROUPS);

    /* Initialize Bearer Model */
    BearerModelInit();

    PingModelInit();

#ifdef ENABLE_FIRMWARE_MODEL
    /* Initialize Firmware Model */
    FirmwareModelInit();
#endif

    /* Set Firmware Version */
    g_lightapp_data.fw_version.major_version = APP_MAJOR_VERSION;
    g_lightapp_data.fw_version.minor_version = APP_MINOR_VERSION;

    /* Initialize Attention Model */
    AttentionModelInit(attention_model_groups, MAX_MODEL_GROUPS);

#ifdef ENABLE_BATTERY_MODEL
    BatteryModelInit();
#endif

    /* Start CSRmesh */
    CsrMeshStart();

    /* Set the CSRmesh device UUID */
    CsrMeshSetDeviceUUID((CSR_MESH_UUID_T *)light_uuid);

    /* Set the CSRmesh device Authorization Code */
    CsrMeshSetAuthorisationCode((CSR_MESH_AUTH_CODE_T *)lightAuthCode);

    /* Tell Security Manager module about the value it needs to initialize it's
     * diversifier to.
     */
    SMInit(g_lightapp_data.gatt_data.diversifier);

    /* Initialise CSRmesh light application State */
    g_lightapp_data.state = app_state_init;

    /* Initialize Light Hardware */
    LightHardwareInit();

#ifdef USE_ASSOCIATION_REMOVAL_KEY
    IOTSwitchInit();
#endif /* USE_ASSOCIATION_REMOVAL_KEY */

    /* Start a timer which does device ID adverts till the time device
     * is associated
     */
    if(app_state_not_associated == g_lightapp_data.assoc_state)
    {
        smAppInitiateAssociation();
    }
    else
    {
        DEBUG_STR("Light is associated\r\n");

        /* Light is already associated. Set the colour from NVM */
        LightHardwareSetColor(g_lightapp_data.light_state.red,
                              g_lightapp_data.light_state.green,
                              g_lightapp_data.light_state.blue);


        /* Set the light power as read from NVM */
        if ((g_lightapp_data.power.power_state == POWER_STATE_ON) ||
            (g_lightapp_data.power.power_state == POWER_STATE_ON_FROM_STANDBY))
        {
            light_poweron = TRUE;
        }

        LightHardwarePowerControl(light_poweron);
    }

    /* Tell GATT about our database. We will get a GATT_ADD_DB_CFM event when
     * this has completed.
     */
    p_gatt_db_pointer = GattGetDatabase(&gatt_db_length);
    GattAddDatabaseReq(gatt_db_length, p_gatt_db_pointer);
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      AppProcesSystemEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a system event, such
 *      as a battery low notification, is received by the system.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
void AppProcessSystemEvent(sys_event_id id, void *data)
{
    switch (id)
    {
        case sys_event_pio_changed:
        {
#if (!defined(GUNILAMP) && !defined(DEBUG_ENABLE))
            handlePIOEvent((pio_changed_data*)data);
#endif
        }
        break;

        default:
        break;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessLmEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a LM-specific event
 *      is received by the system.
 *
 * PARAMETERS
 *      event_code [in]   LM event ID
 *      event_data [in]   LM event data
 *
 * RETURNS
 *      TRUE if the application has finished with the event data;
 *           the control layer will free the buffer.
 *----------------------------------------------------------------------------*/
extern bool AppProcessLmEvent(lm_event_code event_code,
                              LM_EVENT_T *p_event_data)
{
    switch(event_code)
    {
        /* Handle events received from Firmware */

        case GATT_ADD_DB_CFM:
            /* Attribute database registration confirmation */
            handleSignalGattAddDBCfm((GATT_ADD_DB_CFM_T*)p_event_data);
        break;

        case GATT_CANCEL_CONNECT_CFM:
            /* Confirmation for the completion of GattCancelConnectReq()
             * procedure
             */
            handleSignalGattCancelConnectCfm();
        break;

        case LM_EV_CONNECTION_COMPLETE:
            /* Handle the LM connection complete event. */
            handleSignalLmEvConnectionComplete((LM_EV_CONNECTION_COMPLETE_T*)
                                                                p_event_data);
        break;

        case GATT_CONNECT_CFM:
            /* Confirmation for the completion of GattConnectReq()
             * procedure
             */
            handleSignalGattConnectCfm((GATT_CONNECT_CFM_T*)p_event_data);
        break;

        case SM_KEYS_IND:
            /* Indication for the keys and associated security information
             * on a connection that has completed Short Term Key Generation
             * or Transport Specific Key Distribution
             */
            handleSignalSmKeysInd((SM_KEYS_IND_T*)p_event_data);
        break;

        case SM_PAIRING_AUTH_IND:
            handleSignalSmPairingAuthInd((SM_PAIRING_AUTH_IND_T *)p_event_data);
        break;

        case SM_SIMPLE_PAIRING_COMPLETE_IND:
            /* Indication for completion of Pairing procedure */
            handleSignalSmSimplePairingCompleteInd(
                (SM_SIMPLE_PAIRING_COMPLETE_IND_T*)p_event_data);
        break;

        case LM_EV_ENCRYPTION_CHANGE:
            /* Indication for encryption change event */
            /* Nothing to do */
        break;

        case SM_DIV_APPROVE_IND:
            /* Indication for SM Diversifier approval requested by F/W when
             * the last bonded host exchange keys. Application may or may not
             * approve the diversifier depending upon whether the application
             * is still bonded to the same host
             */
            handleSignalSmDivApproveInd((SM_DIV_APPROVE_IND_T *)p_event_data);
        break;

        /* Received in response to the LsConnectionParamUpdateReq()
         * request sent from the slave after encryption is enabled. If
         * the request has failed, the device should again send the same
         * request only after Tgap(conn_param_timeout). Refer Bluetooth 4.0
         * spec Vol 3 Part C, Section 9.3.9 and HID over GATT profile spec
         * section 5.1.2.
         */
        case LS_CONNECTION_PARAM_UPDATE_CFM:
            handleSignalLsConnParamUpdateCfm(
                (LS_CONNECTION_PARAM_UPDATE_CFM_T*) p_event_data);
        break;

        case LM_EV_CONNECTION_UPDATE:
            /* This event is sent by the controller on connection parameter
             * update.
             */
            handleSignalLmConnectionUpdate(
                            (LM_EV_CONNECTION_UPDATE_T*)p_event_data);
        break;

        case LS_CONNECTION_PARAM_UPDATE_IND:
            /* Indicates completion of remotely triggered Connection
             * parameter update procedure
             */
            handleSignalLsConnParamUpdateInd(
                            (LS_CONNECTION_PARAM_UPDATE_IND_T *)p_event_data);
        break;

        case GATT_ACCESS_IND:
            /* Indicates that an attribute controlled directly by the
             * application (ATT_ATTR_IRQ attribute flag is set) is being
             * read from or written to.
             */
            handleSignalGattAccessInd((GATT_ACCESS_IND_T*)p_event_data);
        break;

        case GATT_DISCONNECT_IND:
            /* Disconnect procedure triggered by remote host or due to
             * link loss is considered complete on reception of
             * LM_EV_DISCONNECT_COMPLETE event. So, it gets handled on
             * reception of LM_EV_DISCONNECT_COMPLETE event.
             */
         break;

        case GATT_DISCONNECT_CFM:
            /* Confirmation for the completion of GattDisconnectReq()
             * procedure is ignored as the procedure is considered complete
             * on reception of LM_EV_DISCONNECT_COMPLETE event. So, it gets
             * handled on reception of LM_EV_DISCONNECT_COMPLETE event.
             */
        break;

        case LM_EV_DISCONNECT_COMPLETE:
        {
            /* Disconnect procedures either triggered by application or remote
             * host or link loss case are considered completed on reception
             * of LM_EV_DISCONNECT_COMPLETE event
             */
             handleSignalLmDisconnectComplete(
                    &((LM_EV_DISCONNECT_COMPLETE_T *)p_event_data)->data);
        }
        break;

        case LM_EV_ADVERTISING_REPORT:
            CsrMeshProcessMessage((LM_EV_ADVERTISING_REPORT_T*)p_event_data);
        break;

        case LS_RADIO_EVENT_IND:
        {
            CsrMeshHandleRadioEvent();
        }
        break;

        default:
            /* Ignore any other event */
        break;

    }

    return TRUE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessCsrMeshEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a CSRmesh event
 *      is received by the system.
 *
 * PARAMETERS
 *      event_code csr_mesh_event_t
 *      data       Data associated with the event
 *      length     Length of the data
 *      state_data Pointer to the variable pointing to state data.
 *
 * RETURNS
 *      TRUE if the app has finished with the event data; the control layer
 *      will free the buffer.
 *----------------------------------------------------------------------------*/
extern void AppProcessCsrMeshEvent(csr_mesh_event_t event_code, uint8* data,
                                   uint16 length, void **state_data)
{
    bool   start_nvm_timer = FALSE;

    switch(event_code)
    {
        case CSR_MESH_ASSOCIATION_REQUEST:
        {
            if( g_lightapp_data.assoc_state != app_state_association_started)
            {
                g_lightapp_data.assoc_state = app_state_association_started;
            }
            TimerDelete(g_lightapp_data.mesh_device_id_advert_tid);

            /* Blink Light in Yellow to indicate association started */
            LightHardwareSetBlink(127, 127, 0, 32, 32);
        }
        break;

        case CSR_MESH_KEY_DISTRIBUTION:
        {
            DEBUG_STR("Association complete\r\n");
            g_lightapp_data.assoc_state = app_state_associated;

            /* Write association state to NVM */
            NvmWrite((uint16 *)&g_lightapp_data.assoc_state,
                     sizeof(g_lightapp_data.assoc_state),
                     NVM_OFFSET_ASSOCIATION_STATE);

            TimerDelete(g_lightapp_data.mesh_device_id_advert_tid);

            /* Restore light settings after association */
            LightHardwareSetColor(g_lightapp_data.light_state.red,
                                  g_lightapp_data.light_state.green,
                                  g_lightapp_data.light_state.blue);

            /* Restore power settings after association */
            LightHardwarePowerControl(g_lightapp_data.power.power_state);
        }
        break;

        case CSR_MESH_CONFIG_RESET_DEVICE:
        {
            DEBUG_STR("Reset Device\r\n");

            /* Move device to dissociated state */
            g_lightapp_data.assoc_state = app_state_not_associated;

            /* Write association state to NVM */
            NvmWrite((uint16 *)&g_lightapp_data.assoc_state,
                     sizeof(g_lightapp_data.assoc_state),
                     NVM_OFFSET_ASSOCIATION_STATE);

            /* Reset the supported model groups and save it to NVM */
            /* Light model */
            MemSet(light_model_groups, 0x0000, sizeof(light_model_groups));
            Nvm_Write((uint16 *)light_model_groups, sizeof(light_model_groups),
                                                     NVM_OFFSET_LIGHT_MODEL_GROUPS);

            /* Power model */
            MemSet(power_model_groups, 0x0000, sizeof(light_model_groups));
            Nvm_Write((uint16 *)power_model_groups, sizeof(power_model_groups),
                                                     NVM_OFFSET_LIGHT_MODEL_GROUPS);

            /* Attention model */
            MemSet(attention_model_groups, 0x0000, sizeof(light_model_groups));
            Nvm_Write((uint16 *)attention_model_groups,
                      sizeof(attention_model_groups),NVM_OFFSET_LIGHT_MODEL_GROUPS);

            /* Reset Light State */
            g_lightapp_data.light_state.red   = 0xFF;
            g_lightapp_data.light_state.green = 0xFF;
            g_lightapp_data.light_state.blue  = 0xFF;
            g_lightapp_data.light_state.power = POWER_STATE_OFF;
            g_lightapp_data.power.power_state = POWER_STATE_OFF;
            start_nvm_timer = TRUE;

            /* Start Mesh association again */
            smAppInitiateAssociation();
        }
        break;

        case CSR_MESH_CONFIG_DEVICE_IDENTIFIER:
        {
            DEBUG_STR("Device ID received:");
            DEBUG_U8(data[0]);
            DEBUG_U8(data[1]);
            DEBUG_STR("\r\n");
        }
        break;

        case CSR_MESH_GROUP_SET_MODEL_GROUPID:
        {
            handleCsrMeshGroupSetMsg(data, length);
        }
        break;

        case CSR_MESH_LIGHT_SET_LEVEL:
        {
            /* Update State of RGB in application */
            g_lightapp_data.light_state.red   = data[0];
            g_lightapp_data.light_state.green = data[0];
            g_lightapp_data.light_state.blue  = data[0];
            g_lightapp_data.light_state.power = POWER_STATE_ON;
            g_lightapp_data.power.power_state = POWER_STATE_ON;
            start_nvm_timer = TRUE;

            /* Set the White light level */
            LightHardwareSetLevel(data[0]);

            /* Send Light State Information to Model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.light_state;
            }

            /* Don't apply to hardware unless light is ON */
            DEBUG_STR("Set Level: ");
            DEBUG_U8(data[0]);
            DEBUG_STR("\r\n");
        }
        break;

        case CSR_MESH_LIGHT_SET_RGB:
        {
            /* Update State of RGB in application */
            g_lightapp_data.light_state.red   = data[1];
            g_lightapp_data.light_state.green = data[2];
            g_lightapp_data.light_state.blue  = data[3];
            g_lightapp_data.light_state.power = POWER_STATE_ON;
            g_lightapp_data.power.power_state = POWER_STATE_ON;
            start_nvm_timer = TRUE;

            /* Set Colour of light */
            LightHardwareSetColor(g_lightapp_data.light_state.red,
                                  g_lightapp_data.light_state.green,
                                  g_lightapp_data.light_state.blue);

            /* Send Light State Information to Model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.light_state;
            }

            DEBUG_STR("Set RGB : ");
            DEBUG_U8(data[1]);
            DEBUG_STR(",");
            DEBUG_U8(data[2]);
            DEBUG_STR(",");
            DEBUG_U8(data[3]);
            DEBUG_STR("\r\n");
        }
        break;

        case CSR_MESH_LIGHT_SET_COLOR_TEMP:
        {
#ifdef COLOUR_TEMP_ENABLED
            uint16 temp = 0;
            temp = ((uint16)data[1] << 8) | (data[0]);

            g_lightapp_data.power.power_state = POWER_STATE_ON;
            g_lightapp_data.light_state.power = POWER_STATE_ON;

            /* Set Colour temperature of light */
            LightHardwareSetColorTemp(temp);

            /* Send Light State Information to Model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.light_state;
            }

            DEBUG_STR("Set Colour Temperature: ");
            DEBUG_U16(temp);
            DEBUG_STR("\r\n");
#endif /* COLOUR_TEMP_ENABLED */
        }
        break;

        case CSR_MESH_LIGHT_GET_STATE:
        {
            /* Send Light State Information to Model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.light_state;
            }
        }
        break;

        case CSR_MESH_POWER_GET_STATE:
        {
            /* Send Power State Information to Model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.power;
            }
        }
        break;

        case CSR_MESH_POWER_TOGGLE_STATE:
        case CSR_MESH_POWER_SET_STATE:
        {
            if (CSR_MESH_POWER_SET_STATE == event_code)
            {
                g_lightapp_data.power.power_state = data[0];
            }
            else
            {
                togglePowerState();
            }

            DEBUG_STR("Set Power: ");
            DEBUG_U8(g_lightapp_data.power.power_state);
            DEBUG_STR("\r\n");

            if (g_lightapp_data.power.power_state == POWER_STATE_OFF ||
                g_lightapp_data.power.power_state == POWER_STATE_STANDBY)
            {
                LightHardwarePowerControl(FALSE);
            }
            else if(g_lightapp_data.power.power_state == POWER_STATE_ON ||
                    g_lightapp_data.power.power_state == \
                                                POWER_STATE_ON_FROM_STANDBY)
            {
                LightHardwareSetColor(g_lightapp_data.light_state.red,
                                      g_lightapp_data.light_state.green,
                                      g_lightapp_data.light_state.blue);

                /* Turn on with old colour value restored */
                LightHardwarePowerControl(TRUE);
            }

            g_lightapp_data.light_state.power =
                                            g_lightapp_data.power.power_state;

            /* Send Power State Information to Model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.power;
            }

            start_nvm_timer = TRUE;
        }
        break;

        case CSR_MESH_BEARER_SET_STATE:
        {
            uint8 *pData = data;
            g_lightapp_data.bearer_data.bearerRelayActive = BufReadUint16(&pData);
            g_lightapp_data.bearer_data.bearerEnabled     = BufReadUint16(&pData);

            if (g_lightapp_data.bearer_data.bearerRelayActive & BLE_BEARER_MASK)
            {
                CsrMeshRelayEnable(TRUE);
            }
            else
            {
                CsrMeshRelayEnable(FALSE);
            }

            /* Update Bearer Model Data to NVM */
            Nvm_Write((uint16 *)&g_lightapp_data.bearer_data,
                      sizeof(BEARER_MODEL_STATE_DATA_T), NVM_BEARER_DATA_OFFSET);
        }
        /* Fall through */
        case CSR_MESH_BEARER_GET_STATE:
        {
            /* Send Bearer Model data */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.bearer_data;
            }
        }
        break;

        case CSR_MESH_FIRMWARE_GET_VERSION_INFO:
        {
            /* Send Firmware Version data to the model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.fw_version;
            }
        }
        break;

#ifdef ENABLE_FIRMWARE_MODEL
        case CSR_MESH_FIRMWARE_UPDATE_REQUIRED:
        {
            DEBUG_STR("\r\n FIRMWARE UPDATE IN PROGRESS \r\n");

            /* Write the value CSR_OTA_BOOT_LOADER to NVM so that
             * it starts in OTA mode upon reset
             */
            OtaWriteCurrentApp(csr_ota_boot_loader,
                                  FALSE,/* is bonded */
                                  NULL, /* Typed host BD Address */
                                  0,    /* Diversifier */
                                  NULL, /* local_random_address */
                                  NULL, /* *irk */
                                  FALSE /* service_changed_config */
                                  );

            /* Add a small delay to ensure OTA values are written on NVM */
            TimeDelayUSec(1 * MILLISECOND);

            OtaReset();
        }
        break;
#endif /* ENABLE_FIRMWARE_MODEL */

#ifdef ENABLE_BATTERY_MODEL
        case CSR_MESH_BATTERY_GET_STATE:
        {
            /* Read Battery Level */
            g_lightapp_data.battery_data.battery_level = ReadBatteryLevel();

            /* Pass Battery state data to model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.battery_data;
            }
        }
        break;
#endif /* ENABLE_BATTERY_MODEL */

        case CSR_MESH_ATTENTION_SET_STATE:
        {
            uint32 dur_us;
            /* Read the data */
            g_lightapp_data.attn_data.attract_attn  = BufReadUint8(&data);
            g_lightapp_data.attn_data.attn_duration = BufReadUint16(&data);

            /* Delete attention timer if it exists */
            if (TIMER_INVALID != attn_tid)
            {
                TimerDelete(attn_tid);
                attn_tid = TIMER_INVALID;
            }

            /* If attention Enabled */
            if (g_lightapp_data.attn_data.attract_attn)
            {
                /* Create attention duration timer if required */
                if (g_lightapp_data.attn_data.attn_duration != 0xFFFF)
                {
                    dur_us = (uint32)g_lightapp_data.attn_data.attn_duration * \
                                     MILLISECOND;
                    attn_tid = TimerCreate(dur_us, TRUE, attnTimerHandler);
                }

                /* Enable Red light blinking to attract attention */
                LightHardwareSetBlink(127, 0, 0, 32, 32);
            }
            else
            {
                /* Restore Light State */
                LightHardwareSetColor(g_lightapp_data.light_state.red,
                                      g_lightapp_data.light_state.green,
                                      g_lightapp_data.light_state.blue);

                /* Restore the light Power State */
                LightHardwarePowerControl(g_lightapp_data.power.power_state);
            }

            /* Send response data to model */
            if (state_data != NULL)
            {
                *state_data = (void *)&g_lightapp_data.attn_data;
            }

            /* Debug logs */
            DEBUG_STR("\r\n ATTENTION_SET_STATE : Enable :");
            DEBUG_U8(g_lightapp_data.attn_data.attract_attn);
            DEBUG_STR("Duration : ");
            DEBUG_U16(g_lightapp_data.attn_data.attn_duration);
            DEBUG_STR("\r\n");

        }
        break;

        /* Received a raw message from lower-layers.
         * Notify to the control device if connected.
         */
        case CSR_MESH_RAW_MESSAGE:
        {
            if (g_lightapp_data.state == app_state_connected)
            {
                MeshControlNotifyResponse(g_lightapp_data.gatt_data.st_ucid,
                                          data, length);
            }
        }
        break;

        default:
        break;
    }

    /* Start NVM timer if required */
    if (TRUE == start_nvm_timer)
    {
        /* Delete existing timer */
        if (TIMER_INVALID != g_lightapp_data.nvm_tid)
        {
            TimerDelete(g_lightapp_data.nvm_tid);
        }

        /* Re-start the timer */
        g_lightapp_data.nvm_tid = TimerCreate(NVM_WRITE_DEFER_DURATION,
                                              TRUE,
                                              smLightDataNVMWriteTimerHandler);
    }
}

