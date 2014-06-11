/******************************************************************************
 *  Copyright (C) Cambridge Silicon Radio Limited 2012-2013
 *  Part of CSR uEnergy SDK 2.2.2
 *  Application version 2.2.2.0
 *
 *  FILE
 *      heart_rate_service.c
 *
 * DESCRIPTION
 *      This file defines routines for using Heart rate service.
 *
 ****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <gatt.h>
#include <gatt_prim.h>
#include <buf_utils.h>

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "app_gatt.h"
#include "heart_rate_service.h"
#include "app_gatt_db.h"
#include "nvm_access.h"
#include "hr_sensor.h"

/* added this*/
#include <pio.h>

/*============================================================================*
 *  Private Data Types
 *============================================================================*/

/*Heart Rate service data type */
typedef struct
{
    /* Energy Expended Value */
    uint16                         energy_expended; 

    /* Heart rate measurement client configuration */
    gatt_client_config             hr_meas_client_config; 

    uint8                          switch_intensity;

    /* Offset at which Battery data is stored in NVM */
    uint16                         nvm_offset;

} HR_SERV_DATA_T;


/*============================================================================*
 *  Private Data
 *============================================================================*/

/* Heart Rate service data instance */
static HR_SERV_DATA_T g_hr_serv_data;

/* Heart Rate Control Point Type */
typedef enum
{

    hr_control_point_reserved      = 0x00,
    hr_control_point_reset_energy  = 0x01

} hr_control_point;

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Number of words of memory used by Heart Rate Service */
#define HEART_RATE_SERVICE_NVM_MEMORY_WORDS               (3)

/* The offset of data being stored in NVM for HR service. This offset is 
 * added to HR service offset to NVM region to get the absolute offset
 * at which this data is stored in NVM
 */
#define HR_NVM_HR_MEAS_CLIENT_CONFIG_OFFSET               (0)

#define HR_NVM_ENERGY_EXPENDED_OFFSET                     (1)

#define HR_NVM_SWITCH_OFFSET                              (2)

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      HRDataInit
 *
 *  DESCRIPTION
 *      This function is used to initialise heart rate service data 
 *      structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HRDataInit(void)
{
    if(!AppIsDeviceBonded())
    {
        g_hr_serv_data.hr_meas_client_config = gatt_client_config_none;
    }

    /* As per section 3.1.1.3 of HR Service spec ver 1.0, "The energy expended 
     * field represents the accumulated energy expended in kilo Joules since the 
     * last time it was reset". So, there is a need to maintain energy expended 
     * characteristic value across power cycles. As per application design, it 
     * will be sufficient to write energy expended value whenever device 
     * disconnects with the remote host (bonded or not-bonded).
     */

    /* Write Energy Expended to NVM */
    Nvm_Write(&g_hr_serv_data.energy_expended, 
              sizeof(g_hr_serv_data.energy_expended),
              g_hr_serv_data.nvm_offset + 
              HR_NVM_ENERGY_EXPENDED_OFFSET);

    WriteSwitchIntensity();
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HRInitChipReset
 *
 *  DESCRIPTION
 *      This function is used to initialise heart rate service data 
 *      structure at chip reset
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HRInitChipReset(void)
{
    /* Reset energy expended value at chip reset for initialisation */
    g_hr_serv_data.energy_expended = 0;

    g_hr_serv_data.switch_intensity = 0;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      IsHeartRateNotifyEnabled
 *
 *  DESCRIPTION
 *      This function returns whether notifications are enabled for heart rate 
 *      measurement characteristic.
 *
 *  RETURNS
 *      TRUE/FALSE: Notification configured or not
 *
 *---------------------------------------------------------------------------*/

extern bool IsHeartRateNotifyEnabled(void)
{
    return (g_hr_serv_data.hr_meas_client_config == 
                    gatt_client_config_notification);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HeartRateIncEnergyExpended
 *
 *  DESCRIPTION
 *      This function increments energy expended value for every HR measurement
 *
 *  RETURNS
 *      Nothing
 *
 *---------------------------------------------------------------------------*/

extern void HeartRateIncEnergyExpended(uint16 energy_exp)
{
    /* Energy expended in Kilo Joules */

    if(((uint32)g_hr_serv_data.energy_expended + energy_exp) > 
                            MAX_ENERGY_EXPENDED_IN_KJOULES)
    {
        /* Since Energy Expended is a UINT16, the highest value that can be 
         * represented is 65535 kilo Joules. If the maximum value of 65535 
         * kilo Joules is attained (0xFFFF), the field value should remain at 
         * 0xFFFF so that the client can be made aware that a reset of the 
         * Energy Expended Field is require.
         */
        g_hr_serv_data.energy_expended = MAX_ENERGY_EXPENDED_IN_KJOULES;
    }
    else
    {
        g_hr_serv_data.energy_expended += energy_exp;
    }

}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HeartRateGetEnergyExpended
 *
 *  DESCRIPTION
 *      This function returns energy expended value
 *
 *  RETURNS
 *      Energy expended value.
 *
 *---------------------------------------------------------------------------*/

extern uint16 HeartRateGetEnergyExpended(void)
{
    return g_hr_serv_data.energy_expended;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HeartRateReadDataFromNVM
 *
 *  DESCRIPTION
 *      This function is used to read heart rate service specific data store in 
 *      NVM
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HeartRateReadDataFromNVM(bool nvm_fresh_start, uint16 *p_offset)
{

     g_hr_serv_data.nvm_offset = *p_offset;

    /* Read NVM only if devices are bonded */
    if(AppIsDeviceBonded())
    {

        /* Read Heart Rate Measurement Client Configuration */
        Nvm_Read((uint16*)&g_hr_serv_data.hr_meas_client_config,
                 sizeof(g_hr_serv_data.hr_meas_client_config),
                 g_hr_serv_data.nvm_offset +
                 HR_NVM_HR_MEAS_CLIENT_CONFIG_OFFSET);

    }

    if(nvm_fresh_start)
    {
        /* As NVM is being written for the first time, update NVM with the 
         * energy expended value [initialised in HRInitChipReset() function]
         */

        Nvm_Write(&g_hr_serv_data.energy_expended, 
                 sizeof(g_hr_serv_data.energy_expended),
                 g_hr_serv_data.nvm_offset +
                 HR_NVM_ENERGY_EXPENDED_OFFSET);
    }
    else
    {
        /* Read Energy Expended charcteristic value */
        Nvm_Read(&g_hr_serv_data.energy_expended,
                 sizeof(g_hr_serv_data.energy_expended),
                 g_hr_serv_data.nvm_offset +
                 HR_NVM_ENERGY_EXPENDED_OFFSET);
    }

    /* Increment the offset by the number of words of NVM memory required 
     * by heart rate service 
     */
    *p_offset += HEART_RATE_SERVICE_NVM_MEMORY_WORDS;

}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HeartRateSendMeasValue
 *
 *  DESCRIPTION
 *      This functions sends the heart rate measurement value to the connected 
 *      client.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HeartRateSendMeasValue(uint16 ucid, uint8 hrm_length, 
                                   uint8 *p_hr_meas)
{
    GattCharValueNotification(ucid, 
                              HANDLE_HEART_RATE_MEASUREMENT, 
                              (uint16)hrm_length, 
                              p_hr_meas);  /* heart rate */
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HeartRateHandleAccessRead
 *
 *  DESCRIPTION
 *      This function handles read operation on heart rate service attributes
 *      maintained by the application and responds with the GATT_ACCESS_RSP 
 *      message.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HeartRateHandleAccessRead(GATT_ACCESS_IND_T *p_ind)
{
    /* Initialise to 2 octets for Client Configuration */
    uint16 length = 2;
    uint8  value[2];
    uint8  *p_val = NULL;
    sys_status rc = sys_status_success;
    // bool xx = FALSE;
    
    switch(p_ind->handle)
    {

        case HANDLE_HEART_RATE_MEASUREMENT_C_CFG:
        {
            p_val = value;
            BufWriteUint16(&p_val, 
                g_hr_serv_data.hr_meas_client_config);
        }
        break;

        case HANDLE_SWITCH_INTENSITY:
        {
          length = sizeof(g_hr_serv_data.switch_intensity);
          // p_val = 1;
          // PioSet(10, TRUE);
          // FlipSwitch();
            Nvm_Read((uint16*)&g_hr_serv_data.switch_intensity, 
             length, 
             g_hr_serv_data.nvm_offset + HR_NVM_SWITCH_OFFSET);
          // p_val = (uint8*)&g_hr_serv_data.switch_intensity;
          // xx = !xx;
          // p_val = (uint8*)&xx;
          // if(switch_intensity == TRUE) {
          //   p_val = 0;
          // } else {
          //   p_val = 0;
          // }
          // p_val = &switch_intensity;
        }
        break;

        default:

         rc = gatt_status_read_not_permitted;
        break;

    }

    if(p_ind->handle == HANDLE_SWITCH_INTENSITY) {
      GattAccessRsp(p_ind->cid, p_ind->handle, rc,
                          length, &g_hr_serv_data.switch_intensity);
    } else {
      GattAccessRsp(p_ind->cid, p_ind->handle, rc,
                          length, value);
    }
    

}

extern void AdjustSwitchIntensity(uint8 new_switch_intensity)
{
    // p_val = &switch_intensity;
    g_hr_serv_data.switch_intensity = new_switch_intensity;
    WriteSwitchIntensity();

    // switch_intensity = new_switch_intensity;//GetSwitchIntensity();
    // PioSet(PIO_LIGHT, new_switch_intensity);
}

extern void WriteSwitchIntensity(void){
  Nvm_Write((uint16*)&g_hr_serv_data.switch_intensity,
                          sizeof(g_hr_serv_data.switch_intensity),
                          g_hr_serv_data.nvm_offset + 
                          HR_NVM_SWITCH_OFFSET);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HeartRateHandleAccessWrite
 *
 *  DESCRIPTION
 *      This function handles write operation on heart rate service attributes 
 *      maintained by the application.and responds with the GATT_ACCESS_RSP 
 *      message.
 *
 *  RETURNS
 *      Nothing
 *
 *---------------------------------------------------------------------------*/

extern void HeartRateHandleAccessWrite(GATT_ACCESS_IND_T *p_ind)
{
    uint8 *p_value = p_ind->value;
    gatt_client_config client_config;
    sys_status rc = sys_status_success;
    uint8  *p_val = NULL;
    uint16 length = 1;
    // FlipSwitch();
    switch(p_ind->handle)
    {
        
        case HANDLE_SWITCH_INTENSITY:
        {
          // rc = gatt_status_write_not_permitted;
          // switch_to_state = BufReadUint16(&p_value);
          // p_val = (uint8)FlipSwitch();           
          uint8 new_switch_intensity = BufReadUint16(&p_value);
          AdjustSwitchIntensity(new_switch_intensity);
          // p_val = (uint8*)&g_hr_serv_data.switch_intensity;
          break;
        }
        case HANDLE_HEART_RATE_MEASUREMENT_C_CFG:
        {
            client_config = BufReadUint16(&p_value);

            if((client_config == gatt_client_config_notification) ||
               (client_config == gatt_client_config_none))
            {
                    g_hr_serv_data.hr_meas_client_config = client_config;

                /* Write Heart Rate Measurement Client configuration to NVM if 
                 * the device is bonded.
                 */
                if(AppIsDeviceBonded())
                {
                     Nvm_Write((uint16*)&client_config,
                              sizeof(client_config),
                              g_hr_serv_data.nvm_offset + 
                              HR_NVM_HR_MEAS_CLIENT_CONFIG_OFFSET);
                }

                /* Start sending the HR measurement notifications if they are 
                 * not being sent currently. 
                 */
                StartSendingHRMeasurements();
            }
            else
            {
                /* INDICATION or RESERVED */

                /* Return Error as only Notifications are supported */
                rc = gatt_status_app_mask;
            }

            break;
        }

        case HANDLE_HEART_RATE_CONTROL_POINT:
        {
            uint8 cntl_point_val = BufReadUint8(&p_value);

            if(cntl_point_val == hr_control_point_reset_energy)
            {
                g_hr_serv_data.energy_expended = 0;
            }
            else /* Reserved Value */
            {
                rc  = gatt_status_app_mask;
            }

            break;
        }
        default:
        {
            rc = gatt_status_write_not_permitted;
            break; 
        }
    }

    

    

    if(p_ind->handle == HANDLE_SWITCH_INTENSITY) {
      GattAccessRsp(p_ind->cid, p_ind->handle, rc,
                          length, p_val);
    } else {
      /* Send ACCESS RESPONSE */
      GattAccessRsp(p_ind->cid, p_ind->handle, rc, 0, NULL);
    }

}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HeartRateCheckHandleRange
 *
 *  DESCRIPTION
 *      This function is used to check if the handle belongs to the heart rate 
 *      service
 *
 *  RETURNS
 *      Boolean - Indicating whether handle falls in range or not.
 *
 *---------------------------------------------------------------------------*/

extern bool HeartRateCheckHandleRange(uint16 handle)
{
    return ((handle >= HANDLE_HEART_RATE_SERVICE) &&
            (handle <= HANDLE_HEART_RATE_SERVICE_END))
            ? TRUE : FALSE;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HeartRateBondingNotify
 *
 *  DESCRIPTION
 *      This function is used by application to notify bonding status to 
 *      heart rate service
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void HeartRateBondingNotify(void)
{

    /* Write data to NVM if bond is established */
    if(AppIsDeviceBonded())
    {
        /* Write to NVM the client configuration value of HR measurement 
         * state if it was configured prior to bonding 
         */
        Nvm_Write((uint16*)&g_hr_serv_data.hr_meas_client_config, 
                  sizeof(g_hr_serv_data.hr_meas_client_config),
                  g_hr_serv_data.nvm_offset + 
                  HR_NVM_HR_MEAS_CLIENT_CONFIG_OFFSET);
    }

}

