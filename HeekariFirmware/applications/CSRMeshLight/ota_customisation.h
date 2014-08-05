/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2014
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 *  FILE
 *      ota_customisation.h
 *
 *  DESCRIPTION
 *      Customisation requirements for the CSR OTAU functionality.
 *
 *****************************************************************************/

#ifndef __OTA_CUSTOMISATION_H__
#define __OTA_CUSTOMISATION_H__

/*=============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "user_config.h"

#ifdef ENABLE_GATT_OTA_SERVICE
/* ** CUSTOMISATION **
 * The following header file names may need altering to match your application.
 */

#include "app_gatt.h"
#include "app_gatt_db.h"
#include "csr_mesh_light.h"

/*=============================================================================*
 *  Private Definitions
 *============================================================================*/

/* ** CUSTOMISATION **
 * Change these definitions to match your application.
 */
#define CONNECTION_CID      g_lightapp_data.gatt_data.st_ucid
#define IS_BONDED           g_lightapp_data.gatt_data.bonded
#define CONN_CENTRAL_ADDR   g_lightapp_data.gatt_data.con_bd_addr
#define CONNECTION_IRK      g_lightapp_data.gatt_data.central_device_irk.irk
#define LINK_DIVERSIFIER    g_lightapp_data.gatt_data.diversifier

/* Uncomment the following if this application is using a static random address.
 */
/*#define USE_STATIC_RANDOM_ADDRESS*/

/* Uncomment the following if this application is using a resolvable random 
 * address.
 */
/*#define USE_RESOLVABLE_RANDOM_ADDRESS*/

/* Do not enable both addressing types */
#if defined(USE_STATIC_RANDOM_ADDRESS) && defined(USE_RESOLVABLE_RANDOM_ADDRESS)
#error "Choose just one addressing type"
#endif /* USE_STATIC_RANDOM_ADDRESS && USE_RESOLVABLE_RANDOM_ADDRESS */

#endif /* ENABLE_GATT_OTA_SERVICE */

#endif /* __OTA_CUSTOMISATION_H__ */

