/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2014
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 *  FILE
 *      mesh_control_service.h
 *
 *  DESCRIPTION
 *      Header definitions for mesh control service
 *
 *****************************************************************************/

#ifndef __MESH_CONTROL_SERVICE_H__
#define __MESH_CONTROL_SERVICE_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>
#include <bt_event_types.h>

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Macros for NVM access */
#define NVM_MESH_CONTROL_MTL_CP_CCD_OFFSET                (0)
#define NVM_MESH_CONTROL_SERVICE_MEMORY_WORDS             (1)
#define MESH_LONGEST_MSG_LEN                              (25)

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* This function handles read operation on the Special Authentication service 
 * attributes maintained by the application
 */
extern void MeshControlHandleAccessRead(GATT_ACCESS_IND_T *p_ind);

/* This function handles write operations on the Special Authentication service 
 * attributes maintained by the application
 */
extern void MeshControlHandleAccessWrite(GATT_ACCESS_IND_T *p_ind);

/* This function reads the special authentication service data from NVM */
extern void MeshControlReadDataFromNVM(uint16 *p_offset);

/* This function is used to check if the handle belongs to the Special 
 * Authentication service
 */
extern bool MeshControlCheckHandleRange(uint16 handle);

/* This function notifies responses received on the mesh to the switch 
 * client
 */
extern void MeshControlNotifyResponse(uint16 ucid, uint8 *mtl_msg, uint8 length);


#endif /* __MESH_CONTROL_SERVICE_H__ */

