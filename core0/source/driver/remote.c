/**
  ******************************************************************************
  * @file    remote.c
  * @editor  jace
  * @version V1.0
  * @date    2019.10.5
  * @brief   
  ******************************************************************************

  ******************************************************************************
  */
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_ctimer.h"
#include "osal.h"
#include "board.h"
#include "pin_mux.h"
#include "remote.h"

/*
 * �ز��źſ��ƶ�ʱ��
 */
#define CTIMER CTIMER2                  /* Timer 2 */
#define CTIMER_MAT0_OUT kCTIMER_Match_1 /* Match output 1 */
#define CTIMER_CLK_FREQ CLOCK_GetCTimerClkFreq(2U)

/*
 * �����������
 */
#define IR_OUT_PIN (10)
#define IR_OUT_PORT (1)

#define ir_toggle() GPIO_PortToggle(GPIO,IR_OUT_PORT,0x1<<IR_OUT_PIN)
#define ir_off() GPIO_PinWrite(GPIO, IR_OUT_PORT, IR_OUT_PIN, 0)

static bool has_init=false;

static inline void ir_io_init()
{
    if(has_init)
        return;
    has_init=true;

    //IO ����
    const uint32_t ir_out = (IOCON_PIO_FUNC0 |IOCON_PIO_MODE_PULLUP |IOCON_PIO_SLEW_STANDARD |
                                IOCON_PIO_INV_DI |IOCON_PIO_DIGITAL_EN |IOCON_PIO_OPENDRAIN_DI);
    IOCON_PinMuxSet(IOCON, IR_OUT_PORT, IR_OUT_PIN, ir_out);

    //IO���
    gpio_pin_config_t pin_config = {
        kGPIO_DigitalOutput,
        1,
    };
//    GPIO_PortInit(GPIO, IR_OUT_PORT);//IO �ڲ�Ҫ��ʼ�����Σ�����ȫ��IO����λ

    GPIO_PinInit(GPIO, IR_OUT_PORT, IR_OUT_PIN, &pin_config);
    GPIO_PinWrite(GPIO, IR_OUT_PORT, IR_OUT_PIN, 0);
}

//******************************************************************************

static uint32_t timer_cnt=0,
					data_cnt=0;


/*
	һ�������ڷ��͵����ݣ���һ���ǵȴ����͵����ݣ��ȴ����͵����ݿ��Ա�����
*/
static uint32_t sending_data=0,
				wait_send_key_data=0,
				wait_send_rgb_data=0;

static bool is_sending=false,
			is_waiting_send_key=false,
			is_waiting_send_rgb=false;
			
static uint32_t bit_cnt=0,				//����λ������һ������32λ
				data_h=0,data_l=0;		//��ǰ����λ�ĸߵ�ƽ�͵͵�ƽʱ�䣨�ز�����ʱ��͵͵�ƽʱ�䣩
static bool load_next_bit=true;			//��һλ				
	
#define __clear() \
	bit_cnt=0; \
	load_next_bit=true; \
	data_cnt=0; \
	timer_cnt=0 

static void tim_call_back(uint32_t flags)
{
	//1������26us
	#define T (11)
	
	#define SRART_H (9000/T)
	#define SRART_L (4500/T)
	
	#define DATA0_H (560/T)
	#define DATA0_L (560/T)
	
	#define DATA1_H (560/T)
	#define DATA1_L (1120/T)
	
	#define WAIT_H (30000/T) //�����һ������ 30ms
	
	#define SEND_BIT_NUM (32)
	
	timer_cnt++;
	
	if(timer_cnt<SRART_H)
	{
		ir_toggle();
	}
	else if(timer_cnt<SRART_H+SRART_L)
	{
		ir_off();
	}
	else if(timer_cnt<SRART_H+SRART_L+(DATA1_H+DATA1_L)*SEND_BIT_NUM) 
	{
//		if((data_cnt%(DATA1_H+DATA1_L)) <DATA1_H) //������
//			ir_toggle();
//		else
//			ir_off();
//		data_cnt++;

#if 1
		do{
			if(bit_cnt>=SEND_BIT_NUM)
			{
				timer_cnt=SRART_H+SRART_L+(DATA1_H+DATA1_L)*SEND_BIT_NUM;
				
				break;
			}
			
			if(load_next_bit)
			{
				load_next_bit=false;
				if(sending_data&(1<<bit_cnt))
				{
					data_h=DATA1_H;
					data_l=DATA1_L;
				}
				else
				{
					data_h=DATA0_H;
					data_l=DATA0_L;
				}
			}
			
			if(data_cnt< data_h)
				ir_toggle();
			else
				ir_off();
			data_cnt++;
			
			if(data_cnt>=data_h+data_l)//������һλ
			{
				data_cnt=0;
				bit_cnt++;
				load_next_bit=true;
			}
		}while(0);
#endif
	}
	else if(timer_cnt<SRART_H+SRART_L+(DATA1_H+DATA1_L)*SEND_BIT_NUM + WAIT_H)
	{
		//��Ҫ�෢һ��λ���������һ���ֽ��޷�����
		if(data_cnt < DATA0_H) 
		{
			data_cnt++;
			ir_toggle();
		}
		else
		{
			ir_off();
		}
	}
	else 
	{
		__clear();
		
		if(is_waiting_send_key)
		{
			is_waiting_send_key=false;
			
			//������һ������
			is_sending=true;
			sending_data=wait_send_key_data;
		}
		else if(is_waiting_send_rgb)
		{
			is_waiting_send_rgb=false;
			
			//������һ������
			is_sending=true;
			sending_data=wait_send_rgb_data;
		}
		else
		{
			is_sending=false;
			
			//�չض�ʱ��
			CTIMER_StopTimer(CTIMER);
		}
	}
}

void tim_set_timing(uint32_t fre)
{
    if(!has_init)
    {
        ir_io_init();

        ctimer_config_t config;
        ctimer_match_config_t matchConfig0;

        CLOCK_AttachClk(kFRO_HF_to_CTIMER2);

        CTIMER_GetDefaultConfig(&config);
        CTIMER_Init(CTIMER, &config);

        /* Configuration 0 */
        matchConfig0.enableCounterReset = true;
        matchConfig0.enableCounterStop  = false;
        matchConfig0.matchValue         = CTIMER_CLK_FREQ / (fre);
        matchConfig0.outControl         = kCTIMER_Output_NoAction;
        matchConfig0.outPinInitState    = false;
        matchConfig0.enableInterrupt    = true;

        static ctimer_callback_t ctimer_callback_table[] = {
                tim_call_back
        };

        CTIMER_RegisterCallBack(CTIMER, &ctimer_callback_table[0], kCTIMER_SingleCallback);
        CTIMER_SetupMatch(CTIMER, CTIMER_MAT0_OUT, &matchConfig0);
    }

    CTIMER_StartTimer(CTIMER);
}

void tim_stop()
{
    CTIMER_StopTimer(CTIMER);
}


//���ⷢ������
void start_ir_send(bool is_rgb,uint32_t sdat)
{
	bool need_start=false;
	
OS_ENTER_CRITICAL_SECTION();
	if(!is_sending)
	{
		need_start=true;
		is_sending=true;
		sending_data=sdat;
	}
	else if(is_rgb)
	{
		wait_send_rgb_data=sdat;
		is_waiting_send_rgb=true;
	}
	else
	{
		wait_send_key_data=sdat;
		is_waiting_send_key=true;
	}
OS_LEAVE_CRITICAL_SECTION();
	
	if(!need_start)
		return;
	
	__clear();
	
	//38K �ز��ź�
    tim_set_timing(38000*2);
	
}
