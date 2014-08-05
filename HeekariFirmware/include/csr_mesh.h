/*! \file csr_mesh.h
 *  \brief CSRmesh library configuration and control functions
 *
 *   This file contains the functions to provide the application with
 *   access to the CSRmesh library
 *
 *   NOTE: This library includes the Mesh Transport Layer, Mesh Control
 *   Layer and Mesh Association Layer functionality.
 *
 *   Copyright (c) CSR plc 013
 */

#ifndef __CSR_MESH_H__
#define __CSR_MESH_H__

#include "types.h"
#include <bt_event_types.h>

/*! \addtogroup CSRmesh
 * @{
 */

/*============================================================================*
Public Definitions
*============================================================================*/
/*! \brief Number of timers required for CSRmesh library to be reserved by
 * the application.
 */
#define MAX_CSR_MESH_TIMERS         (4) /*!< \brief User application needs
                                          * to reserve these many timers along
                                          * with application timers.\n
                                          * Example:\n
                                          * \code #define MAX_APP_TIMERS (3 + MAX_CSR_MESH_TIMERS)
                                          * \endcode
                                          */

/*! \brief Number of words required by the CSRMesh library to save the internal
 *   configuration values on the NVM
 */
#define CSR_MESH_NVM_SIZE           (14)

/*! \struct LM_EV_ADVERTISING_REPORT_T
 *  \brief  LM Advertisement report structure. Refer to CSR uEnergy SDK API
 *   documentation for details
 */

/*! \brief 128 bit UUID type */
typedef struct
{
    uint16 uuid[8]; /*!< \brief CSRmesh 128 bit UUID */
} CSR_MESH_UUID_T;

/*! \brief 64 bit Authorisation Code type */
typedef struct
{
    uint16 auth_code[4]; /*!< \brief CSRmesh 64 bit Authorisation Code */
}CSR_MESH_AUTH_CODE_T;

/*! \brief CSRmesh Scan and Advertising Parameters */
typedef struct
{
    uint8   scan_duty_cycle;      /*!< \brief CSRmesh scan duty cycle */
    uint16  advertising_interval; /*!< \brief CSRmesh advertising interval in milliseconds */
    uint16  advertising_time;     /*!< \brief CSRmesh advertising time  in milliseconds */
}CSR_MESH_ADVSCAN_PARAM_T;


/*! \brief CSRmesh device power state */
typedef enum
{
    POWER_STATE_OFF = 0,            /*!< \brief Device is in OFF state */
    POWER_STATE_ON = 1,             /*!< \brief Device is in ON state */
    POWER_STATE_STANDBY = 2,        /*!< \brief Device is in STANDBY state */
    POWER_STATE_ON_FROM_STANDBY = 3 /*!< \brief Device returned to ON state from STANDBY
                                     * state
                                     */
}POWER_STATE_T;

/*! \brief CS User Key indices for CSRmesh internal settings */
typedef enum
{
    CSR_MESH_CS_USERKEY_INDEX_ADV_INTERVAL = 0,  /*!< \brief CS User Key index for setting the CSRmesh advertising interval */
    CSR_MESH_CS_USERKEY_INDEX_ADV_TIME = 1,      /*!< \brief CS User Key index for setting the CSRmesh advertising time, the time for which a message will be advertised */
}CONFIG_CS_KEYS_T;

/*! \brief CSRmesh Bearers */
typedef enum
{
    CSR_MESH_BEARER_BLE              = 0,  /*!< \brief Bluetooth Low Energy Bearer. */
    CSR_MESH_BEARER_BLE_GATT_SERVER  = 1   /*!< \brief Bluetooth Low Energy GATT Server (CSRmesh Control Service) Bearer. */
} CSR_MESH_BEARER_T;

/*! \brief CSRmesh Message types */
typedef enum
{
    CSR_MESH_MESSAGE_ASSOCIATION, /*!< \brief CSRmesh Association message. */
    CSR_MESH_MESSAGE_CONTROL      /*!< \brief CSRmesh Control message. */
} CSR_MESH_MESSAGE_T;

/*! \brief CSRmesh Model types */
typedef enum
{
    CSR_MESH_WATCHDOG_MODEL = 0,
    CSR_MESH_CONFIG_MODEL = 1,
    CSR_MESH_GROUP_MODEL = 2,
    CSR_MESH_KEY_MODEL = 3,
    CSR_MESH_SENSOR_MODEL = 4,
    CSR_MESH_ACTUATOR_MODEL = 5,
    CSR_MESH_ASSET_MODEL = 6,
    CSR_MESH_LOCATION_MODEL = 7,
    CSR_MESH_DATA_MODEL = 8,
    CSR_MESH_FIRMWARE_MODEL = 9,
    CSR_MESH_DIAGNOSTIC_MODEL = 10,
    CSR_MESH_BEARER_MODEL = 11,
    CSR_MESH_PING_MODEL = 12,
    CSR_MESH_BATTERY_MODEL = 13,
    CSR_MESH_ATTENTION_MODEL = 14,
    CSR_MESH_IDENTIFIER_MODEL = 15,
    CSR_MESH_WALLCLOCK_MODEL = 16,
    CSR_MESH_SEMANTIC_MODEL = 17,
    CSR_MESH_EFFECT_MODEL = 18,
    CSR_MESH_POWER_MODEL = 19,
    CSR_MESH_LIGHT_MODEL = 20,
    CSR_MESH_SWITCH_MODEL = 21,
    CSR_MESH_EVENT_MODEL = 22,
    CSR_MESH_VOLUME_MODEL = 23,
    CSR_MESH_IV_MODEL = 24,
    CSR_MESH_REMOTE_MODEL = 25,
    CSR_MESH_USER_MODEL = 26,
    CSR_MESH_TIMER_MODEL = 27,
}CSR_MESH_MODEL_TYPE_T;

/*! \brief CSRmesh event types */
typedef enum
{
    /* Device association event types */
    CSR_MESH_ASSOCIATION_REQUEST = 0x0001,               /*!< \brief Received when a control device sends association request*/
    CSR_MESH_KEY_DISTRIBUTION = 0x0002,                  /*!< \brief Received when association is complete and control device assigns a network key*/

    /* Config Model Events */
    CSR_MESH_CONFIG_WATCHDOG = 0x0100,                   /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_LAST_SEQUENCE_NUMBER = 0x0101,       /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_RESET_DEVICE = 0x0102,               /*!< \brief Received when control device sends reset command */
    CSR_MESH_CONFIG_DISCOVER_DEVICE = 0x0103,            /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_SET_DEVICE_IDENTIFIER = 0x0104,      /*!< \brief Received when a control device sets a device identifier*/
    CSR_MESH_CONFIG_DEVICE_IDENTIFIER = 0x0105,          /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_GET_DEVICE_MODELS = 0x0106,          /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_DEVICE_MODELS = 0x0107,              /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_GET_DEVICE_UUID_LOW = 0x0108,        /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_DEVICE_UUID_LOW = 0x0109,            /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_GET_DEVICE_UUID_HIGH = 0x010A,       /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_DEVICE_UUID_HIGH = 0x010B,           /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_SET_PARAMETERS = 0x010C,             /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_GET_PARAMETERS = 0x010D,             /*!< \brief Reserved for future use */
    CSR_MESH_CONFIG_PARAMETERS = 0x010E,                 /*!< \brief Reserved for future use */

    /* Group Model Events */
    CSR_MESH_GROUP_GET_NUMBER_OF_MODEL_GROUPIDS = 0x0110,/*!< \brief Reserved for future use */
    CSR_MESH_GROUP_NUMBER_OF_MODEL_GROUPIDS = 0x0111,    /*!< \brief Reserved for future use */
    CSR_MESH_GROUP_SET_MODEL_GROUPID = 0x0112,           /*!< \brief Received when a control device sends a group assignment command */
    CSR_MESH_GROUP_GET_MODEL_GROUPID = 0x0113,           /*!< \brief Reserved for future use */
    CSR_MESH_GROUP_MODEL_GROUPID = 0x0114,               /*!< \brief Reserved for future use */

    /* Ping Model Events */
    CSR_MESH_PING_RESPONSE        = 0x0119,              /*!< \brief Received in response to the ping sent from the device */

    /* Diagnostic Model Events */
    CSR_MESH_DIAGNOSTIC_TRAFFIC_STATS  = 0x011a,         /*!< \brief Received in response to the DIAGNOSTIC_RESET or DIAGNOSTIC_READ command sent from the device */

    /* Attention Model Events */
    CSR_MESH_ATTENTION_SET_STATE = 0x011b,               /*!< \brief Received on a device when a control device sends an ATTENTION_SET_STATE command */

    /* Power Model events */
    CSR_MESH_POWER_SET_STATE = 0x0131,                   /*!< \brief Received when a control device sends a power set state command */
    CSR_MESH_POWER_TOGGLE_STATE = 0x0133,                /*!< \brief Received when a control device sends a power toggle state command */
    CSR_MESH_POWER_GET_STATE = 0x0134,                   /*!< \brief Received when control device requests for device state */
    CSR_MESH_POWER_STATE = 0x0135,                       /*!< \brief Received on a control device when a device sends its own state in response
                                                          *   to a Power Get State or Power Set State commands
                                                          */
    /* Light Model Events */
    CSR_MESH_LIGHT_SET_LEVEL = 0x0141,                   /*!< \brief Received when a control device sends a Light Set Level command */
    CSR_MESH_LIGHT_SET_RGB = 0x0143,                     /*!< \brief Received when a control device sends a Light Set RGB command */
    CSR_MESH_LIGHT_SET_POWER_LEVEL = 0x0145,             /*!< \brief Received when a control device sends a light Set power level command */
    CSR_MESH_LIGHT_GET_STATE = 0x0146,                   /*!< \brief Received when a control device requests for the current light state of the device */
    CSR_MESH_LIGHT_STATE = 0x0147,                       /*!< \brief Received on a control device in response to a Light Set command or Light Get state command */
    CSR_MESH_LIGHT_SET_COLOR_TEMP = 0x0148,              /*!< \brief Received when a control device sends a Light Set Colour Temperature command */

    /* Data Stream Model Events */
    CSR_MESH_DATA_STREAM_DATA_IND = 0x0171,              /*!< \brief Received when a stream data is received. Received data will be queued to the
                                                          *          Rx Stream buffer. Application needs to call StreamDataRead to 
                                                          *          obtain the data
                                                          */
    CSR_MESH_DATA_STREAM_SEND_CFM = 0x0172,              /*!< \brief Received when the remote device confirms amount of stream data received */
    CSR_MESH_DATA_BLOCK_IND = 0x0173,                    /*!< \brief Received a block of data */
    CSR_MESH_DATA_STREAM_FLUSH_IND = 0x0174,             /*!< \brief Received Data stream Flush indicating data stream complete */

    /* Firmware Model Events */
    CSR_MESH_FIRMWARE_GET_VERSION_INFO = 0x01C0,         /*!< \brief Reserved for future use */
    CSR_MESH_FIRMWARE_VERSION_INFO = 0x01C1,             /*!< \brief Reserved for future use */
    CSR_MESH_FIRMWARE_UPDATE_REQUIRED = 0x01C2,          /*!< \brief Received when a control device requests for a firmware upgrade of the device */
    CSR_MESH_FIRMWARE_UPDATE_ACKNOWLEDGED = 0x01C3,      /*!< \brief Reserved for future use */
    CSR_MESH_FIRMWARE_BLOCK_REQ = 0x01C4,                /*!< \brief Reserved for future use */
    CSR_MESH_FIRMWARE_BLOCK = 0x01C5,                    /*!< \brief Reserved for future use */
    CSR_MESH_FIRMWARE_CHECKSUM = 0x01C6,                 /*!< \brief Reserved for future use */
    CSR_MESH_FIRMWARE_RESTART = 0x01C7,                  /*!< \brief Reserved for future use */

    /* Bearer Model Events */
    CSR_MESH_BEARER_SET_STATE = 0x0128,                  /*!< \brief Received on the device when a control device sends BEARER_SET_STATE command */
    CSR_MESH_BEARER_GET_STATE = 0x0129,                  /*!< \brief Received on the device when a control device sends BEARER_GET_STATE command */
    CSR_MESH_BEARER_STATE = 0x012A,                      /*!< \brief Received on a control device when a device sends its bearer status
                                                          *   in response to a BEARER_SET_STATE or BEARER_GET_STATE commands.
                                                          */

    CSR_MESH_BATTERY_GET_STATE = 0x012C,                 /*!< \brief Received when a control device requests for the battery status */

    CSR_MESH_RAW_MESSAGE = 0xFFFF                        /*!< \brief Received when a CSRmesh message is received if the application has enabled 
                                                          *   notification of raw messages 
                                                          */
}csr_mesh_event_t;

/*============================================================================*
Public Function Implementations
*============================================================================*/

/*----------------------------------------------------------------------------*
 * CsrMeshInit
 */
/*! \brief Initialise the CSRmesh library.
 * 
 *  This function initialises the CSRmesh library
 *
 *  \returns TRUE if the request was successful.
 */
/*----------------------------------------------------------------------------*/
extern bool CsrMeshInit(void);

/*----------------------------------------------------------------------------*
 * CsrMeshGetDeviceUUID
 */
/*! \brief Returns the CSRmesh library 128 bit UUID.
 *
 *  \param[in] uuid pointer to a CSR_MESH_UUID_T type
 *  \param[out] uuid pointer to the CSR_MESH_UUID_T of the device
 *
 *  \returns TRUE if request was successful.
 */
/*----------------------------------------------------------------------------*/
extern bool CsrMeshGetDeviceUUID(CSR_MESH_UUID_T *uuid);

/*----------------------------------------------------------------------------*
 * CsrMeshSetDeviceUUID
 */
/*! \brief Sets the 128 bit CSRmesh UUID of the device
 *
 *  This function sets 128 bit UUID of the device. The CSRmesh library uses
 *  this value to advertise itself for Mesh association.
 *
 *  \param uuid pointer to a CSR_MESH_UUID_T type
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void CsrMeshSetDeviceUUID(CSR_MESH_UUID_T *uuid);

/*----------------------------------------------------------------------------*
 * CsrMeshSetAuthorisationCode
 */
/*! \brief Sets the 64 bit Authorisation Code for CSRmesh network association.
 *
 *  This function sets the 64 bit authorisation code to be used during the
 *  CSRmesh  network association procedure.
 *
 *  \param code Pointer to a CSR_MESH_AUTH_CODE_T
 *
 *  \returns TRUE if the request was successful.
 */
/*----------------------------------------------------------------------------*/
extern bool CsrMeshSetAuthorisationCode(CSR_MESH_AUTH_CODE_T *code);

/*----------------------------------------------------------------------------*
 * CsrMeshGetDeviceID
 */
/*! \brief Gets the 16 bit Device Identifier of the CSRmesh device
 *
 *  Gets the 16 bit Device Identifier of the CSRmesh device.
 *
 *  \returns 16 bit Device Identifier. 0 if the device is not associated.
 */
/*----------------------------------------------------------------------------*/
extern uint16 CsrMeshGetDeviceID(void);

/*----------------------------------------------------------------------------*
 * CsrMeshReset
 */
/*! \brief Reset the CSRmesh library
 *
 *  Resets the CSRmesh library and clears all the network association data.\n
 *  If the device was associated, the association with the network will be lost
 *  and it cannot communicate with the network unless it is associated again
 *
 *  \returns TRUE if the request was successful.
 */
/*----------------------------------------------------------------------------*/
extern bool CsrMeshReset(void);

/*----------------------------------------------------------------------------*
 * CsrMeshStart
 */
/*! \brief Start the CSRmesh system
 *
 *  Starts processing of received CSRmesh messages.
 *
 *  \returns TRUE if the request was successful.
 */
/*----------------------------------------------------------------------------*/
extern bool CsrMeshStart(void);

/*----------------------------------------------------------------------------*
 * CsrMeshStop
 */
/*! \brief Stop the CSRmesh system
 *
 *  Stops the CSRmesh activity.
 *
 *  \param force_stop When this flag is set all the CSRmesh activities
 *  in progress would be stopped and it will return TRUE always.\n
 *  When this flag is cleared, CSRmesh activities will be stopped only
 *  when it is listening. Return status will indicate if it is stopped or not.
 *
 *  \returns TRUE if the request was successful.
 */
/*----------------------------------------------------------------------------*/
extern bool CsrMeshStop(bool force_stop);

/*----------------------------------------------------------------------------*
 * CsrMeshSendMessage
 */
/*! \brief Sends a CSRmesh message
 *
 *  This function transmits the given message over the Mesh
 *
 *  \param message Pointer to the message to be sent
 *  \param message_length Length of the message to be sent
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool CsrMeshSendMessage(uint8  *message,
                                 uint16  message_length);

/*----------------------------------------------------------------------------*
 * CsrMeshProcessMessage
 */
/*! \brief Called to process a CSRmesh Message. 
 *
 *   This function extracts the CSRmesh messages from the advertisement reports
 *   The application calls this function when it receives a 
 *   LM_EV_ADVERTISING_REPORT LM event. This function processes the 
 *   message if the report contains a Mesh message belonging to a known network 
 *   otherwise returns FALSE. The application can process the report if it
 *   is not a CSRmesh report.
 *
 *  \param report Advertisement report from the LM_EV_ADVERTISING_REPORT event
 *  received while scanning
 *
 *  \returns TRUE if the received packet is a CSRmesh message and
 *   belongs to a known network. FALSE otherwise
 */
 /*---------------------------------------------------------------------------*/
extern bool CsrMeshProcessMessage(LM_EV_ADVERTISING_REPORT_T* report);

/*----------------------------------------------------------------------------*
 * CsrMeshProcessRawMessage
 */
/*! \brief Called to process a Raw CSRmesh Message.
 *   
 *   This function processes the given message as complete CSRmesh packet.
 *   The application must call this function to process a raw CSRmesh message
 *   received over a GATT connection, in the bridge role. 
 *
 *  \param msg Pointer to message buffer.
 *  \param len length of message in bytes.
 *
 *  \returns TRUE if the received packet is a CSRmesh message and
 *   belongs to a known network. FALSE otherwise
 */
 /*---------------------------------------------------------------------------*/
extern bool CsrMeshProcessRawMessage(uint8 *msg, uint8 len);

/*----------------------------------------------------------------------------*
 * CsrMeshRelayEnable
 */
/*! \brief Enable or disable relaying of CSRmesh messages.
 *
 *  This function enables/disables relaying of CSRmesh Messages.\n
 *  If the CSRmesh relay is enabled, the device retransmits the received
 *  messages after decrementing the ttl(time to live) field of message.\n
 *  This can enabled when messages need to be propagated in the CSRmesh
 *  where devices are located beyond the radio range of individual devices.
 *
 *  \param enable TRUE enables relay
 *
 *  \returns Nothing
 */
/*-----------------------------------------------------------------------------*/
extern void CsrMeshRelayEnable(bool enable);

/*----------------------------------------------------------------------------*
 * CsrMeshIsRelayEnabled
 */
/*! \brief Called to check whether CSRmesh relay is enabled or not.
 *
 *  Returns the current relay status of the device.
 *
 *  \returns TRUE if relay is enabled.
 */
/*-----------------------------------------------------------------------------*/
extern bool CsrMeshIsRelayEnabled(void);

/*----------------------------------------------------------------------------*
 * CsrMeshEnableRawMsgEvent
 */
/*! \brief Enables or disables the notification of CSR_MESH_RAW_MESSAGE event.
 * 
 *   Enables/disables the notification of \ref CSR_MESH_RAW_MESSAGE
 *   event. This event is sent when a CSRmesh message not belonging to any
 *   enabled models is received. This can be used by the application for its own
 *   processing.
 *
 *   Typically Raw message event is enabled on a GATT bridge which supports
 *   Mesh Control service and notifies the received CSRmesh messages to the
 *   connected CSRmesh Control application.
 *
 *  \param enable TRUE enables the notification of the event
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void CsrMeshEnableRawMsgEvent(bool enable);

/*----------------------------------------------------------------------------*
 * CsrMeshNvmSetStartOffset
 */
/*! \brief Sets the NVM start offset to store the CSRmesh specific NVM data
 *
 *   Sets the NVM start offset to store the CSRmesh specific NVM data.
 *
 *   This function must be called only once after power on and before any other
 *   CSRmesh functions are called.
 *
 *  \param offset Word offset from NVM start where CSRmesh has to store its NVM
                  values
 *
 *  \returns Number of NVM words required to save CSRMesh internal
 *           configuration values.
 */
/*----------------------------------------------------------------------------*/
extern void CsrMeshNvmSetStartOffset(uint16 offset);

/*----------------------------------------------------------------------------*
 * CsrMeshNvmGetSize
 */
/*! \brief Returns number of NVM words needed by the CSRmesh library
 *
 *   Returns the number the of NVM words required to store the
 *   internal configuration values of the CSRMesh system.
 *
 *  \returns \ref CSR_MESH_NVM_SIZE
 */
/*----------------------------------------------------------------------------*/
extern uint16 CsrMeshNvmGetSize(void);

/*----------------------------------------------------------------------------*
 * CsrMeshAssociateToANetwork
 */
/*! \brief Advertises a CSRmesh device identification message. 
 *   
 *   Advertises a CSRmesh device identification message to enable the
 *   device to be associated to a CSRmesh Network.
 *   The application ready to be associated to a network must call this
 *   function periodically to advertise itself till it receives a
 *   \ref CSR_MESH_ASSOCIATION_REQUEST event.
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool CsrMeshAssociateToANetwork(void);

/*----------------------------------------------------------------------------*
 * AppProcessCsrMeshEvent
 */
/*! \brief Application function called to notify a CSRmesh event.
 *
 *  This application function is called by the CSRmesh library to notify a 
 *  CSRmesh message.\n
 *
 *  \param event_code CSRmesh Event code
 *  \param data Data associated with the event
 *  \param length Length of the associated data in bytes
 *  \param state_data Pointer to the variable pointing to state data.
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void AppProcessCsrMeshEvent(csr_mesh_event_t event_code, uint8* data,
                                   uint16 length, void **state_data);

/*----------------------------------------------------------------------------*
 * CsrMeshHandleDataInConnection
 */
/*! \brief Synchronizes the CSRmesh activity with the connection events.
 *
 *   Allows the CSRmesh to synchronise the Mesh advertisements with the 
 *   connection events when the application is connected to a central device.\n\n 
 *   The application must call this function when \n
 *   1. A GATT connection is established with the device(indicated by the
 *      LM_EV_CONNECTION_COMPLETE event).\n
 *         2. Connection parameters of a connection are updated (indicated by
 *      the LS_CONNECTION_PARAM_UPDATE_IND event).\n
 *         3. An existing GATT connection is dropped (indicated by the
 *      LM_EV_DISCONNECT_COMPLETE event).\n
 *
 *  \param ucid          The connection identifier passed by the firmware to the
 *               application during connection establishment.\n
 *                       If the function is being called because of GATT
 *                       disconnection, this parameter should be
 *                       GATT_INVALID_UCID(0xFFFF).
 *  \param conn_interval The connection interval being used for the connection.
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void CsrMeshHandleDataInConnection(uint16 ucid, uint16 conn_interval);

/*----------------------------------------------------------------------------*
 * CsrMeshHandleRadioEvent
 */
/*! \brief Adjusts the CSRmesh advertisement timings based on the connection
 *  radio events.
 *  
 *  This function allows the CSRmesh to adjust the CSRmesh advertisements and
 *  scan timing with the connection radio events.\n
 *  The application must call this function when a LS_RADIO_EVENT_IND is received.
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void CsrMeshHandleRadioEvent(void);

/*----------------------------------------------------------------------------*
 * CsrMeshGetAdvScanParam
 */
/*! \brief Read the current CSRmesh Advertise and Scan timing Parameters.
 *
 *  This function reads advertising and scanning parameters from CSRmesh
 *  library.
 *
 *  \param   param Pointer to a structure of Advertising and Scan timing
 *           parameters to be read into.
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void CsrMeshGetAdvScanParam(CSR_MESH_ADVSCAN_PARAM_T *param);

/*----------------------------------------------------------------------------*
 * CsrMeshSetAdvScanParam
 */
/*! \brief Set the CSRmesh Advertise and Scan Timing parameters
 *
 *  Application can set advertising and scanning parameters to CSRmesh
 *  library using this function.
 *
 *  \param   param Pointer to a structure containing Advertising and Scan
 *           timing parameters to be set.
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void CsrMeshSetAdvScanParam(CSR_MESH_ADVSCAN_PARAM_T *param);

/*----------------------------------------------------------------------------*
 * CsrMeshGetMessageTTL
 */
/*!  \brief Obtains the TTL value of the CSRmesh messages.
 *
 *   Returns the Time To Live value for Control or Association messages.
 *
 *   \param msg_type Type of CSRmesh (Association or Control) message.
 *
 *   \returns TTL value of CSRmesh messages.
 */
/*----------------------------------------------------------------------------*/
extern uint8 CsrMeshGetMessageTTL(CSR_MESH_MESSAGE_T msg_type);

/*----------------------------------------------------------------------------*
 * CsrMeshSetMessageTTL
 */
/*!  \brief Sets the TTL value of the CSRmesh messages to be sent.
 *
 *   Modifies the Time To Live value for Control or Association messages.
 *
 *   \param msg_type Type of CSRmesh (Association or Control) message.
 *
 *   \param ttl TTL value to be used for CSRmesh messages.
 *
 *   \returns Nothing.
 */
/*----------------------------------------------------------------------------*/
extern void CsrMeshSetMessageTTL(CSR_MESH_MESSAGE_T msg_type, uint8 ttl);

/*----------------------------------------------------------------------------*
 * CsrMeshGetTxPower
 */
/*!  \brief Obtain the Transmit Power level in dBm.
 *
 *   Returns the Transmit Power of CSRmesh messages in dBm.
 *
 *   \returns Power level in dBm
 */
/*----------------------------------------------------------------------------*/
extern int8 CsrMeshGetTxPower(void);

/*----------------------------------------------------------------------------*
 * CsrMeshSetTxPower
 */
/*!  \brief Sets the Transmit Power to the nearest value possible.
 *
 *   Modifies the Transmit Power of CSRmesh messages. CSR device supports
 *   a specific set of transmit power levels from -18dBm  to +8dBm.\n
 *   This function sets the transmit power to supported value nearest to the
 *   requested value and returns the set transmit power.\n
 *
 *   \param power Transmit power level in dBm.
 *
 *   \returns Set power level in dBm
 */
/*----------------------------------------------------------------------------*/
extern int8 CsrMeshSetTxPower(int8 power);

/*!@} */
#endif /* __CSR_MESH_H__ */

