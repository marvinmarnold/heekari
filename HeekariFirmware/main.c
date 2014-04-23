#include <main.h>
#include <ls_app_if.h>

/****************************************************************************
NAME
    AppPowerOnReset

DESCRIPTION
    This user application function is called just after a power-on reset
    (including after a firmware panic), or after a wakeup from Hibernate or
    Dormant sleep states.

    At the time this function is called, the last sleep state is not yet
    known.

    NOTE: this function should only contain code to be executed after a
    power-on reset or panic. Code that should also be executed after an
    HCI_RESET should instead be placed in the AppInit() function.

RETURNS

*/
void AppPowerOnReset(void)
{
}

/****************************************************************************
NAME    
    AppInit
    
DESCRIPTION
    This user application function is called after a power-on reset
    (including after a firmware panic), after a wakeup from Hibernate or
    Dormant sleep states, or after an HCI Reset has been requested.

    The last sleep state is provided to the application in the parameter.

    NOTE: In the case of a power-on reset, this function is called
    after app_power_on_reset().
    
RETURNS
    
*/
void AppInit(sleep_state last_sleep_state)
{

}


/****************************************************************************
NAME
    AppProcesSystemEvent

DESCRIPTION
    This user application function is called whenever a system event, such
    as a battery low notification, is received by the system.

RETURNS

*/
void AppProcessSystemEvent(sys_event_id id, void *data)
{
}

/****************************************************************************
NAME
    AppProcessLmEvent

DESCRIPTION
    This user application function is called whenever a LM-specific event is
    received by the system.

RETURNS
    Application should always return TRUE. Refer API Documentation under the module
    named "Application" for more information.
*/
bool AppProcessLmEvent(lm_event_code event_code, LM_EVENT_T *event_data)
{
    return TRUE;
}
