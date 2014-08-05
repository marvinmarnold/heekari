/*! \file attention_model.h
 *  \brief Defines the functions implemented in CSRmesh Attention Model
 *
 *   Copyright (c) CSR plc 2014
 */

#ifndef __ATTENTION_MODEL_H__
#define __ATTENTION_MODEL_H__

/*! \addtogroup Attention_Model
 * @{
 */

/*============================================================================*
Public Definitions
*============================================================================*/
/*! \brief CSRmesh Attention Model state */
typedef struct
{
    bool   attract_attn;  /*!< \brief Enable/disable attracting of attention */
    uint16 attn_duration; /*!< \brief Duration for attracting attention */
} ATTENTION_MODEL_STATE_DATA_T;

/*============================================================================*
Public Function Implementations
*============================================================================*/

/*----------------------------------------------------------------------------*
 * AttentionModelInit
 */
/*! \brief Initialise the Attention Model of the CSRmesh library
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
extern void AttentionModelInit(uint16 *group_id_list, uint16 num_groups);

/*!@} */
#endif /* __ATTENTION_MODEL_H__ */

