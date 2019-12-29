/*
 * gui_lock.c
 *
 *  Created on: 2019Äê7ÔÂ12ÈÕ
 *      Author: jace
 */

#include <string.h>
#include <stdbool.h>
#include "osal.h"
#include "gui.h"
#include "console.h"
#include "gui_lock.h"

//#if 0
#ifndef OS_BAREMETAL

static OS_MUTEX m_gui_lock=NULL;

void gui_lock()
{
    if(m_gui_lock==NULL)
    {
        OS_MUTEX_CREATE(m_gui_lock);
    }
    OS_MUTEX_GET(m_gui_lock, OS_MUTEX_FOREVER);
}

void gui_unlock()
{
    OS_MUTEX_PUT(m_gui_lock);
}

#endif



