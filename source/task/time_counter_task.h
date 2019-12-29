/*
 * time_counter_task.h
 *
 *  Created on: 2019Äê12ÔÂ9ÈÕ
 *      Author: jace
 */

#ifndef WEAR_SRC_TASK_TIME_COUNTER_TASK_H_
#define WEAR_SRC_TASK_TIME_COUNTER_TASK_H_


void os_set_time(const uint8_t* data);

/*
 * SDK symbol
 */
#include "time.h"
time_t os_get_utc_second();
struct tm os_get_now_time();


#endif /* WEAR_SRC_TASK_TIME_COUNTER_TASK_H_ */
