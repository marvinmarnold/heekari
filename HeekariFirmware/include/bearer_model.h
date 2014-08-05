/*! \file bearer_model.h
 *  \brief Defines the functions implemented in CSRmesh Bearer Model
 *     
 *   Copyright (c) CSR plc 2014
 */
    
#ifndef __BEARER_MODEL_H__
#define __BEARER_MODEL_H__

/*! \addtogroup Bearer_Model
 * @{
 */

/*============================================================================*
Public Definitions
 *============================================================================*/

/*! \brief CSRmesh Bearer Model state */
typedef struct
{
    uint16 bearerRelayActive; /*!< \brief Mask of bearer Relay state.    */
    uint16 bearerEnabled;     /*!< \brief Mask of bearer Active state.   */
    uint8  promiscuous;       /*!< \brief Turns on/off promiscuous mode. */

} BEARER_MODEL_STATE_DATA_T;

/*============================================================================*
Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * BearerModelInit 
 */
/*! \brief Initialise the Bearer Model of the CSRmesh library
 *
 *  This function enables the bearer model functionality on the device.\n
 *  The events from the model are reported to application in
 *  AppProcessCsrMeshEvent.\n
 *
 *  \returns nothing
 */
/*----------------------------------------------------------------------------*/
extern void BearerModelInit(void);

/*!@} */
#endif /* __BEARER_MODEL_H__ */

