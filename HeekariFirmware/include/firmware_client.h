/*! \file firmware_client.h
 *  \brief Defines the client functions implemented in CSRmesh Firmware Model 
 *   the CSRmesh library
 *   
 *   Copyright (c) CSR plc 2013
 */
    
#ifndef __FIRMWARE_CLIENT_H__
#define __FIRMWARE_CLIENT_H__

/*! \addtogroup Firmware_Client
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
 * FirmwareUpdateRequest
 */
/*! \brief Sends a firmware update request to a device.
 *
 *  Sends a firmware update request to destination device.\n
 *  The acknowledge for request is reported in AppProcessCsrMeshEvent with
 *  the event CSR_MESH_FIRMWARE_UPDATE_ACKNOWLEDGED.
 *
 *  \param devID Device identifier of the target device.
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool FirmwareUpdateRequest(uint16 devID);

/*----------------------------------------------------------------------------*
 * FirmwareGetVersion
 */
/*! \brief Queries version information of a device.
 *
 *  Sends a Firmware version query command to destination device.\n
 *  Firmware version response is reported to application in
 *  AppProcessCsrMeshEvent with the event CSR_MESH_FIRMWARE_VERSION_INFO.
 *
 *  \param devID Device identifier of the target device.
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool FirmwareGetVersion(uint16 devID);

/*----------------------------------------------------------------------------*
 * FirmwareModelClientInit
 */
/*! \brief Initialises Firmware Model Client functionality.
 *
 *  This function enables the firmware model client functionality
 *  on the device.\n
 *  The events from the client are reported to application in
 *  AppProcessCsrMeshEvent.\n
 *
 *  \returns Nothing.
 */
/*----------------------------------------------------------------------------*/
extern void FirmwareModelClientInit(void);

/*!@} */
#endif /* __FIRMWARE_CLIENT_H__ */
