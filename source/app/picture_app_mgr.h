/*
 * demo_app_mgr.h
 *
 *  Created on: 2019年8月19日
 *      Author: jace
 */

#ifndef WEAR_SRC_APP_DEMO_DEMO_APP_MGR_H_
#define WEAR_SRC_APP_DEMO_DEMO_APP_MGR_H_

/*
 * 默认表盘应用
 */
#define SYS_APP_ID_DEFAULT_DIAL     (5)
#define SYS_APP_ID_DEFAULT_DIAL2    (6)
#define SYS_APP_ID_DEFAULT_DIAL3    (7)
#define SYS_APP_ID_DEFAULT_DIAL4    (8)
#define SYS_APP_ID_DEFAULT_DIAL5    (9)
#define SYS_APP_ID_DEFAULT_DIAL6    (10)

#define DEFAULT_START_APP_ID    SYS_APP_ID_DEFAULT_DIAL  /* 默认启动的APP */

#define SYS_APP_ID_APPLIC_BASE (20)
#define SYS_APP_ID_LIGHT        (SYS_APP_ID_APPLIC_BASE)
#define SYS_APP_ID_SETTING      (SYS_APP_ID_APPLIC_BASE+1)
#define SYS_APP_ID_MUSIC        (SYS_APP_ID_APPLIC_BASE+2)
#define SYS_APP_ID_GAME         (SYS_APP_ID_APPLIC_BASE+3)

//系统APP初始化函数
void dial1_init(void);
void dial2_init(void);
void dial3_init(void);
void dial4_init(void);
void dial5_init(void);
void dial6_init(void);

void app_light_init(void);
void app_setting_init(void);
void app_game_init(void);
void app_music_init(void);

/*
 * 默认应用初始化
 *
 */
void internal_app_register();
void default_app_dispatch(void *pevt);

#endif /* WEAR_SRC_APP_DEMO_DEMO_APP_MGR_H_ */
