/*
 * gui_lock.h
 *
 *  Created on: 2019Äê7ÔÂ12ÈÕ
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_LOCK_H_
#define WEAR_SRC_GUI_GUI_LOCK_H_

//#if 0
#ifndef OS_BAREMETAL

void gui_lock();
void gui_unlock();

#else

#define gui_lock()
#define gui_unlock()

#endif

#endif /* WEAR_SRC_GUI_GUI_LOCK_H_ */
