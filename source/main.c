/**
 ****************************************************************************************
 *
 * @file main.c
 *
 *  灯光积木 控制系统。
 *
 * JUST FOR :
 *      NXP MCU挑战赛
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

static void system_init_task(void *pvParameters)
{
    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* attach 50 MHz clock to HSLSPI */
    CLOCK_AttachClk(kMAIN_CLK_to_HSLSPI);

    /* reset FLEXCOMM for SPI */
    RESET_PeripheralReset(kHSLSPI_RST_SHIFT_RSTn);

    BOARD_InitPins();
    BOARD_InitDebugConsole();

    hs_spi_init();

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
