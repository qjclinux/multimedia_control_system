#ifndef __FS_H__ 
#define __FS_H__ 

#include <stdint.h>

//����״̬
typedef enum{
    FS_RET_SUCCESS          =0,     /* �ɹ� */
    FS_RET_UNKNOW_ERR       =-1,    /* δ֪���� */
    FS_RET_PARAM_ERR        =-2,    /* �������� */
    FS_RET_NOT_READY        =-3,    /* δ��ʼ�� */
    FS_RET_NO_ENOUGH_SPACE  =-4,    /* �ռ䲻�� */
    FS_RET_NO_ENOUGH_FILE   =-5,    /* �ļ��б��� */
    FS_RET_FILE_EXIST       =-6,    /* �ļ��Ѿ����� */
    FS_RET_FILE_NOT_EXIST   =-7,    /* �ļ������� */
    FS_RET_FILE_OVER_SIZE   =-8,    /* �ļ��Ѿ���������ռ��С */
}jacefs_error_t;

typedef uint16_t jacefs_file_id_t;

#define FS_INVALID_FS_ID (0xffff)


/*
 *  FSʹ��RAM���ռ���Ϣ���ļ��������Ļ���
 */
#define FS_USE_RAM_CACHE (1)
#if FS_USE_RAM_CACHE
    void jacefs_sync(void);
#else
    #define jacefs_sync()
#endif

/*
 *  FS�����������ʹ�ö��߳�����Ҫ��
 */
#define FS_USE_LOCK (0)

/*
 *  FS�����ӿ�
 */
jacefs_error_t jacefs_init(void);
jacefs_error_t jacefs_create(jacefs_file_id_t *file_id,int size,uint16_t app_id);
jacefs_error_t jacefs_delete(jacefs_file_id_t file_id,uint16_t app_id);
jacefs_error_t jacefs_delete_all(uint16_t app_id,jacefs_file_id_t except_file_id);
jacefs_error_t jacefs_clean_all(void);

int jacefs_append(jacefs_file_id_t file_id,uint16_t app_id,uint8_t *dat,int size);
int jacefs_write(jacefs_file_id_t file_id,uint16_t app_id,uint8_t *dat,int size,int offset);
int jacefs_read(jacefs_file_id_t file_id,uint16_t app_id,uint8_t *dat,int size,int offset);

jacefs_error_t jacefs_find_app_id(jacefs_file_id_t file_id,uint16_t *app_id,int *offset);
jacefs_error_t jacefs_file_exist(jacefs_file_id_t file_id,uint16_t app_id);
jacefs_error_t jacefs_file_get_phy_addr(jacefs_file_id_t file_id,uint16_t app_id,int offset,uint32_t *phy_addr);

/*
 *  FS���Խӿ�
 */
void jacefs_self_test(void);



#endif

