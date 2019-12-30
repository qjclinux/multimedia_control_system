/*
 * sw_delay.h
 *
 *  Created on: 2019年12月22日
 *      Author: jace
 */

#ifndef UTIL_SW_DELAY_H_
#define UTIL_SW_DELAY_H_

/*
 * 这是 150M 主频，用逻辑分析仪测量出来的延时
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
