/*! \file ping_client.h
 *  \brief Defines the client functions implemented in the CSRmesh Ping Model
 *   
 *   Copyright (c) CSR plc 2013
 */
    
#ifndef __PING_CLIENT_H__
#define __PING_CLIENT_H__

/*! \addtogroup Ping_Client
 * @{
 */

/*============================================================================*
 *  Private Data Type
 *===========================================================================*/ 

/*============================================================================*
Public Definitions
*============================================================================*/

/*============================================================================*
Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * PingModelClientInit
 */
/*! \brief Initialises Ping Model Client functionality.
 *
 *  This function enables the ping model client functionality
 *  on the device.\n
 *  The events from the client are reported to application in
 *  AppProcessCsrMeshEvent.\n
 *
 *  \returns Nothing.
 */
/*----------------------------------------------------------------------------*/
extern void PingModelClientInit(void);

/*----------------------------------------------------------------------------*
 * PingDevice
 */
/*! \brief Ping a device to get the RSSI and TTL at the destination.
 *
 *   Sends a ping to the destination device.\n
 *   Response to the ping is reported to application in AppProcessCsrMeshEvent
 *   with the event CSR_MESH_PING_RESPONSE.\n
 *   The device needs to be associated to the network before this function
 *   is called.
 *
 *  \param destDevID Device identifier of the target device.
 *
 *  \param arb_data Arbitrary data (4-octets) from the application. This data is
 *  transparent to the model and set by application. Applications needs to
 *  send a 4-octet array of uint8 to the function.
 *
 *  \param ttl Time To Live value of the Ping response message.
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool PingDevice(uint16 destDevID, uint8 *arb_data, uint8 ttl);

/*!@} */
#endif /* __PING_CLIENT_H__ */

