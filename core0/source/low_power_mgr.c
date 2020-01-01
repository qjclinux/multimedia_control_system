/*
 * low_power_mgr.c
 *
 *  Created on: 2020��1��1��
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
