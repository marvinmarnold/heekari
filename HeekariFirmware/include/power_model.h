/*! \file power_model.h
 *  \brief Defines the functions implemented in CSRmesh Power Model
 *
 *   Copyright (c) CSR plc 2013
 */

#ifndef __POWER_MODEL_H__
#define __POWER_MODEL_H__

/*! \addtogroup Power_Model
 * @{
 */

/*============================================================================*
Public Definitions
*============================================================================*/

/*! \brief CSRmesh Power Model state */
typedef struct
{
    POWER_STATE_T power_state;   /*!< \brief Power State of the device. */
} POWER_MODEL_STATE_DATA_T;

/*============================================================================*
Public Function Implementations
*============================================================================*/

/*----------------------------------------------------------------------------*
 * PowerModelInit
 */
/*! \brief Initialise the Power Model of the CSRmesh library
 *
 *   Registers the model handler with the CSRmesh. Sets the CSRmesh to report
 *   num_groups as the maximum number of groups supported for the model
 *
 *  \param group_id_list Pointer to a uint16 array to hold assigned group_ids. 
 *                       This must be NULL if no groups are supported
 *  \param num_groups Size of the group_id_list. This must be 0 if no groups
 *                    are supported.
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void PowerModelInit(uint16 *group_id_list, uint16 num_groups);

/*!@} */
#endif /* __POWER_MODEL_H__ */

