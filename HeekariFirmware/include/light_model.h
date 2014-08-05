/*! \file light_model.h
 *  \brief Defines CSRmesh Light Model specific functions of
 *   the CSRmesh library
 *
 *   This file contains the functions defined in the CSRmesh light model
 *
 *   Copyright (c) CSR plc 2013
 */

#ifndef __LIGHT_MODEL_H__
#define __LIGHT_MODEL_H__

/*! \addtogroup Light_Model
 * @{
 */

/*============================================================================*
    Public Definitions
 *============================================================================*/

/*! \brief CSRmesh Light Model state */
typedef struct
{
    uint8  power;      /*!< \brief Power State of the light. */
    uint8  level;      /*!< \brief Luminance component of the light. */
    uint8  red;        /*!< \brief Red component of colour light. */
    uint8  green;      /*!< \brief Green component of colour light. */
    uint8  blue;       /*!< \brief Blue component of colour light. */
    uint16 color_temp; /*!< \brief Colour temperature of colour light. */
    uint8  supports;   /*!< \brief Reserved for future use */
} LIGHT_MODEL_STATE_DATA_T;

/*============================================================================*
    Public Function Definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * LightModelInit
 */
/*! \brief Initialises the Light Model of the CSRMesh library
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
extern void LightModelInit(uint16 *group_id_list, uint16 num_groups);

/*!@} */
#endif /* __LIGHT_MODEL_H__ */

