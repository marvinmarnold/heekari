/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2014
 *  CSR Bluetooth Low Energy CSRmesh 1.0 Release
 *  Application version 1.0
 *
 *  FILE
 *      iot_hw.c
 *
 *  DESCRIPTION
 *      This file implements the LED Controller of IOT hardware
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <pio.h>            /* Programmable I/O configuration and control */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "iot_hw.h"
#include "fast_pwm.h"

/*============================================================================*
 *  Private data
 *============================================================================*/
/* Colour depth in bits, as passed by application. */
#define LIGHT_INPUT_COLOR_DEPTH  (8)
/* Colour depth in bits, mapped to actual hardware. */
#define LIGHT_MAPPED_COLOR_DEPTH (6)
/* Colour depth lost due to re-quantization of levels. */
#define QUANTIZATION_ERROR       (LIGHT_INPUT_COLOR_DEPTH -\
                                  LIGHT_MAPPED_COLOR_DEPTH)
/* Maximum colour level supported by mapped colour depth bits. */
#define COLOR_MAX_VALUE          ((0x1 << LIGHT_MAPPED_COLOR_DEPTH) - 1)

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      IOTLightControlDeviceInit
 *
 *  DESCRIPTION
 *      This function initialises the Red, Green and Blue LED lines.
 *      Configure the IO lines connected to Switch as inputs.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void IOTLightControlDeviceInit(void)
{
    PioFastPwmConfig(PIO_BIT_MASK(LED_PIO_RED) | \
                     PIO_BIT_MASK(LED_PIO_GREEN) | \
                     PIO_BIT_MASK(LED_PIO_BLUE));
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      IOTLightControlDevicePower
 *
 *  DESCRIPTION
 *      This function sets power state of LED.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void IOTLightControlDevicePower(bool power_on)
{
    PioFastPwmEnable(power_on);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      IOTLightControlDeviceSetLevel
 *
 *  DESCRIPTION
 *      This function sets the brightness level.
 *      Convert Level to equal values of RGB.
 *      Note that linear translation has been assumed for now.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void IOTLightControlDeviceSetLevel(uint8 level)
{
    /* Maps level to equal values of Red, Green and Blue */
    IOTLightControlDeviceSetColor(level, level, level);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      IOTLightControlDeviceSetColor
 *
 *  DESCRIPTION
 *      This function sets the colour as passed in argument values.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void IOTLightControlDeviceSetColor(uint8 red, uint8 green, uint8 blue)
{
    PioFastPwmSetWidth(LED_PIO_RED, red, 0xFF - red, TRUE);
    PioFastPwmSetWidth(LED_PIO_GREEN, green, 0xFF - green, TRUE);
    PioFastPwmSetWidth(LED_PIO_BLUE, blue, 0xFF - blue, TRUE);
    PioFastPwmSetPeriods(1, 0);
    PioFastPwmEnable(TRUE);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      IOTLightControlDeviceBlink
 *
 *  DESCRIPTION
 *      This function sets colour and blink time for LEDs.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void IOTLightControlDeviceBlink(uint8 red, uint8 green, uint8 blue,
                                       uint8 on_time, uint8 off_time)
{
    PioFastPwmSetWidth(LED_PIO_RED, red, 0, TRUE);
    PioFastPwmSetWidth(LED_PIO_GREEN, green, 0, TRUE);
    PioFastPwmSetWidth(LED_PIO_BLUE, blue, 0, TRUE);
    PioFastPwmSetPeriods((on_time << 4), (off_time << 4));
    PioFastPwmEnable(TRUE);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      IOTSwitchInit
 *
 *  DESCRIPTION
 *      This function sets GPIO to switch mode.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void IOTSwitchInit(void)
{
    /* Set-up the PIOs for the switches */
    PioSetDir(SW2_PIO, PIO_DIRECTION_INPUT);
    PioSetMode(SW2_PIO, pio_mode_user);

    PioSetDir(SW3_PIO, PIO_DIRECTION_INPUT);
    PioSetMode(SW3_PIO, pio_mode_user);

    PioSetDir(SW4_PIO, PIO_DIRECTION_INPUT);
    PioSetMode(SW4_PIO, pio_mode_user);
    
    PioSetPullModes(BUTTONS_BIT_MASK , pio_mode_strong_pull_up);
    
    PioSetEventMask(BUTTONS_BIT_MASK , pio_event_mode_both);
}

