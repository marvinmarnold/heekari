/*****************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2014
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 *  FILE
 *      csr_mesh_switch.h
 *
 *  DESCRIPTION
 *      This file defines a simple implementation of CSRmesh switch device
 *
 *****************************************************************************/

#ifndef __CSR_MESH_SWITCH_H__
#define __CSR_MESH_SWITCH_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>
#include <bt_event_types.h>
#include <timer.h>

/*============================================================================*
 *  Local Header File
 *============================================================================*/
#ifdef ENABLE_BATTERY_MODEL
#include <battery_model.h>
#endif /* ENABLE_BATTERY_MODEL */
#include "csr_mesh_switch_gatt.h"
#include <bearer_model.h>

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Maximum number of words in central device IRK */
#define MAX_WORDS_IRK                 (8)

/* Bearer Masks */
#define BLE_BEARER_MASK               (0x1 << CSR_MESH_BEARER_BLE)
#define BLE_GATT_SERVER_BEARER_MASK   (0x1 << CSR_MESH_BEARER_BLE_GATT_SERVER)

/*============================================================================*
 *  Public Data Types
 *============================================================================*/

/* CSRmesh device association state */
typedef enum
{
    /* Application Initial State */
    app_state_not_associated = 0,
            
    /* Application state association started */
    app_state_association_started,
    
    /* Application state associated */
    app_state_associated,

} app_association_state;


typedef enum
{
    /* Application Initial State */
    app_state_init = 0,

    /* Enters when fast undirected advertisements are configured */
    app_state_fast_advertising,

    /* Enters when slow Undirected advertisements are configured */
    app_state_slow_advertising,

    /* Enters when connection is established with the host */
    app_state_connected,

    /* Enters when disconnect is initiated by the application */
    app_state_disconnecting,

    /* Enters when the application is not connected to remote host */
    app_state_idle

} app_state;

/* Structure defined for Central device IRK */
typedef struct
{
    uint16                         irk[MAX_WORDS_IRK];

} CENTRAL_DEVICE_IRK_T;

/* GATT Service Data Structure */
typedef struct
{
    /* TYPED_BD_ADDR_T of the host to which device is connected */
    TYPED_BD_ADDR_T                con_bd_addr;

    /* Track the UCID as Clients connect and disconnect */
    uint16                         st_ucid;

    /* Boolean flag to indicated whether the device is bonded */
    bool                           bonded;

    /* TYPED_BD_ADDR_T of the host to which device is bonded.  */
    TYPED_BD_ADDR_T                bonded_bd_addr;

    /* Diversifier associated with the LTK of the bonded device */
    uint16                         diversifier;

    /* Boolean flag to indicated whether to set white list with the bonded 
     * device. This flag is used in an interim basis while configuring 
     * advertisements
     */
    bool                           enable_white_list;

    /*Central Private Address Resolution IRK  Will only be used when
     *central device used resolvable random address. 
     */
    CENTRAL_DEVICE_IRK_T           central_device_irk;

    /* Value for which advertisement timer needs to be started */
    uint32                         advert_timer_value;

    /* Store timer id while doing 'UNDIRECTED ADVERTS' and for Idle timer 
     * in CONNECTED' states.
     */
    timer_id                       app_tid;

    /* When device is not connected, switch to fast advertisement mode,
     * after idle time.
     */
    timer_id                       idle_tid;

    /* Variable to store the current connection interval being used. */
    uint16                         conn_interval;

    /* Variable to store the current slave latency. */
    uint16                         conn_latency;

    /* Variable to store the current connection time-out value. */
    uint16                         conn_timeout;

    /* Variable to keep track of number of connection 
     * parameter update requests made.
     */
    uint8                          num_conn_update_req;

    /* Store timer id for Connection Parameter Update timer in Connected 
     * state
     */
    timer_id                       con_param_update_tid;

    /* Connection Parameter Update timer value. Upon a connection, it's started
     * for a period of TGAP_CPP_PERIOD, upon the expiry of which it's restarted
     * for TGAP_CPC_PERIOD. When this timer is running, if a GATT_ACCESS_IND is
     * received, it means, the central device is still doing the service discov-
     * -ery procedure. So, the connection parameter update timer is deleted and
     * recreated. Upon the expiry of this timer, a connection parameter update
     * request is sent to the central device.
     */
    uint32                         cpu_timer_value;

} APP_GATT_SERVICE_DATA_T;


/* CSRmesh Light application data structure */
typedef struct
{
    /* Application GATT data */
    APP_GATT_SERVICE_DATA_T        gatt_data;

    /* Current state of application */
    app_state                       state;

    /* CSRmesh Association State */
    app_association_state           assoc_state;

    /* CSRmesh transmit device id advertise timer id */
    timer_id                       mesh_device_id_advert_tid;

    /* Toggle switch de-bounce timer id */
    timer_id                       debounce_tid;

    /* Battery Level Data */
#ifdef ENABLE_BATTERY_MODEL
    BATTERY_MODEL_STATE_DATA_T     battery_data;
#endif

    /* Bearer Model Data */
    BEARER_MODEL_STATE_DATA_T      bearer_data;

} CSRMESH_SWITCH_DATA_T;


/*============================================================================*
 *  Public Data
 *============================================================================*/

/* CSRmesh switch application specific data */
extern CSRMESH_SWITCH_DATA_T g_switchapp_data;

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* This function is used to set the state of the application */
extern void AppSetState(app_state new_state);

/* This function contains handling of short button press */
extern void HandleShortButtonPress(void);

#endif /* __CSR_MESH_SWITCH_H__ */

