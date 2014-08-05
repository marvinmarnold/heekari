/*! \file bearer_client.h
 *  \brief Defines the client functions implemented in the CSRmesh Bearer Model
 *   
 *   Copyright (c) CSR plc 2014
 */
    
#ifndef __BEARER_CLIENT_H__
#define __BEARER_CLIENT_H__

/*! \addtogroup Bearer_Client
 * @{
 */

/*============================================================================*
 *  CSRmesh Header files.
 *============================================================================*/
#include <bearer_model.h>

/*============================================================================*
 *  Private Data Type
 *============================================================================*/

/*============================================================================*
Public Definitions
*=============================================================================*/


/*============================================================================*
Public Function Implementations
*=============================================================================*/

/*----------------------------------------------------------------------------*
 * BearerSetState
 */
/*! \brief Set the Bearer state of the target device.
 *
 *   Sends a set bearer state command to destination device.\n
 *   State response is reported to application in AppProcessCsrMeshEvent
 *   with the event CSR_MESH_BEARER_STATE.\n
 *   The device needs to be associated to the network before this function
 *   is called.
 *
 *  \param devID Device identifier of the target device.
 *  \param pBearerState Pointer to the bearer state to set.
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool BearerSetState(uint16 devID,
                           BEARER_MODEL_STATE_DATA_T *pBearerState);

/*----------------------------------------------------------------------------*
 * BearerGetState
 */
/*! \brief Get the Bearer state of the destined device.
 *
 *   Sends a command to get bearer state from destination device.\n
 *   State response from destination device is reported to application in
 *   AppProcessCsrMeshEvent with the event CSR_MESH_BEARER_STATE.\n
 *   The device needs to be associated to the network before this function
 *   is called.
 *
 *  \param devID Device or group identifier of the target device.
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool BearerGetState(uint16 devID);

/*----------------------------------------------------------------------------*
 * BearerModelClientInit
 */
/*! \brief Initialises Bearer Model Client functionality.
 *
 *  This function enables the bearer model client functionality
 *  on the device.\n
 *  The events from the client are reported to application in
 *  AppProcessCsrMeshEvent.\n
 *
 *  \returns Nothing.
 */
/*----------------------------------------------------------------------------*/
extern void BearerModelClientInit(void);

/*!@} */
#endif /* __BEARER_CLIENT_H__ */

