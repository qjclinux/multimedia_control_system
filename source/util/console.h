#ifndef __CONSOLE__
#define __CONSOLE__


#define USE_OS_DEBUG 1

#if USE_OS_DEBUG
    #include "fsl_debug_console.h"
    #define os_printk PRINTF

    #define OS_LOG(fmt,args...)     do{\
                            /*os_printk("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
                            os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
                            os_printk(fmt,##args);\
                    }while(0)
    #define OS_INFO(fmt,args...)    do{\
                            os_printk(fmt,##args);\
                    }while(0)

#else
    #define os_printk(fmt,args...)

    #define OS_LOG(fmt,args...)
    #define OS_INFO(fmt,args...)

#endif

#endif
