/*! \file power_client.h
 *  \brief Defines the client functions implemented in the CSRmesh Power Model
 *   
 *   Copyright (c) CSR plc 2013
 */
    
#ifndef __POWER_CLIENT_H__
#define __POWER_CLIENT_H__

/*! \addtogroup Power_Client
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
 * PowerSetState
 */
/*! \brief Set the Power state of the target device or group of devices
 *
 *   Sends a command to set power state of a device or group of devices.\n
 *   If acknowledgement flag is enabled, response from destination device is
 *   reported in AppProcessCsrMeshEvent with the event CSR_MESH_POWER_STATE.\n
 *   The device needs to be associated to the network before this function
 *   is called.
 *
 *  \param devID Device or group identifier of the target device or group
 *               of devices 
 *  \param state new power state to set
 *  \param ack_needed  boolean to specify if an acknowledgement is required 
 *                     from the target
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool PowerSetState(uint16 devID, POWER_STATE_T state, bool ack_needed);

/*----------------------------------------------------------------------------*
 * PowerGetState
 */
/*! \brief Get the Power state of the destined device or group of devices
 *
 *   Sends a command to obtain power state of a device.\n
 *   Response from destination device is reported in AppProcessCsrMeshEvent
 *   with the event CSR_MESH_POWER_STATE.\n
 *   The device needs to be associated to the network before this function
 *   is called.
 *
 *  \param devID Device or group identifier of the target device or group
 *         of devices 
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool PowerGetState(uint16 devID );

/*----------------------------------------------------------------------------*
 * PowerModelClientInit
 */
/*! \brief Initialises Power Model Client functionality.
 *
 *  This function enables the power model client functionality
 *  on the device.\n
 *  The events from the model are reported to application in
 *  AppProcessCsrMeshEvent.\n
 *
 *  \returns Nothing.
 */
/*----------------------------------------------------------------------------*/
extern void PowerModelClientInit(void);

/*!@} */
#endif /* __POWER_CLIENT_H__ */
