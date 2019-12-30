/**
 ****************************************************************************************
 *
 * @file main.c
 *
 *   基于LPC55S69 双核MCU平台的多媒体控制系统，3.2寸触摸彩屏，系统搭载jace GUI、jace FS ，
 *   高效的Code使得系统操作效果非常流畅。
 *
 *   本系统功能完整，配合灯光积木使用，可控制灯光效果。
 *
 * JUST FOR :
 *      NXP MCU挑战赛
 *
 *
 *      作者：jace
 *      email:734136032@qq.com
 *
 *      本工程jace GUI、jace FS 为jace本人发明，暂时不开源，如有兴趣，可与本人取得联系。
 *      当然，此工程的lib、源码可在不经过本人同意的情况下随意使用。
 *
 *
 *  date:2019.12.31
 *
 *      祝大家新快乐！
 *
 *
 ****************************************************************************************
 */
#include <stdbool.h>
#include "osal.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_spi.h"
#include "fsl_spi_dma.h"
#include "fsl_dma.h"
#include "fsl_spi_freertos.h"
#include "board.h"
#include "pin_mux.h"
#include "hs_spi.h"
#include "fsl_powerquad.h"
#include "mcmgr.h"
#include "gui.h"
#include "console.h"
#include "project_cfg.h"

/*!
 * @brief Application-specific implementation of the SystemInitHook() weak function.
 */
void SystemInitHook(void)
{
    /* Initialize MCMGR - low level multicore management library. Call this
       function as close to the reset entry as possible to allow CoreUp event
       triggering. The SystemInitHook() weak function overloading is used in this
       application. */
    MCMGR_EarlyInit();
}
/*-------------------------------------------------------------------------------------------*/

/*
 * Core1 操作
 */
#define RPMSG_FLUSH_SCREEN_REQ  1
#define RPMSG_FLUSH_SCREEN_DONE 2

static OS_TASK  core1_req_task = NULL;
#define REQ_COMPLETE (0x1<<24)

static void RPMsgRemoteReadyEventHandler(uint16_t eventData, void *context)
{
    if(eventData==RPMSG_FLUSH_SCREEN_DONE)
    {
        if(core1_req_task)
        {
            OS_TASK_NOTIFY_FROM_ISR(core1_req_task,REQ_COMPLETE,OS_NOTIFY_SET_BITS);
        }
    }
}
//请求核1刷屏
void notif_cor1_flush_screen()
{
    uint32_t notif;
    core1_req_task=OS_GET_CURRENT_TASK();

    OS_ENTER_CRITICAL_SECTION();
    MCMGR_TriggerEventForce(kMCMGR_RemoteApplicationEvent, RPMSG_FLUSH_SCREEN_REQ);
    OS_LEAVE_CRITICAL_SECTION();

    //等待核1处理完成,超时100ms
    OS_TASK_NOTIFY_WAIT(0x0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_MS_2_TICKS(100));
    if ((notif&REQ_COMPLETE) == 0)
    {
        OS_LOG("core1 handle err!\r\n");
    }
    else
    {
        OS_LOG("core1 done!\r\n");
    }
}
/*-------------------------------------------------------------------------------------------*/

static void system_init_task(void *pvParameters)
{
    MCMGR_Init();

    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

#if !USE_CORE1_FLUSH_SCREEN
    /* attach 50 MHz clock to HSLSPI */
    CLOCK_AttachClk(kMAIN_CLK_to_HSLSPI);

    /* reset FLEXCOMM for SPI */
    RESET_PeripheralReset(kHSLSPI_RST_SHIFT_RSTn);
#endif

    BOARD_InitPins();
    BOARD_InitDebugConsole();

    PQ_Init(POWERQUAD);

#if !USE_CORE1_FLUSH_SCREEN
    hs_spi_init();
#endif

    /*
     * 启动Core1，把GUI的fb地址传输过去
     */
#define CORE1_BOOT_ADDRESS (void *)0x20033000
    MCMGR_StartCore(kMCMGR_Core1, CORE1_BOOT_ADDRESS, (uint32_t)gui_get_front_fb(), kMCMGR_Start_Synchronous);
    MCMGR_RegisterEvent(kMCMGR_RemoteApplicationEvent, RPMsgRemoteReadyEventHandler,0);

    void main_task_init(void);
    main_task_init();

    OS_TASK_DELETE(OS_GET_CURRENT_TASK());
}

/*!
 * @brief Main function
 */
int main(void)
{
    OS_BASE_TYPE status;

    BOARD_BootClockPLL150M();

    status=xTaskCreate(system_init_task, "SysInit",
            1024, NULL, OS_TASK_PRIORITY_HIGHEST, NULL);

    OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

    vTaskStartScheduler();

    for ( ;; );
}
