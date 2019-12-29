/*
 * hs_spi.c
 *
 *  Created on: 2019年12月23日
 *      Author: jace
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

#include "console.h"

#if 1

    static OS_MUTEX m_lock=NULL;

    static inline void _lock()
    {
        if(m_lock==NULL)
        {
            OS_MUTEX_CREATE(m_lock);
        }
        OS_MUTEX_GET(m_lock, OS_MUTEX_FOREVER);
    }

    static inline void _unlock()
    {
        OS_MUTEX_PUT(m_lock);
    }
#else
    #define _lock()
    #define _unlock()

#endif


#define EXAMPLE_SPI_MASTER SPI8
#define EXAMPLE_SPI_MASTER_IRQ LSPI_HS_IRQn
#define EXAMPLE_SPI_MASTER_CLK_SRC kCLOCK_HsLspi
#define EXAMPLE_SPI_MASTER_CLK_FREQ CLOCK_GetHsLspiClkFreq()
#define EXAMPLE_SPI_SSEL 1
#define EXAMPLE_DMA DMA0
#define EXAMPLE_SPI_MASTER_RX_CHANNEL 2
#define EXAMPLE_SPI_MASTER_TX_CHANNEL 3
#define EXAMPLE_MASTER_SPI_SPOL kSPI_SpolActiveAllLow


static dma_handle_t masterTxHandle;
static dma_handle_t masterRxHandle;
static spi_dma_handle_t masterHandle;

/*
 * free rtos 通知
 */
static OS_TASK  dma_start_task_hdl = NULL;
#define DMA_COMPLETE (0x1<<24)
static void SPI_MasterUserCallback(SPI_Type *base, spi_dma_handle_t *handle, status_t status, void *userData)
{
    if (status == kStatus_Success)
    {
        if(dma_start_task_hdl)
        {
            OS_TASK_NOTIFY_FROM_ISR(dma_start_task_hdl,DMA_COMPLETE,OS_NOTIFY_SET_BITS);
        }
    }
}


static void EXAMPLE_MasterInit(void)
{
    /* SPI init */
    uint32_t srcClock_Hz = 0U;
    spi_master_config_t masterConfig;
    srcClock_Hz = EXAMPLE_SPI_MASTER_CLK_FREQ;

    SPI_MasterGetDefaultConfig(&masterConfig);
    masterConfig.sselNum      = (spi_ssel_t)EXAMPLE_SPI_SSEL;
    masterConfig.sselPol      = (spi_spol_t)EXAMPLE_MASTER_SPI_SPOL;
    masterConfig.baudRate_Bps = 50000000U;//100000U;//
    masterConfig.direction = kSPI_MsbFirst;

    SPI_MasterInit(EXAMPLE_SPI_MASTER, &masterConfig, srcClock_Hz);
}

static void EXAMPLE_MasterDMASetup(void)
{
    /* DMA init */
    DMA_Init(EXAMPLE_DMA);
    /* Configure the DMA channel,priority and handle. */
    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_SPI_MASTER_TX_CHANNEL);
    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_SPI_MASTER_RX_CHANNEL);
    DMA_SetChannelPriority(EXAMPLE_DMA, EXAMPLE_SPI_MASTER_TX_CHANNEL, kDMA_ChannelPriority3);
    DMA_SetChannelPriority(EXAMPLE_DMA, EXAMPLE_SPI_MASTER_RX_CHANNEL, kDMA_ChannelPriority2);
    DMA_CreateHandle(&masterTxHandle, EXAMPLE_DMA, EXAMPLE_SPI_MASTER_TX_CHANNEL);
    DMA_CreateHandle(&masterRxHandle, EXAMPLE_DMA, EXAMPLE_SPI_MASTER_RX_CHANNEL);
}

static void EXAMPLE_MasterStartDMATransfer(void)
{
    /* Set up handle for spi master */
    SPI_MasterTransferCreateHandleDMA(EXAMPLE_SPI_MASTER, &masterHandle, SPI_MasterUserCallback, NULL, &masterTxHandle,
            &masterRxHandle);

}

void hs_spi_init()
{
    /* Initialize SPI master with configuration. */
    EXAMPLE_MasterInit();

    /* Set up DMA for SPI master TX and RX channel. */
    EXAMPLE_MasterDMASetup();

    /* Prepare DMA and start SPI master transfer in DMA way. */
    EXAMPLE_MasterStartDMATransfer();
}

int spi_write_data(uint8_t *wdat,size_t size)
{
    spi_transfer_t masterXfer;

    /* Start master transfer */
    masterXfer.txData      = wdat;
    masterXfer.rxData      = 0;
    masterXfer.dataSize    = size;
    masterXfer.configFlags = kSPI_FrameAssert;

    _lock();
    dma_start_task_hdl=OS_GET_CURRENT_TASK();

    if (kStatus_Success != SPI_MasterTransferDMA(EXAMPLE_SPI_MASTER, &masterHandle, &masterXfer))
    {
        OS_LOG("There is an error when start SPI_MasterTransferDMA \r\n ");
        _unlock();
        return -1;
    }

    /*
     * 等待DMA完成通知，超时为1 s
     */
    uint32_t notif;
    OS_TASK_NOTIFY_WAIT(0x0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_MS_2_TICKS(1000));
    if ((notif&DMA_COMPLETE) == 0)
    {
        OS_LOG("dma send err! task=%08x\r\n",dma_start_task_hdl);
        _unlock();
        return -1;
    }

    _unlock();
    return 0;
}

int spi_read_data(uint8_t *rdat,size_t size)
{
    spi_transfer_t masterXfer;

    /* Start master transfer */
    masterXfer.txData      = 0;
    masterXfer.rxData      = rdat;
    masterXfer.dataSize    = size;
    masterXfer.configFlags = kSPI_FrameAssert;

    _lock();
    dma_start_task_hdl=OS_GET_CURRENT_TASK();

    if (kStatus_Success != SPI_MasterTransferDMA(EXAMPLE_SPI_MASTER, &masterHandle, &masterXfer))
    {
        OS_LOG("There is an error when start SPI_MasterTransferDMA \r\n ");
        _unlock();
        return -1;
    }

    /*
         * 等待DMA完成通知，超时为1 s
     */
    uint32_t notif;
    OS_TASK_NOTIFY_WAIT(0x0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_MS_2_TICKS(1000));
    if ((notif&DMA_COMPLETE) == 0)
    {
        OS_LOG("dma send err! task=%08x\r\n",dma_start_task_hdl);
        _unlock();
        return -1;
    }

    _unlock();
    return 0;
}



int spi_req_lock()
{
    _lock();
    return 0;
}

int spi_req_unlock()
{
    _unlock();
    return 0;
}





