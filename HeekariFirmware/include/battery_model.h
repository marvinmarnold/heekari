/*! \file battery_model.h
 *  \brief Defines CSRmesh Battery Model specific functions
 *   the CSRmesh library
 *
 *   This file contains the functions defined in the CSRmesh battery model
 *   
 *   Copyright (c) CSR plc 2013
 */
    
#ifndef __BATTERY_MODEL_H__
#define __BATTERY_MODEL_H__

/*! \addtogroup Battery_Model
 * @{
 */

/*============================================================================*
    Public Definitions
 *============================================================================*/

/*! \brief CSRmesh Battery Model state */
typedef struct
{
    uint8  battery_level;    /*!< \brief Percentage Level of the battery. */
    uint8  battery_state;    /*!< \brief Battery State. */
} BATTERY_MODEL_STATE_DATA_T;

/*============================================================================*
    Public Functions
 *============================================================================*/
/*----------------------------------------------------------------------------*
 * BatteryModelInit 
 */
/*! \brief Initialise the Battery Model of the CSRmesh library
 * 
 *  Initialises the battery model.\n
 *  Enables notification of battery model messages addressed to the device
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void BatteryModelInit(void);

/*!@} */
#endif /* __BATTERY_MODEL_H__ */

