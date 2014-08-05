/*! \file firmware_model.h
 *  \brief Defines functions implemented in the CSRmesh Firmware Model 
 *
 *   This file defines the functions implemented in CSRmesh Firmware Model
 *   
 *   Copyright (c) CSR plc 2013
 */
    
#ifndef __FIRMWARE_MODEL_H__
#define __FIRMWARE_MODEL_H__

/*! \addtogroup Firmware_Model
 * @{
 */

/*============================================================================*
Public Definitions
*============================================================================*/

/*! \brief CSRmesh Firmware Model state */
typedef struct
{
    uint16 major_version;   /*!< \brief Major version number of firmware */
    uint16 minor_version;   /*!< \brief Minor version number of firmware */
    uint16 reserved[2];     /*!< \brief Reserved for Future Use */
} FIRMWARE_MODEL_STATE_DATA_T;

/*============================================================================*
Public Function Implementations
*============================================================================*/

/*----------------------------------------------------------------------------*
 * FirmwareModelInit 
 */
/*! \brief Initialise the Firmware Model
 *
 *  This function enables the Firmware model functionality on the device.\n
 *  The events from the model are reported to application in
 *  AppProcessCsrMeshEvent.\n
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void FirmwareModelInit(void);

/*!@} */
#endif /* __FIRMWARE_MODEL_H__ */

