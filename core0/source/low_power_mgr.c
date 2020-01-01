/*
 * low_power_mgr.c
 *
 *  Created on: 2020Äê1ÔÂ1ÈÕ
 *      Author: jace
 */
#include <stdbool.h>
#include "osal.h"
#include "fsl_power.h"

#if configUSE_TICKLESS_IDLE>0
void prvSystemSleep( uint32_t xExpectedIdleTime )
{
    POWER_EnterSleep();
}
#endif
