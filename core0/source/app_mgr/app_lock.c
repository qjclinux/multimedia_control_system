/*
 * app_lock.c
 *
 *  Created on: 2019Äê7ÔÂ12ÈÕ
 *      Author: jace
 */

#include <string.h>
#include <stdbool.h>
#include "osal.h"
#include "app_mgr.h"
#include "app_lock.h"

//#if 0
#ifndef OS_BAREMETAL

static OS_MUTEX m_app_lock=NULL;

    void app_lock()
    {
        if(m_app_lock==NULL)
        {
            OS_MUTEX_CREATE(m_app_lock);
        }
        OS_MUTEX_GET(m_app_lock, OS_MUTEX_FOREVER);
    }

    void app_unlock()
    {
        OS_MUTEX_PUT(m_app_lock);
    }

#endif



