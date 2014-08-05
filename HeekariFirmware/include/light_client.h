/*! \file light_client.h
 *  \brief Defines the functions implemented for CSRmesh light model 
 *   the CSRmesh library
 *
 *   This file defines the functions belonging to CSRmesh light_model
 *   
 *   Copyright (c) CSR plc 2013
 */
    
#ifndef __LIGHT_CLIENT_H__
#define __LIGHT_CLIENT_H__



/*! \addtogroup Light_Client
 * @{
 */

/*============================================================================*
Public Function Prototypes
*============================================================================*/

/*----------------------------------------------------------------------------*
 * LightSetLevel 
 */
/*! \brief Set the brightness level of the light identified by the DestDevID
 *
 *   Sends a command to set brightness level.\n
 *   If acknowledgement flag is enabled, response from destination device is
 *   reported in AppProcessCsrMeshEvent with the event CSR_MESH_LIGHT_STATE.\n
 *   The device needs to be associated to the network prior to calling this
 *   function.
 *
 *  \param DestDevID 16 bit device ID of the light whose colour needs to be set
 *  \param level 8bit value for brightness level to be set
 *  \param send_ack if TRUE the function sends a LIGHT_SET_LEVEL command with
 *   acknowledgement
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool LightSetLevel(uint16 DestDevID, uint8 level, bool send_ack);

/*----------------------------------------------------------------------------*
 * LightGetState
 */
/*! \brief Get the state of the light identified by the DestDevID
 *
 *   Sends a command to get state of a light or group of lights.\n
 *   State response from device is reported to application in
 *   AppProcessCsrMeshEvent with the event CSR_MESH_LIGHT_STATE.\n
 *   The device needs to be associated to the network prior to calling this
 *   function.
 *
 *  \param DestDevID 16 bit device ID of the light
 *
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool LightGetState(uint16 DestDevID);

/*----------------------------------------------------------------------------*
 * LightSetColor 
 */
/*! \brief Sends a CSRmesh Light Set Colour command to the device 
 *   identified by the DestDevID
 *
 *   Sends a command to set colour of a light or group of lights.\n
 *   If acknowledgement flag is enabled, response from destination device is
 *   reported in AppProcessCsrMeshEvent with the event CSR_MESH_LIGHT_STATE.\n
 *   The device needs to be associated to the network prior to calling this
 *   function.
 * 
 *  \param DestDevID 16 bit device ID of the light whose colour needs to be set
 *  \param red 8bit level of the red component of the light
 *  \param green 8bit level of the green component of the light
 *  \param blue 8bit level of the blue component of the light
 *  \param send_ack if TRUE the function sends a LIGHT_SET_RGB command with
 *   acknowledgement
 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool LightSetColor(uint16 DestDevID, uint8 red, uint8 green, uint8 blue, 
                                                                 bool send_ack);

/*----------------------------------------------------------------------------*
 * LightSetColorTemperature
 */
/*! \brief Sends a CSRmesh Light Set Colour Temperature command to the device
 *   identified by the DestDevID
 *
 *   Sends a command to set colour temperature of a light or group of lights.\n
 *   The device needs to be associated to the network prior to calling this
 *   function.
 *  
 *  \param DestDevID 16 bit device ID of the light whose colour needs to be set
 *  \param temp 16 bit colour temperature value in degrees Kelvin.

 *  \returns TRUE if the request was successful
 */
/*----------------------------------------------------------------------------*/
extern bool LightSetColorTemperature(uint16 DestDevID, uint16 temp);

/*----------------------------------------------------------------------------*
 * LightModelClientInit
 */
/*! \brief Initialises Light Model Client functionality.
 *
 *  This function enables the light model client functionality
 *  on the device.\n
 *  The events from the client are reported to application in
 *  AppProcessCsrMeshEvent.\n
 *
 *  \returns Nothing.
 */
/*----------------------------------------------------------------------------*/
extern void LightModelClientInit(void);

/*!@} */
#endif /* __LIGHT_CLIENT_H__ */
