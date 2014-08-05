/*! \file switch_model.h
 *  \brief Defines CSRmesh Switch Model specific functions
 *   the CSRmesh library
 *
 *   This file contains the functions defined in the CSRmesh switch model
 *   
 *   Copyright (c) CSR plc 2013
 */
    
#ifndef __SWITCH_MODEL_H__
#define __SWITCH_MODEL_H__

/*! \addtogroup Switch_Model
 * @{
 */

/*============================================================================*
Public Definitions
*============================================================================*/

/*----------------------------------------------------------------------------*
 * SwitchModelInit 
 */
/*! \brief Initialise the Switch Model of the CSRMesh library.
 *
 *    Registers the Switch model with the CSRmesh and processes messages related
 *    to the model. It reports the number of instances it can support as given 
 *    in the parameter \b instances when a control device queries the device
 *
 *  \param group_id_list Array to hold the destination groups assigned to the
 *    the switch
 *  \param instances Number of instances the application supports.
 *    The application can set the number of instances to the number of 
 *    physical switches on the target device. Each physical switch can then
 *    be assigned to control a group of devices.
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void SwitchModelInit(uint16 *group_id_list, uint16 instances);

/*!@} */
#endif /* __SWITCH_MODEL_H__ */

