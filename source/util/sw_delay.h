/*
 * sw_delay.h
 *
 *  Created on: 2019��12��22��
 *      Author: jace
 */

#ifndef UTIL_SW_DELAY_H_
#define UTIL_SW_DELAY_H_

/*
 * ���� 150M ��Ƶ�����߼������ǲ�����������ʱ
 */
static inline void delay_ms(uint32_t ms)
{
    for(int i=0;i<ms;i++)
    {
        for(int j=0;j<16666;j++)
        {

        }
    }
}

#endif /* UTIL_SW_DELAY_H_ */
