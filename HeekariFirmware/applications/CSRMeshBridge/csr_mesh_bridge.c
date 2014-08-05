/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2014
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 *  FILE
 *      csr_mesh_bridge.c
 *
 *  DESCRIPTION
 *      This file defines a CSR Mesh bridge application supporting the
 *      CSR custom defined Mesh Control Service
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <main.h>
#include <pio.h>
#include <mem.h>
#include <Sys_events.h>
#include <Sleep.h>
#include <timer.h>
#include <security.h>
#include <gatt.h>
#include <gatt_prim.h>
#include <panic.h>
#include <nvm.h>
#include <buf_utils.h>
#include <config_store.h>

/*============================================================================*
 *  CSR Mesh Header Files
 *============================================================================*/
#include <csr_mesh.h>
#include <light_client.h>
#include <power_client.h>
#include <firmware_client.h>

/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"
#include "app_debug.h"
#include "app_gatt.h"
#include "mesh_control_service.h"
#include "gap_service.h"
#include "csr_mesh_bridge.h"
#include "csr_mesh_bridge_gatt.h"
#include "app_gatt_db.h"
#include "nvm_access.h"

#ifdef ENABLE_GATT_OTA_SERVICE
#include <csr_ota.h>
#include "csr_ota_service.h"
#include "gatt_service.h"
#endif /* ENABLE_GATT_OTA_SERVICE */
/*============================================================================*
 *  Private Definitions
 *============================================================================*/
/* Maximum number of timers */
#define MAX_APP_TIMERS                 (6 + MAX_CSR_MESH_TIMERS)

/* Advertisement Timer for sending device identification */
#define DEVICE_ID_ADVERT_TIMER_ID      (5 * SECOND)

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

/* NVM offset for NVM sanity word */
#define NVM_OFFSET_SANITY_WORD         (0)

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

/* NVM offset Application Data */
#define NVM_MAX_APP_MEMORY_WORDS       (NVM_OFFSET_ASSOCIATION_STATE + 1)

/*============================================================================*
 *  Public Data
 *============================================================================*/

/* CSR Mesh bridge application data instance */
CSRMESH_BRIDGE_DATA_T g_bridgeapp_data;

/*============================================================================*
 *  Private Data
 *============================================================================*/

/* Declare space for application timers. */
static uint16 app_timers[SIZEOF_APP_TIMER * MAX_APP_TIMERS];

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/
static void smBridgeDataInit(void);
static void readPersistentStore(void);
static void handleSignalLmEvConnectionComplete(
                                     LM_EV_CONNECTION_COMPLETE_T *p_event_data);
static void handleSignalLmConnectionUpdate(
                                       LM_EV_CONNECTION_UPDATE_T* p_event_data);
static void handleGapCppTimerExpiry(timer_id tid);


#ifdef DEBUG_ENABLE
/* UART Receive callback */
static uint16 UartDataRxCallback ( void* p_data, uint16 data_count,
                                   uint16* p_num_additional_words );
#endif

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *  NAME
 *      smBridgeDataInit
 *
 *  DESCRIPTION
 *      This function is called to initialise CSR Mesh bridge application
 *      data structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void smBridgeDataInit(void)
{
    /* Reset/Delete all the timers */
    TimerDelete(g_bridgeapp_data.gatt_data.app_tid);
    g_bridgeapp_data.gatt_data.app_tid = TIMER_INVALID;

    TimerDelete(g_bridgeapp_data.gatt_data.con_param_update_tid);
    g_bridgeapp_data.gatt_data.con_param_update_tid = TIMER_INVALID;
    g_bridgeapp_data.gatt_data.cpu_timer_value = 0;

    g_bridgeapp_data.gatt_data.st_ucid = GATT_INVALID_UCID;

    g_bridgeapp_data.gatt_data.enable_white_list = FALSE;

    g_bridgeapp_data.gatt_data.advert_timer_value = 0;

    /* Reset the connection parameter variables. */
    g_bridgeapp_data.gatt_data.conn_interval = 0;
    g_bridgeapp_data.gatt_data.conn_latency = 0;
    g_bridgeapp_data.gatt_data.conn_timeout = 0;

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

    nvm_offset = NVM_MAX_APP_MEMORY_WORDS + CSR_MESH_NVM_SIZE;

    /* Read the sanity word to check the validity of NVM contents */
    Nvm_Read(&nvm_sanity, sizeof(nvm_sanity),
             NVM_OFFSET_SANITY_WORD);

    if(nvm_sanity == NVM_SANITY_MAGIC)
    {
        /* Read Bonded Flag from NVM */
        Nvm_Read((uint16*)&g_bridgeapp_data.gatt_data.bonded,
                  sizeof(g_bridgeapp_data.gatt_data.bonded),
                  NVM_OFFSET_BONDED_FLAG);

        if(g_bridgeapp_data.gatt_data.bonded)
        {

            /* Bonded Host Typed BD Address will only be stored if bonded flag
             * is set to TRUE. Read last bonded device address.
             */
            Nvm_Read((uint16*)&g_bridgeapp_data.gatt_data.bonded_bd_addr,
                     sizeof(TYPED_BD_ADDR_T),
                     NVM_OFFSET_BONDED_ADDR);

            /* If device is bonded and bonded address is resolvable then read
             * the bonded device's IRK
             */
            if(GattIsAddressResolvableRandom(
                                &g_bridgeapp_data.gatt_data.bonded_bd_addr))
            {
                Nvm_Read(g_bridgeapp_data.gatt_data.central_device_irk.irk,
                         MAX_WORDS_IRK,
                         NVM_OFFSET_SM_IRK);
            }

        }
        else /* Case when we have only written NVM_SANITY_MAGIC to NVM but
              * didn't get bonded to any host in the last powered session
              */
        {
            g_bridgeapp_data.gatt_data.bonded = FALSE;
        }

        /* Read the diversifier associated with the presently bonded/last
         * bonded device.
         */
        Nvm_Read(&g_bridgeapp_data.gatt_data.diversifier,
                 sizeof(g_bridgeapp_data.gatt_data.diversifier),
                 NVM_OFFSET_SM_DIV);

        /* Read association state from NVM */
        NvmRead((uint16 *)&g_bridgeapp_data.assoc_state,
                sizeof(g_bridgeapp_data.assoc_state),
                NVM_OFFSET_ASSOCIATION_STATE);

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
        Nvm_Write(&nvm_sanity, sizeof(nvm_sanity),
                  NVM_OFFSET_SANITY_WORD);

        /* The device will not be bonded as it is coming up for the first
         * time
         */
        g_bridgeapp_data.gatt_data.bonded = FALSE;

        /* Write bonded status to NVM */
        Nvm_Write((uint16*)&g_bridgeapp_data.gatt_data.bonded,
                  sizeof(g_bridgeapp_data.gatt_data.bonded),
                  NVM_OFFSET_BONDED_FLAG);

        /* When the application is coming up for the first time after flashing
         * the image to it, it will not have bonded to any device. So, no LTK
         * will be associated with it. Hence, set the diversifier to 0.
         */
        g_bridgeapp_data.gatt_data.diversifier = 0;

        /* Write the same to NVM. */
        Nvm_Write(&g_bridgeapp_data.gatt_data.diversifier,
                  sizeof(g_bridgeapp_data.gatt_data.diversifier),
                  NVM_OFFSET_SM_DIV);

        /* The device will not be associated as it is coming up for the
         * first time
         */
        g_bridgeapp_data.assoc_state = app_state_not_associated;

        /* Write association state to NVM */
        NvmWrite((uint16 *)&g_bridgeapp_data.assoc_state,
                 sizeof(g_bridgeapp_data.assoc_state),
                 NVM_OFFSET_ASSOCIATION_STATE);

        /* If fresh NVM, write device name and length to NVM for the
         * first time.
         */
        GapInitWriteDataToNVM(&nvm_offset);
    }

#ifdef ENABLE_GATT_OTA_SERVICE
    /* Read GATT data from NVM */
    GattReadDataFromNVM(&nvm_offset);
#endif /* ENABLE_GATT_OTA_SERVICE */

    /* Read Mesh Control Service data from NVM if the devices are bonded and
     * update the offset with the number of words of NVM required by
     * this service
     */
    MeshControlReadDataFromNVM(&nvm_offset);
}

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

    if(g_bridgeapp_data.gatt_data.con_param_update_tid == tid)
    {

        g_bridgeapp_data.gatt_data.con_param_update_tid = TIMER_INVALID;
        g_bridgeapp_data.gatt_data.cpu_timer_value = 0;

        /*Handling signal as per current state */
        switch(g_bridgeapp_data.state)
        {

            case app_state_connected:
            {
                /* Increment the count for Connection Parameter Update
                 * requests
                 */
                ++ g_bridgeapp_data.gatt_data.num_conn_update_req;

                /* If it is first or second request, preferred connection
                 * parameters should be request
                 */
                if(g_bridgeapp_data.gatt_data.num_conn_update_req == 1 ||
                   g_bridgeapp_data.gatt_data.num_conn_update_req == 2)
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
                else if(g_bridgeapp_data.gatt_data.num_conn_update_req == 3 ||
                        g_bridgeapp_data.gatt_data.num_conn_update_req == 4)
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

                if(LsConnectionParamUpdateReq(&g_bridgeapp_data.gatt_data.con_bd_addr,
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
    if(g_bridgeapp_data.gatt_data.con_param_update_tid == tid)
    {
        g_bridgeapp_data.gatt_data.con_param_update_tid =
                           TimerCreate(TGAP_CPC_PERIOD, TRUE,
                                       requestConnParamUpdate);
        g_bridgeapp_data.gatt_data.cpu_timer_value = TGAP_CPC_PERIOD;
    }
}

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
        if(g_bridgeapp_data.gatt_data.bonded &&
            (!GattIsAddressResolvableRandom(&g_bridgeapp_data.gatt_data.bonded_bd_addr)))
        {
            /* If the device is bonded and bonded device address is not
             * resolvable random, configure White list with the Bonded
             * host address
             */
            if(LsAddWhiteListDevice(&g_bridgeapp_data.gatt_data.bonded_bd_addr)!=
                ls_err_none)
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
        TimerDelete(g_bridgeapp_data.gatt_data.app_tid);
        g_bridgeapp_data.gatt_data.app_tid = TIMER_INVALID;
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
    switch(g_bridgeapp_data.state)
    {
        case app_state_init:
        {
            if(p_event_data->result == sys_status_success)
            {
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
    switch(g_bridgeapp_data.state)
    {
        case app_state_fast_advertising:
        {
            /* Start slow adverts only if a physical switch toggle
             * is not being handled
             */
            AppSetState(app_state_slow_advertising);
        }
        break;

        case app_state_slow_advertising:
            /*AppSetState(app_state_idle); */
        break;

        case app_state_connected:
            /* The CSR Mesh could have been sending data on
             * advertisements so do not panic
             */
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
    g_bridgeapp_data.gatt_data.conn_interval = p_event_data->data.conn_interval;
    g_bridgeapp_data.gatt_data.conn_latency  = p_event_data->data.conn_latency;
    g_bridgeapp_data.gatt_data.conn_timeout  =
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
    switch(g_bridgeapp_data.state)
    {
        case app_state_fast_advertising:
        case app_state_slow_advertising:
        {
            if(p_event_data->result == sys_status_success)
            {
                /* Store received UCID */
                g_bridgeapp_data.gatt_data.st_ucid = p_event_data->cid;

                /* Store connected BD Address */
                g_bridgeapp_data.gatt_data.con_bd_addr = p_event_data->bd_addr;

                if((g_bridgeapp_data.gatt_data.bonded) &&
                   (GattIsAddressResolvableRandom(
                            &g_bridgeapp_data.gatt_data.bonded_bd_addr)) &&
                   (SMPrivacyMatchAddress(&p_event_data->bd_addr,
                           g_bridgeapp_data.gatt_data.central_device_irk.irk,
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

                    /* Inform CSR Mesh that we are connected now */
                    CsrMeshHandleDataInConnection(
                                    g_bridgeapp_data.gatt_data.st_ucid,
                                    g_bridgeapp_data.gatt_data.conn_interval);

                    /* Start Listening in CSR Mesh */
                    CsrMeshStart();

#ifdef ENABLE_GATT_OTA_SERVICE
                    /* If we are bonded to this host, then it may be appropriate
                     * to indicate that the database is not now what it had
                     * previously.
                     */
                    if(g_bridgeapp_data.gatt_data.bonded && 
                       (!MemCmp(&g_bridgeapp_data.gatt_data.bonded_bd_addr, 
                                &g_bridgeapp_data.gatt_data.con_bd_addr,
                          sizeof(g_bridgeapp_data.gatt_data.bonded_bd_addr))))
                    {
                        GattOnConnection();
                    }
#endif /* ENABLE_GATT_OTA_SERVICE */

                    /* Since CSR Mesh Switch application does not mandate
                     * encryption requirement on its characteristics, so the
                     * remote master may or may not encrypt the link. Start a
                     * timer  here to give remote master some time to encrypt
                     * the link and on expiry of that timer, send a connection
                     * parameter update request to remote side.
                     */

                    /* Don't request security as this causes connection issues
                     * with Android 4.4
                     *
                     *  SMRequestSecurityLevel(&g_bridgeapp_data.gatt_data.con_bd_addr);
                     */

                    /* If the current connection parameters being used don't
                     * comply with the application's preferred connection
                     * parameters and the timer is not running and , start timer
                     * to trigger Connection Parameter Update procedure
                     */
                    if((g_bridgeapp_data.gatt_data.con_param_update_tid ==
                                                            TIMER_INVALID) &&
                       (g_bridgeapp_data.gatt_data.conn_interval <
                                                 PREFERRED_MIN_CON_INTERVAL ||
                        g_bridgeapp_data.gatt_data.conn_interval >
                                                 PREFERRED_MAX_CON_INTERVAL
#if PREFERRED_SLAVE_LATENCY
                        || g_bridgeapp_data.gatt_data.conn_latency <
                                                 PREFERRED_SLAVE_LATENCY
#endif
                       )
                      )
                    {
                        /* Set the num of conn update attempts to zero */
                        g_bridgeapp_data.gatt_data.num_conn_update_req = 0;

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
                        g_bridgeapp_data.gatt_data.con_param_update_tid =
                                          TimerCreate(TGAP_CPP_PERIOD, TRUE,
                                                      handleGapCppTimerExpiry);
                        g_bridgeapp_data.gatt_data.cpu_timer_value =
                                                            TGAP_CPP_PERIOD;
                    }
                      /* Else at the expiry of timer Connection parameter
                       * update procedure will get triggered
                       */
                }
            }
            else
            {
                /* Move to app_state_idle state and wait for some user event
                 * to trigger advertisements
                 */
                AppSetState(app_state_slow_advertising);
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
    switch(g_bridgeapp_data.state)
    {
        case app_state_connected:
        {
            /* If keys are present, save them */
            if((p_event_data->keys)->keys_present & (1 << SM_KEY_TYPE_DIV))
            {
                /* Store the diversifier which will be used for accepting/
                 * rejecting the encryption requests.
                 */
                g_bridgeapp_data.gatt_data.diversifier = (p_event_data->keys)->div;

                /* Write the new diversifier to NVM */
                Nvm_Write(&g_bridgeapp_data.gatt_data.diversifier,
                          sizeof(g_bridgeapp_data.gatt_data.diversifier),
                          NVM_OFFSET_SM_DIV);
            }

            /* Store IRK if the connected host is using random resolvable
             * address. IRK is used afterwards to validate the identity of
             * connected host
             */
            if(GattIsAddressResolvableRandom(
                                    &g_bridgeapp_data.gatt_data.con_bd_addr)  &&
               ((p_event_data->keys)->keys_present & (1 << SM_KEY_TYPE_ID)))
            {
                MemCopy(g_bridgeapp_data.gatt_data.central_device_irk.irk,
                        (p_event_data->keys)->irk,
                        MAX_WORDS_IRK);

                /* If bonded device address is resolvable random
                 * then store IRK to NVM
                 */
                Nvm_Write(g_bridgeapp_data.gatt_data.central_device_irk.irk,
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
    switch(g_bridgeapp_data.state)
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
    switch(g_bridgeapp_data.state)
    {
        case app_state_connected:
        {
            if(p_event_data->status == sys_status_success)
            {
                /* Store bonded host information to NVM. This includes
                 * application and services specific information
                 */
                g_bridgeapp_data.gatt_data.bonded = TRUE;
                g_bridgeapp_data.gatt_data.bonded_bd_addr =
                                                   p_event_data->bd_addr;

                /* Write one word bonded flag */
                Nvm_Write((uint16*)&g_bridgeapp_data.gatt_data.bonded,
                          sizeof(g_bridgeapp_data.gatt_data.bonded),
                          NVM_OFFSET_BONDED_FLAG);

                /* Write typed bd address of bonded host */
                Nvm_Write((uint16*)&g_bridgeapp_data.gatt_data.bonded_bd_addr,
                          sizeof(TYPED_BD_ADDR_T),
                          NVM_OFFSET_BONDED_ADDR);

                /* Configure white list with the Bonded host address only
                 * if the connected host doesn't support random resolvable
                 * address
                 */
                if(!GattIsAddressResolvableRandom(
                    &g_bridgeapp_data.gatt_data.bonded_bd_addr))
                {
                    /* It is important to note that this application
                     * doesn't support reconnection address. In future, if
                     * the application is enhanced to support Reconnection
                     * Address, make sure that we don't add reconnection
                     * address to white list
                     */
                    if(LsAddWhiteListDevice(
                             &g_bridgeapp_data.gatt_data.bonded_bd_addr) !=
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
    switch(g_bridgeapp_data.state)
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
                if(g_bridgeapp_data.gatt_data.diversifier == p_event_data->div)
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
    switch(g_bridgeapp_data.state)
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
                (g_bridgeapp_data.gatt_data.num_conn_update_req <
                                        MAX_NUM_CONN_PARAM_UPDATE_REQS))
            {
                /* Delete timer if running */
                TimerDelete(g_bridgeapp_data.gatt_data.con_param_update_tid);

                g_bridgeapp_data.gatt_data.con_param_update_tid =
                                 TimerCreate(GAP_CONN_PARAM_TIMEOUT,
                                             TRUE, requestConnParamUpdate);
                g_bridgeapp_data.gatt_data.cpu_timer_value =
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
    switch(g_bridgeapp_data.state)
    {
        case app_state_connected:
        case app_state_disconnecting:
        {
            /* Store the new connection parameters. */
            g_bridgeapp_data.gatt_data.conn_interval =
                                            p_event_data->data.conn_interval;
            g_bridgeapp_data.gatt_data.conn_latency =
                                            p_event_data->data.conn_latency;
            g_bridgeapp_data.gatt_data.conn_timeout =
                                        p_event_data->data.supervision_timeout;

            CsrMeshHandleDataInConnection(g_bridgeapp_data.gatt_data.st_ucid,
                                       g_bridgeapp_data.gatt_data.conn_interval);

            DEBUG_STR("Parameter Update Complete: ");
            DEBUG_U16(g_bridgeapp_data.gatt_data.conn_interval);
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
    switch(g_bridgeapp_data.state)
    {
        case app_state_connected:
        {
            /* Delete timer if running */
            TimerDelete(g_bridgeapp_data.gatt_data.con_param_update_tid);
            g_bridgeapp_data.gatt_data.con_param_update_tid = TIMER_INVALID;
            g_bridgeapp_data.gatt_data.cpu_timer_value = 0;

            /* The application had already received the new connection
             * parameters while handling event LM_EV_CONNECTION_UPDATE.
             * Check if new parameters comply with application preferred
             * parameters. If not, application shall trigger Connection
             * parameter update procedure
             */

            if(g_bridgeapp_data.gatt_data.conn_interval <
                                                PREFERRED_MIN_CON_INTERVAL ||
               g_bridgeapp_data.gatt_data.conn_interval >
                                                PREFERRED_MAX_CON_INTERVAL
#if PREFERRED_SLAVE_LATENCY
               || g_bridgeapp_data.gatt_data.conn_latency <
                                                PREFERRED_SLAVE_LATENCY
#endif
              )
            {
                /* Set the num of conn update attempts to zero */
                g_bridgeapp_data.gatt_data.num_conn_update_req = 0;

                /* Start timer to trigger Connection Parameter Update
                 * procedure
                 */
                g_bridgeapp_data.gatt_data.con_param_update_tid =
                                TimerCreate(GAP_CONN_PARAM_TIMEOUT,
                                            TRUE, requestConnParamUpdate);
                g_bridgeapp_data.gatt_data.cpu_timer_value =
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
    switch(g_bridgeapp_data.state)
    {
        case app_state_connected:
        {
            /* GATT_ACCESS_IND indicates that the central device is still disco-
             * -vering services. So, restart the connection parameter update
             * timer
             */
             if(g_bridgeapp_data.gatt_data.cpu_timer_value == TGAP_CPC_PERIOD &&
                g_bridgeapp_data.gatt_data.con_param_update_tid != TIMER_INVALID)
             {
                TimerDelete(g_bridgeapp_data.gatt_data.con_param_update_tid);
                g_bridgeapp_data.gatt_data.con_param_update_tid =
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
    g_bridgeapp_data.gatt_data.conn_interval = 0;
    g_bridgeapp_data.gatt_data.conn_latency = 0;
    g_bridgeapp_data.gatt_data.conn_timeout = 0;

    CsrMeshHandleDataInConnection(GATT_INVALID_UCID, 0);

#ifdef ENABLE_GATT_OTA_SERVICE
    if(OtaResetRequired())
    {
        OtaReset();
    }
#endif /* ENABLE_GATT_OTA_SERVICE */

    /* Device has disconnected so stop all activities on mesh */
    CsrMeshStop(TRUE);

    /* LM_EV_DISCONNECT_COMPLETE event can have following disconnect
     * reasons:
     *
     * HCI_ERROR_CONN_TIMEOUT - Link Loss case
     * HCI_ERROR_CONN_TERM_LOCAL_HOST - Disconnect triggered by device
     * HCI_ERROR_OETC_* - Other end (i.e., remote host) terminated connection
     */

    /*Handling signal as per current state */
    switch(g_bridgeapp_data.state)
    {
        case app_state_connected:
        case app_state_disconnecting:
        {
            /* Link Loss Case */
            if(p_event_data->reason == HCI_ERROR_CONN_TIMEOUT)
            {
                DEBUG_STR("Link loss\r\n");
                /* Start undirected advertisements by moving to
                 * app_state_fast_advertising state
                 */
                AppSetState(app_state_fast_advertising);
            }
            else if(p_event_data->reason == HCI_ERROR_CONN_TERM_LOCAL_HOST)
            {
                DEBUG_STR("Disconnected Local\r\n");
                if(g_bridgeapp_data.state == app_state_connected)
                {
                    /* It is possible to receive LM_EV_DISCONNECT_COMPLETE
                     * event in app_state_connected state at the expiry of
                     * lower layers ATT/SMP timer leading to disconnect
                     */

                    /* Start undirected advertisements by moving to
                     * app_state_fast_advertising state
                     */
                    AppSetState(app_state_fast_advertising);
                }
                else
                {
                    /* Case when application has triggered disconnect */

                    if(g_bridgeapp_data.gatt_data.bonded)
                    {
                        /* If the device is bonded and host uses resolvable
                         * random address, the device initiates disconnect
                         * procedure if it gets reconnected to a different
                         * host,in which case device should trigger fast
                         * advertisements after disconnecting from the last
                         * connected host.
                         */
                        if(GattIsAddressResolvableRandom(
                                &g_bridgeapp_data.gatt_data.bonded_bd_addr) &&
                           (SMPrivacyMatchAddress(
                              &g_bridgeapp_data.gatt_data.con_bd_addr,
                               g_bridgeapp_data.gatt_data.central_device_irk.irk,
                               MAX_NUMBER_IRK_STORED,
                               MAX_WORDS_IRK) < 0))
                        {
                            AppSetState(app_state_fast_advertising);
                        }
                        else
                        {
                            /* Else move to app_state_idle state because of
                             * inactivity
                             */
                            AppSetState(app_state_slow_advertising);
                        }
                    }
                    else /* Case of Bonding/Pairing removal */
                    {
                        /* Start undirected advertisements by moving to
                         * app_state_fast_advertising state
                         */
                        AppSetState(app_state_fast_advertising);
                    }
                }

            }
            else /* Remote user terminated connection case */
            {
                DEBUG_STR("Disconnected Remote\r\n");
                /* If the device has not bonded but disconnected, it may just
                 * have discovered the services supported by the application or
                 * read some un-protected characteristic value like device name
                 * and disconnected. The application should be connectible
                 * because the same remote device may want to reconnect and
                 * bond. If not the application should be discoverable by other
                 * devices.
                 */
                if(!g_bridgeapp_data.gatt_data.bonded)
                {
                    AppSetState(app_state_fast_advertising);
                }
                else /* Case when disconnect is triggered by a bonded Host */
                {
                    AppSetState(app_state_slow_advertising);
                }
            }

        }
        break;

        default:
            /* Control should never come here */
            ReportPanic(app_panic_invalid_state);
        break;
    }
}


/*============================================================================*
 *  Public Function Implementations
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
    Nvm_Write((uint16*)&g_bridgeapp_data.gatt_data.bonded,
               sizeof(g_bridgeapp_data.gatt_data.bonded),
               NVM_OFFSET_BONDED_FLAG);


    /* Write Bonded address to NVM. */
    Nvm_Write((uint16*)&g_bridgeapp_data.gatt_data.bonded_bd_addr,
              sizeof(TYPED_BD_ADDR_T),
              NVM_OFFSET_BONDED_ADDR);

    /* Write the diversifier to NVM */
    Nvm_Write(&g_bridgeapp_data.gatt_data.diversifier,
              sizeof(g_bridgeapp_data.gatt_data.diversifier),
              NVM_OFFSET_SM_DIV);

    /* Store the IRK to NVM */
    Nvm_Write(g_bridgeapp_data.gatt_data.central_device_irk.irk,
             MAX_WORDS_IRK,
              NVM_OFFSET_SM_IRK);

    /* Store the Association State */
    Nvm_Write(g_bridgeapp_data.assoc_state,
             sizeof(g_bridgeapp_data.assoc_state),
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
    app_state old_state = g_bridgeapp_data.state;

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

                /* Initialise CSR mesh bridge data and services data structure
                 * while exiting Disconnecting state
                 */
                smBridgeDataInit();
            break;

            case app_state_fast_advertising:
            case app_state_slow_advertising:
                /* Common things to do whenever application exits
                 * APP_*_ADVERTISING state.
                 */
                GattStopAdverts();
                appAdvertisingExit();
            break;

            case app_state_connected:
                /* Do nothing */
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
        g_bridgeapp_data.state = new_state;

        /* Handle entering new state */
        switch (new_state)
        {
            case app_state_fast_advertising:
            {
                CsrMeshStop(TRUE);
                GattTriggerFastAdverts();
            }
            break;

            case app_state_slow_advertising:
            {
                GattStartAdverts(FALSE);
            }
            break;

            case app_state_idle:
            {
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

                /* Update battery status at every connection instance. It may
                 * not be worth updating timer more often, but again it will
                 * primarily depend upon application requirements
                 */
            }
            break;

            case app_state_disconnecting:
                GattDisconnectReq(g_bridgeapp_data.gatt_data.st_ucid);
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

    if(g_bridgeapp_data.gatt_data.bonded)
    {
        if(g_bridgeapp_data.state == app_state_connected)
        {
            if(!MemCmp(&g_bridgeapp_data.gatt_data.bonded_bd_addr,
                       &g_bridgeapp_data.gatt_data.con_bd_addr,
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

#ifdef USE_STATIC_RANDOM_ADDRESS
    /* Use static random address for the CSR mesh bridge. */
    GapSetStaticAddress();
#endif

    /* Initialise the application timers */
    TimerInit(MAX_APP_TIMERS, (void*)app_timers);

#ifdef DEBUG_ENABLE
    /* Initialize UART and configure with
     * default baud rate and port configuration.
     */
    DebugInit(1, UartDataRxCallback, NULL);

    /* UART Rx threshold is set to 1,
     * so that every byte received will trigger the rx callback.
     */
    UartRead(1, 0);
#endif

    DEBUG_STR("\r\nBridge Application\r\n");

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

    /* Initialize the gatt and gap data.
     * Needs to be done before readPersistentStore
     */
    smBridgeDataInit();

    /* Read persistent storage.
     * Call this before calling CsrMeshInit.
     */
    readPersistentStore();

    /* Set the CsrMesh NVM start offset.
     * Note: This function must be called before the CsrMeshInit()
     */
    CsrMeshNvmSetStartOffset(NVM_MAX_APP_MEMORY_WORDS);

    /* Initialize the CSR Mesh */
    CsrMeshInit();

    /* Disable Relay on Bridge */
    CsrMeshRelayEnable(FALSE);

    /* Enable Notify messages */
    CsrMeshEnableRawMsgEvent(TRUE);

    /* Start CSR Mesh */
    CsrMeshStart();

    /* Tell Security Manager module about the value it needs to initialize it's
     * diversifier to.
     */
    SMInit(g_bridgeapp_data.gatt_data.diversifier);

    /* Tell GATT about our database. We will get a GATT_ADD_DB_CFM event when
     * this has completed.
     */
    p_gatt_db_pointer = GattGetDatabase(&gatt_db_length);

    /* Initialise CSR Mesh bridge application State */
    g_bridgeapp_data.state = app_state_init;

    GattAddDatabaseReq(gatt_db_length, p_gatt_db_pointer);

}


/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessSystemEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a system event, such
 *      as a battery low notification, is received by the system.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void AppProcessSystemEvent(sys_event_id id, void *data)
{
    switch(id)
    {
        case sys_event_pio_changed:
            /* Nothing to do here */
        break;

        default:
            /* Ignore anything else */
        break;
    }
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessLmEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a LM-specific event is
 *      received by the system.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

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
 *      This user application function is called whenever a CSR Mesh event
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
    switch(event_code)
    {
        /* Received a raw message from lower-layers.
         * Notify to the control device if connected.
         */
        case CSR_MESH_RAW_MESSAGE:
        {
            if (g_bridgeapp_data.state == app_state_connected)
            {
                DEBUG_STR("\r\n Notify Over GATT : ");
                DEBUG_U16(length);
                DEBUG_STR("\r\n");
                MeshControlNotifyResponse(g_bridgeapp_data.gatt_data.st_ucid,
                                          data, length);
            }
        }
        break;

        default:
            /*
             * Do nothing here.
             */
        break;
    }
}

#ifdef DEBUG_ENABLE
/*----------------------------------------------------------------------------*
 * NAME
 *    UartDataRxCallback
 *
 * DESCRIPTION
 *     This callback is issued when data is received over UART. Application
 *     may ignore the data, if not required. For more information refer to
 *     the API documentation for the type "uart_data_out_fn"
 *
 * RETURNS
 *     The number of words processed, return data_count if all of the received
 *     data had been processed (or if application don't care about the data)
 *
 *----------------------------------------------------------------------------*/
static uint16 UartDataRxCallback ( void* p_data, uint16 data_count,
        uint16* p_num_additional_words )
{
    *p_num_additional_words = 1; /* Application needs 1 additional
                                       data to be received */

    return data_count;
}
#endif

