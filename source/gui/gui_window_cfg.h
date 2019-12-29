/*
 * gui_window_cfg.h
 *
 *  Created on: 2019年12月7日
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_WINDOW_CFG_H_
#define WEAR_SRC_GUI_GUI_WINDOW_CFG_H_



/*
 * 该宏用于测量并打印  __gui_window_fb_fill() 从进入到退出的总时间
 *
 * 即测量数据从flash读取、处理一共花费的时间
 *
 */
#define MEASURE_FILL_TIME (0)

/*
 * 该宏用于测量并打印 gui_window_render() 从进入到退出的总时间
 */
#define MEASURE_RENDER_TIME      (0)

/*
 * 该宏用于测量并打印  gui_window_render_pre() 从进入到退出的总时间
 */
#define MEASURE_RENDER_PRE_TIME      (0)

/*
 * 该宏用于测量并打印  gui_window_render_mix() 从进入到退出的总时间
 */
#define MEASURE_RENDER_MIX_TIME (0)





/*
 * 该宏控制的是 gui_window_render() 中buf的填充方式：
 *  0 使用双buf加速显示，把窗口数据加载到buf1，然后buf1再复制到buf2，buf2显示到屏上
 *  1 使用单buf显示，把窗口数据加载到buf1，buf1直接显示到屏上
 */
#define USE_SINGLE_BUF_RENDER (1)

/*
 * 该宏控制的是 gui_window_render_pre() 中buf的填充方式：
 *  0 使用双buf加速显示，把窗口数据加载到buf1，buf2为上一次刷新的数据将其左/右/上/下移动后，
 *      再把buf1再复制到buf2移动出来的空间，buf2显示到屏上
 *  1 使用单buf显示，buf1为上一次刷新的数据将其左/右/上/下移动后，把窗口数据加载到buf1移动出来的空间，
 *      buf1直接显示到屏上
 */
#define USE_SINGLE_BUF_RENDER_PRE (1)


/*
 * 该宏控制的是 gui_window_render_mix() 中buf的填充方式：
 *  0 使用双buf加速显示，把窗口数据加载到buf1（同一方向时，只在第一次进入窗口时加载，直到方向改变才重新加载），
 *      buf2为上一次刷新的数据将其左/右/上/下移动后，再把buf1中对应区域的内容复制到buf2移动出来的空间，
 *      buf2显示到屏上
 *
 *      这种方式的好像在于：在往同一方向滑动时，显示的数据只从RAM加载，不从flash中读取，速度非常的快！
 *
 *  1 使用单buf显示，buf1为上一次刷新的数据将其左/右/上/下移动后，把窗口数据加载到buf1移动出来的空间，
 *      buf1直接显示到屏上
 *
 *      这种方式类似于 gui_window_render_pre()方法，改变的只是在两个窗口中取数据。
 *
 */
#define USE_SINGLE_BUF_RENDER_MIX (1)




/*
 * __gui_window_fb_fill() 使用DMA复制数据，能够提升速度
 *
 */
#define USE_DMA_SPEED_UP_FILL (0)

/*
 * gui_window_render() 使用DMA复制数据，能够提升速度
 *
 */
#define USE_DMA_SPEED_UP_RENDER (0)

/*
 * gui_window_render_pre() 使用DMA复制数据，能够提升速度
 *
 */
#define USE_DMA_SPEED_UP_RENDER_PRE (0)


/*
 * gui_window_render_mix() 使用DMA复制数据，能够提升速度
 *
 */
#define USE_DMA_SPEED_UP_RENDER_MIX (0)





#endif /* WEAR_SRC_GUI_GUI_WINDOW_CFG_H_ */
