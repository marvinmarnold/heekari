/******************************************************************************
 *
 *  FILE
 *      switch_uuids.h
 *
 * DESCRIPTION
 *      UUID MACROs for Battery service
 *
 *****************************************************************************/

#ifndef __SWITCH_UUIDS_H__
#define __SWITCH_UUIDS_H__

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Brackets should not be used around the value of a macro. The parser which 
 * creates .c and .h files from .db file doesn't understand brackets and will
 * raise syntax errors. 
 */

/* For UUID values, refer http://developer.bluetooth.org/gatt/services/
 * Pages/ServiceViewer.aspx?u=org.bluetooth.service.battery_service.xml
 */

#define UUID_SWITCH_SERVICE                          0x181A

#define UUID_SWITCH_INTENSITY                        0x2C44

#endif /* __SWITCH_UUIDS_H__ */
