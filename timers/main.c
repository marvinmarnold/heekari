/******************************************************************************
 *  Copyright (C) Cambridge Silicon Radio Limited 2012-2013
 *  Part of CSR uEnergy SDK 2.2.2
 *  Application version 2.2.2.0
 *
 *  FILE
 *      main.c
 *
 *  DESCRIPTION
 *      Simple timers example to show Timers usage. This application also
 *      demonstrates how to produce multiple timeouts using a single timer
 *      resource by chaining timer requests.
 *
 ******************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/
 
#include <main.h>           /* Functions relating to powering up the device */
#include <ls_app_if.h>      /* Link Supervisor application interface */
#include <debug.h>          /* Simple host interface to the UART driver */
#include <timer.h>          /* Chip timer functions */
#include <panic.h>          /* Support for applications to panic */
#include <pio.h>
/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Number of timers used in this application */
#define MAX_TIMERS 1

/* First timeout at which the timer has to fire a callback */
#define TIMER_TIMEOUT1 (1 * MILLISECOND)

/* Subsequent timeout at which the timer has to fire next callback */
#define TIMER_TIMEOUT2 (3)

#define PIO_LIGHT 10

/*============================================================================*
 *  Private Data
 *============================================================================*/

/* Declare timer buffer to be managed by firmware library */
static uint16 app_timers[SIZEOF_APP_TIMER * MAX_TIMERS];

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/

/* Start timer */

static void startTimer(uint32 timeout, timer_callback_arg handler);

/* Callback after first timeout */
static void timerCallback1(timer_id const id);

/* Callback after second timeout */
static void timerCallback2(timer_id const id);

/* Read the current system time and write to UART */
static void printCurrentTime(void);

/* Convert an integer value into an ASCII string and send to the UART */
static uint8 writeASCIICodedNumber(uint32 value);

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      startTimer
 *
 *  DESCRIPTION
 *      Start a timer
 *
 * PARAMETERS
 *      timeout [in]    Timeout period in seconds
 *      handler [in]    Callback handler for when timer expires
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void startTimer(uint32 timeout, timer_callback_arg handler)
{
    /* Now starting a timer */
    
    const timer_id tId = TimerCreate(timeout, TRUE, handler);
    
    /* If a timer could not be created, panic to restart the app */
    if (tId == TIMER_INVALID)
    {
        DebugWriteString("\r\nFailed to start timer");
        
        /* Panic with panic code 0xfe */
        Panic(0xfe);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      timerCallback1
 *
 *  DESCRIPTION
 *      This function is called when the timer created by TimerCreate expires.
 *      It creates a new timer that will expire after the second timer interval.
 *
 * PARAMETERS
 *      id [in]     ID of timer that has expired
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void timerCallback1(timer_id const id)
{
     PioSet(PIO_LIGHT, FALSE);   
    const uint32 elapsed = TIMER_TIMEOUT1 / SECOND;
    
    if (elapsed == 1)
        DebugWriteString("1 second elapsed\r\n");
    else          
    {
        writeASCIICodedNumber(elapsed);
        DebugWriteString(" seconds elapsed\r\n");
    }

    /* Now start a new timer for second callback */
    
    startTimer(TIMER_TIMEOUT1, timerCallback2);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      timerCallback2
 *
 *  DESCRIPTION
 *      This function is called when the timer created by TimerCreate expires.
 *      It creates a new timer that will expire after the first timer interval.
 *
 * PARAMETERS
 *      id [in]     ID of timer that has expired
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void timerCallback2(timer_id const id)
{
    const uint32 elapsed = TIMER_TIMEOUT2 / SECOND;
    PioSet(PIO_LIGHT, FALSE);
    if (elapsed == 1)
        DebugWriteString("1 second elapsed\r\n");
    else          
    {
        writeASCIICodedNumber(elapsed);
        DebugWriteString(" seconds elapsed\r\n");
    }

    /* Report current system time */
    printCurrentTime();
    
    /* Now start a new timer for first callback */
    
    startTimer(TIMER_TIMEOUT1, timerCallback1);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      printCurrentTime
 *
 *  DESCRIPTION
 *      Read the current system time and write to UART.
 *
 * PARAMETERS
 *      None
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void printCurrentTime(void)
{
    /* Read current system time */
    const uint32 now = TimeGet32();
    
    /* Report current system time */
    DebugWriteString("\n\nCurrent system time: ");
    writeASCIICodedNumber(now / MINUTE);
    DebugWriteString("m ");
    writeASCIICodedNumber((now % MINUTE)/SECOND);
    DebugWriteString("s\r\n");
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      writeASCIICodedNumber
 *
 *  DESCRIPTION
 *      Convert an integer value into an ASCII encoded string and send to the
 *      UART
 *
 * PARAMETERS
 *      value [in]     Integer to convert to ASCII and send over UART
 *
 * RETURNS
 *      Number of characters sent to the UART.
 *----------------------------------------------------------------------------*/
static uint8 writeASCIICodedNumber(uint32 value)
{
#define BUFFER_SIZE 11          /* Buffer size required to hold maximum value */
    
    uint8  i = BUFFER_SIZE;     /* Loop counter */
    uint32 remainder = value;   /* Remaining value to send */
    char   buffer[BUFFER_SIZE]; /* Buffer for ASCII string */

    /* Ensure the string is correctly terminated */    
    buffer[--i] = '\0';
    
    /* Loop at least once and until the whole value has been converted */
    do
    {
        /* Convert the unit value into ASCII and store in the buffer */
        buffer[--i] = (remainder % 10) + '0';
        
        /* Shift the value right one decimal */
        remainder /= 10;
    } while (remainder > 0);

    /* Send the string to the UART */
    DebugWriteString(buffer + i);
    
    /* Return length of ASCII string sent to UART */
    return (BUFFER_SIZE - 1) - i;
}

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppPowerOnReset
 *
 *  DESCRIPTION
 *      This user application function is called just after a power-on reset
 *      (including after a firmware panic), or after a wakeup from Hibernate or
 *      Dormant sleep states.
 *
 *      At the time this function is called, the last sleep state is not yet
 *      known.
 *
 *      NOTE: this function should only contain code to be executed after a
 *      power-on reset or panic. Code that should also be executed after an
 *      HCI_RESET should instead be placed in the AppInit() function.
 *
 * PARAMETERS
 *      None
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
void AppPowerOnReset(void)
{
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppInit
 *
 *  DESCRIPTION
 *      This user application function is called after a power-on reset
 *      (including after a firmware panic), after a wakeup from Hibernate or
 *      Dormant sleep states, or after an HCI Reset has been requested.
 *
 *      NOTE: In the case of a power-on reset, this function is called
 *      after app_power_on_reset().
 *
 * PARAMETERS
 *      last_sleep_state [in]   Last sleep state
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
void AppInit(sleep_state last_sleep_state)
{
    /* Initialise communications */
    /* Set LIGHT to be controlled directly via PioSet */
    PioSetModes((1UL << PIO_LIGHT), pio_mode_user);

    /* Configure LED0 and LED1 to be outputs */
    PioSetDir(PIO_LIGHT, TRUE);

    /* Set the LED0 and LED1 to have strong internal pull ups */
    PioSetPullModes((1UL << PIO_LIGHT),
                    pio_mode_strong_pull_up);

    /* Turn off both LEDs by setting output to Low */
    PioSets((1UL << PIO_LIGHT), 0UL);
    PioSet(PIO_LIGHT, FALSE); ///new
    DebugInit(1, NULL, NULL);

    DebugWriteString("\r\nInitialising timers");
    TimerInit(MAX_TIMERS, (void *)app_timers);

    /* Report current time */
    printCurrentTime();

    /* Start the first timer */
    PioSet(PIO_LIGHT, TRUE); 
    startTimer(TIMER_TIMEOUT1, timerCallback1);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcesSystemEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a system event, such
 *      as a battery low notification, is received by the system.
 *
 * PARAMETERS
 *      id   [in]   System event ID
 *      data [in]   Event data
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
void AppProcessSystemEvent(sys_event_id id, void *data)
{
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessLmEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a LM-specific event
 *      is received by the system.
 *
 * PARAMETERS
 *      event_code [in]   LM event ID
 *      event_data [in]   LM event data
 *
 * RETURNS
 *      TRUE if the app has finished with the event data; the control layer
 *      will free the buffer.
 *----------------------------------------------------------------------------*/
bool AppProcessLmEvent(lm_event_code event_code, LM_EVENT_T *event_data)
{
    return TRUE;
}
