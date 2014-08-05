/*! \file ping_model.h
 *  \brief Defines the functions implemented in CSRmesh Ping Model
 *
 *   Copyright (c) CSR plc 2013
 */

#ifndef __PING_MODEL_H__
#define __PING_MODEL_H__

/*! \addtogroup Ping_Model
 * @{
 */

/*============================================================================*
Public Definitions
*============================================================================*/

/*============================================================================*
Public Function Implementations
*============================================================================*/

/*----------------------------------------------------------------------------*
 * PingModelInit
 */
/*! \brief Initialise the Ping Model of the CSRmesh library
 *
 *  This function enables the Ping model functionality on the device.\n
 *  The events from the model are reported to application in
 *  AppProcessCsrMeshEvent.\n
 *
 *  \returns Nothing
 */
/*----------------------------------------------------------------------------*/
extern void PingModelInit(void);

/*!@} */
#endif /* __PING_MODEL_H__ */

