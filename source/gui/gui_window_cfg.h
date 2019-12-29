/*
 * gui_window_cfg.h
 *
 *  Created on: 2019��12��7��
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_WINDOW_CFG_H_
#define WEAR_SRC_GUI_GUI_WINDOW_CFG_H_



/*
 * �ú����ڲ�������ӡ  __gui_window_fb_fill() �ӽ��뵽�˳�����ʱ��
 *
 * ���������ݴ�flash��ȡ������һ�����ѵ�ʱ��
 *
 */
#define MEASURE_FILL_TIME (0)

/*
 * �ú����ڲ�������ӡ gui_window_render() �ӽ��뵽�˳�����ʱ��
 */
#define MEASURE_RENDER_TIME      (0)

/*
 * �ú����ڲ�������ӡ  gui_window_render_pre() �ӽ��뵽�˳�����ʱ��
 */
#define MEASURE_RENDER_PRE_TIME      (0)

/*
 * �ú����ڲ�������ӡ  gui_window_render_mix() �ӽ��뵽�˳�����ʱ��
 */
#define MEASURE_RENDER_MIX_TIME (0)





/*
 * �ú���Ƶ��� gui_window_render() ��buf����䷽ʽ��
 *  0 ʹ��˫buf������ʾ���Ѵ������ݼ��ص�buf1��Ȼ��buf1�ٸ��Ƶ�buf2��buf2��ʾ������
 *  1 ʹ�õ�buf��ʾ���Ѵ������ݼ��ص�buf1��buf1ֱ����ʾ������
 */
#define USE_SINGLE_BUF_RENDER (1)

/*
 * �ú���Ƶ��� gui_window_render_pre() ��buf����䷽ʽ��
 *  0 ʹ��˫buf������ʾ���Ѵ������ݼ��ص�buf1��buf2Ϊ��һ��ˢ�µ����ݽ�����/��/��/���ƶ���
 *      �ٰ�buf1�ٸ��Ƶ�buf2�ƶ������Ŀռ䣬buf2��ʾ������
 *  1 ʹ�õ�buf��ʾ��buf1Ϊ��һ��ˢ�µ����ݽ�����/��/��/���ƶ��󣬰Ѵ������ݼ��ص�buf1�ƶ������Ŀռ䣬
 *      buf1ֱ����ʾ������
 */
#define USE_SINGLE_BUF_RENDER_PRE (1)


/*
 * �ú���Ƶ��� gui_window_render_mix() ��buf����䷽ʽ��
 *  0 ʹ��˫buf������ʾ���Ѵ������ݼ��ص�buf1��ͬһ����ʱ��ֻ�ڵ�һ�ν��봰��ʱ���أ�ֱ������ı�����¼��أ���
 *      buf2Ϊ��һ��ˢ�µ����ݽ�����/��/��/���ƶ����ٰ�buf1�ж�Ӧ��������ݸ��Ƶ�buf2�ƶ������Ŀռ䣬
 *      buf2��ʾ������
 *
 *      ���ַ�ʽ�ĺ������ڣ�����ͬһ���򻬶�ʱ����ʾ������ֻ��RAM���أ�����flash�ж�ȡ���ٶȷǳ��Ŀ죡
 *
 *  1 ʹ�õ�buf��ʾ��buf1Ϊ��һ��ˢ�µ����ݽ�����/��/��/���ƶ��󣬰Ѵ������ݼ��ص�buf1�ƶ������Ŀռ䣬
 *      buf1ֱ����ʾ������
 *
 *      ���ַ�ʽ������ gui_window_render_pre()�������ı��ֻ��������������ȡ���ݡ�
 *
 */
#define USE_SINGLE_BUF_RENDER_MIX (1)




/*
 * __gui_window_fb_fill() ʹ��DMA�������ݣ��ܹ������ٶ�
 *
 */
#define USE_DMA_SPEED_UP_FILL (0)

/*
 * gui_window_render() ʹ��DMA�������ݣ��ܹ������ٶ�
 *
 */
#define USE_DMA_SPEED_UP_RENDER (0)

/*
 * gui_window_render_pre() ʹ��DMA�������ݣ��ܹ������ٶ�
 *
 */
#define USE_DMA_SPEED_UP_RENDER_PRE (0)


/*
 * gui_window_render_mix() ʹ��DMA�������ݣ��ܹ������ٶ�
 *
 */
#define USE_DMA_SPEED_UP_RENDER_MIX (0)





#endif /* WEAR_SRC_GUI_GUI_WINDOW_CFG_H_ */
