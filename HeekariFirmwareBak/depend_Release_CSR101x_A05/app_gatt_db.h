/*
 * THIS FILE IS AUTOGENERATED, DO NOT EDIT!
 *
 * generated by gattdbgen from depend_Release_CSR101x_A05/app_gatt_db.db_
 */
#ifndef __APP_GATT_DB_H
#define __APP_GATT_DB_H

#include <main.h>

#define HANDLE_GAP_SERVICE              (0x0001)
#define HANDLE_GAP_SERVICE_END          (0x0007)
#define ATTR_LEN_GAP_SERVICE            (2)
#define HANDLE_DEVICE_NAME              (0x0003)
#define ATTR_LEN_DEVICE_NAME            (1)
#define HANDLE_DEVICE_APPEARANCE        (0x0005)
#define ATTR_LEN_DEVICE_APPEARANCE      (2)
#define HANDLE_GATT_SERVICE             (0x0008)
#define HANDLE_GATT_SERVICE_END         (0x0008)
#define ATTR_LEN_GATT_SERVICE           (2)
#define HANDLE_DEVICE_INFO_SERVICE      (0x0009)
#define HANDLE_DEVICE_INFO_SERVICE_END  (0xffff)
#define ATTR_LEN_DEVICE_INFO_SERVICE    (2)
#define HANDLE_DEVICE_INFO_SERIAL_NUMBER (0x000b)
#define ATTR_LEN_DEVICE_INFO_SERIAL_NUMBER (17)
#define HANDLE_DEVICE_INFO_HARDWARE_REVISION (0x000d)
#define ATTR_LEN_DEVICE_INFO_HARDWARE_REVISION (11)
#define HANDLE_DEVICE_INFO_FIRMWARE_REVISION (0x000f)
#define ATTR_LEN_DEVICE_INFO_FIRMWARE_REVISION (21)
#define HANDLE_DEVICE_INFO_SOFTWARE_REVISION (0x0011)
#define ATTR_LEN_DEVICE_INFO_SOFTWARE_REVISION (27)
#define HANDLE_DEVICE_INFO_MANUFACTURER_NAME (0x0013)
#define ATTR_LEN_DEVICE_INFO_MANUFACTURER_NAME (23)

extern uint16 *GattGetDatabase(uint16 *len);

#endif

/* End-of-File */
