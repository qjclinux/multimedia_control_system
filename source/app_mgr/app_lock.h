/*
 * app_lock.h
 *
 *  Created on: 2019Äê7ÔÂ12ÈÕ
 *      Author: jace
 */

#ifndef WEAR_SRC_APP_LOCK_H_
#define WEAR_SRC_APP_LOCK_H_

//#if 0
#ifndef OS_BAREMETAL

void app_lock();
void app_unlock();

#else

#define app_lock()
#define app_unlock()

#endif

#endif /* WEAR_SRC_APP_LOCK_H_ */
