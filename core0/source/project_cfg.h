/*
 * project_cfg.h
 *
 *  Created on: 2019年12月30日
 *      Author: jace
 */

#ifndef PROJECT_CFG_H_
#define PROJECT_CFG_H_

/*
 * 此宏控制着是否使用核1刷屏，1使用，0不使用
 *
 * 修改些宏时，同时要修改Core1工程，把刷屏部分代码屏蔽，否则硬件资源冲突
 */
#define USE_CORE1_FLUSH_SCREEN 1

#endif /* PROJECT_CFG_H_ */
