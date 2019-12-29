/*
******************************************************************************
- file    		jacefs.c 
- author  		jace
- version 		V1.0
- create date	2018.11.11
- brief 
	jacefs Դ�롣
    
- release note:

2018.11.11 V1.0
    �������̣�ʵ��jacefs�ļ�ϵͳ��
    1�������ļ������ӿڣ���ǰ�汾����һ��ҳ����¼�ļ���Ϣ������ļ��� JACEFS_FILE_NUM ����Ϊһҳ�ɴ��ļ���

2019.3.27 V1.1
    �����ļ���ȡ�ӿ�


2019.12.22
    ��ֲ��LPC55s69ƽ̨��

******************************************************************************
*/
#include "stdbool.h"
#include "crc16.h"
#include "string.h"
#include "console.h"
#include "jacefs.h"
#include "jacefs_port.h"
#include "cmsis_gcc.h"

#define DEBUG_FS 0

#if DEBUG_FS
#define FS_LOG(fmt,args...)	do{\
								/*os_printk("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
                                os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
								os_printk(fmt,##args);\
							}while(0)
#define FS_INFO(fmt,args...)	do{\
								os_printk(fmt,##args);\
							}while(0)								
#else
#define FS_LOG(fmt,args...)
#define FS_INFO(fmt,args...)
#endif


//
//----------------------�ļ�ϵͳ�궨��---------------
//

//�ռ����ʼҳ������� FS_HW_START_ADDR ��ҳ����������ʵ�ʵ�����ҳ��
#define SPACE_DESC_START_PAGE (0) 

//�ļ�ϵͳ��ҳ��
#define FS_TOTAL_SIZE (FS_HW_TOTAL_SIZE) 

//�ļ�ϵͳ�ܴ�С
#define FS_TOTAL_PAGE (FS_HW_PAGE_NUM) 

//ҳ��С                            
#define FS_PAGE_SIZE (FS_HW_PAGE_SIZE) 
                            
//1ҳ��װ�ļ���������(ǰ4�ֽ�Ϊ 2�ֽ�CRC16+2�ֽ��ļ���)
#define FILE_DESC_PER_PAGE ((FS_PAGE_SIZE-4)/sizeof(jacefs_fd_t)) 

//�ļ�����ʹ��ҳ(ϵͳ��ʽ��ʱĬ��ֻ��1ҳ�����ļ������ӵ�2ҳ���ٶ�̬����ռ�--��ʵ��)
#define FILE_DESC_PAGE_NUM_DEFAULT (1)

//�ռ���У�ҳʹ������ռ���ֽ�(2�ֽڿ�����256MB��4�ֽڿ����� 16777216 MB)
#define SPACE_BYTE_PER_PAGE 2 

//�ռ�־�У�һҳ���Ա�ʾ��FLASHҳ ��
#define DESC_NUM_PER_PAGE (FS_PAGE_SIZE/SPACE_BYTE_PER_PAGE)
//�ռ��ռ��ҳ
#define SPACE_DESC_PAGE_NUM \
    ( (FS_TOTAL_PAGE+(DESC_NUM_PER_PAGE-1))/(DESC_NUM_PER_PAGE) )

//�ļ�ϵͳ���ɹ���ҳ
#define FS_MANAGE_PAGE_MAX (0x1<<(SPACE_BYTE_PER_PAGE*8)) 

//ϵͳ����ҳ���ռ�����+�ļ�������
#define FS_MANAGE_PAGE_MIN (SPACE_DESC_PAGE_NUM+FILE_DESC_PAGE_NUM_DEFAULT)

#if (FS_MANAGE_PAGE_MAX<FS_TOTAL_PAGE)
    #error  "jacefs page over size!!!"
#elif (FS_TOTAL_PAGE<=FS_MANAGE_PAGE_MIN)
	#error  "jacefs page too little!!!"
#endif

//�ļ�������
#define FILE_DESC_START_PAGE (SPACE_DESC_START_PAGE+SPACE_DESC_PAGE_NUM)

//ҳʹ��������Ŀǰ��֧��2�ֽڵķ�ʽ��
#if SPACE_BYTE_PER_PAGE==2
    typedef uint16_t page_desc_val_t;
//#elif SPACE_BYTE_PER_PAGE==4
//    typedef uint32_t page_desc_val_t;
#else
//    #error  "SPACE_BYTE_PER_PAGE value must be 4 or 2!"
	#error  "SPACE_BYTE_PER_PAGE value must be 2!"
#endif

//ҳת�ɵ�ַ
#define PAGE_TO_ADDR(page) ((page)*FS_PAGE_SIZE+FS_HW_START_ADDR)

//
//----------------------�ļ�ϵͳ���ݽṹ---------------
//
//ҳʹ�ñ�ʶ
typedef enum{
    PAGE_NOT_USED		=0xffff,
    PAGE_SPACE_BLOCK	=0xfffe,
    PAGE_FILE_DESC		=0xfffd,
    
    //0xfffc~0xfffa Ԥ��
    
    PAGE_FILE_END		=0xfff9,
    PAGE_FILE_USING_MAX	=0xfff8,
    PAGE_FILE_USING_MIN	=0x0,
}fs_page_status_t;

//�ļ�����
typedef struct{
    uint16_t id;			/* �ļ�ID */    
	uint16_t app_id;        /* �ļ����ڵ�APP ID */  
    uint32_t size;          /* �ļ���С */
    uint32_t wsize;         /* �ļ���д���С */
	
#if SPACE_BYTE_PER_PAGE==2
    uint16_t start_page;    /* �ļ��洢��ʼҳ */
#else
	#error  "SPACE_BYTE_PER_PAGE value must be 2!"
#endif
	
    /* �ļ���ʶ */
    uint16_t is_continues:1;/* �ļ��Ƿ������洢 */
    uint16_t reserved:15;
}jacefs_fd_t;

typedef struct{
    uint16_t crc16;
    uint16_t file_num;
}jacefs_fd_hdr_t;


//
//----------------------�ļ�ϵͳ�ռ�ҳ���ļ�����ҳ���棨Ϊ����߶�д�ٶȣ���RAM���棩---------------
//
#if FS_USE_RAM_CACHE
    static uint8_t m_info_cache[(SPACE_DESC_PAGE_NUM+FILE_DESC_PAGE_NUM_DEFAULT)*FS_PAGE_SIZE];
    static page_desc_val_t *m_space_desc_cache=(page_desc_val_t *)m_info_cache;
    static uint8_t *m_file_desc_cache=&m_info_cache[SPACE_DESC_PAGE_NUM*FS_PAGE_SIZE];

//    #define PAGE_TO_CACHE_ADDR(page) ((page)*(FS_PAGE_SIZE/SPACE_BYTE_PER_PAGE))

    #define PAGE_TO_SPACE_DESC_ADDR(page) (((page)-SPACE_DESC_START_PAGE)*(FS_PAGE_SIZE/SPACE_BYTE_PER_PAGE))
    #define PAGE_TO_FILE_DESC_ADDR(page) (((page)-FILE_DESC_START_PAGE)*FS_PAGE_SIZE)

    typedef enum{
        FS_SYNC_NONE=0,
        FS_SYNC_SPACE_DESC=0x1<<0,
        FS_SYNC_FILE_DESC=0x1<<1,
        FS_SYNC_ALL=FS_SYNC_SPACE_DESC|FS_SYNC_FILE_DESC,
    }fs_sync_t;
    static uint8_t m_fs_sync=FS_SYNC_NONE;
#endif


//
//----------------------�ļ�ϵͳ����---------------
//
static uint8_t m_fs_ready=false;
static uint32_t m_fs_remainig_size=0;
#if !FS_USE_RAM_CACHE
    static uint8_t m_swap_buf[FS_PAGE_SIZE];
#endif

/*---------------------------------------------------------------------------------------------------*/
#if FS_USE_LOCK
    #ifndef OS_BAREMETAL

    static OS_MUTEX m_fs_lock=NULL;

    static inline void fs_lock()
    {
        if(m_fs_lock==NULL)
        {
            OS_MUTEX_CREATE(m_fs_lock);
        }
        OS_MUTEX_GET(m_fs_lock, OS_MUTEX_FOREVER);
    }

    static inline void fs_unlock()
    {
        OS_MUTEX_PUT(m_fs_lock);
    }

    #endif
#else
    #define fs_lock()
    #define fs_unlock()
#endif
/*---------------------------------------------------------------------------------------------------*/

#if FS_USE_RAM_CACHE
/**
@brief : ������Ϣͬ����flash
                �ú������ڶ�ʱ���е��ã����鶨ʱ10S~30Sͬ��һ��

@param : ��

@retval:
- FS_RET_SUCCESS �ɹ�
- FS_UNKNOW_ERR   ʧ��
*/
void jacefs_sync(void)
{
    int i;
    uint32_t page;
    fs_lock();

    if(m_fs_sync==FS_SYNC_NONE)
    {
        FS_LOG("fs no need sync.\r\n");
        fs_unlock();
        return;
    }

    if(m_fs_sync&FS_SYNC_SPACE_DESC)
    {
        do{
            for(i=0;i<SPACE_DESC_PAGE_NUM;i++)
            {
                page=SPACE_DESC_START_PAGE+i;
                if(fs_port_control(FS_CTL_ERASE_PAGE,&page)!=FS_RET_SUCCESS)
                {
                    FS_LOG("erase page=%d error!\r\n",page);
                    break ;
                }
            }

            if(fs_port_write( PAGE_TO_ADDR(SPACE_DESC_START_PAGE),
                SPACE_DESC_PAGE_NUM*FS_PAGE_SIZE,(uint8_t*)m_space_desc_cache)
                !=SPACE_DESC_PAGE_NUM*FS_PAGE_SIZE)
            {
                FS_LOG("write err!\r\n");
                break;
            }

            m_fs_sync&=~FS_SYNC_SPACE_DESC;
        }while(0);
        FS_LOG("fs sync space desc!\r\n");
    }

    if(m_fs_sync&FS_SYNC_FILE_DESC)
    {
        do{
            for(i=0;i<FILE_DESC_PAGE_NUM_DEFAULT;i++)
            {
                page=FILE_DESC_START_PAGE+i;
                if(fs_port_control(FS_CTL_ERASE_PAGE,&page)!=FS_RET_SUCCESS)
                {
                    FS_LOG("erase page=%d error!\r\n",page);
                    break ;
                }
            }

            if(fs_port_write( PAGE_TO_ADDR(FILE_DESC_START_PAGE),
                FILE_DESC_PAGE_NUM_DEFAULT*FS_PAGE_SIZE,(uint8_t*)m_file_desc_cache)
                !=FILE_DESC_PAGE_NUM_DEFAULT*FS_PAGE_SIZE)
            {
                FS_LOG("write err!\r\n");
                break;
            }

            m_fs_sync&=~FS_SYNC_FILE_DESC;
        }while(0);
        FS_LOG("fs sync space desc!\r\n");
    }
    fs_unlock();
}

//static void fs_sync_timer_callback(TimerHandle_t id )
//{
//    /*
//     * Ϊ�˽�ʡ�ڴ棬���ڰѶ�ʱ��ջ��С�ˣ����Բ�Ҫ������ֱ�Ӳ���flash���Է�ջ���
//     */
//    //jacefs_sync();
//
//    sys_event_t evt;
//    evt.event_type = SYS_EVENT_FS_SYNC;
//    evt.event_data=jacefs_sync;
//    if ( xQueueSend( m_sys_evt_queue,( void * ) &evt,0 ) != pdPASS)
//    {
//        //TODO:fix this error.
//    }
//}

static int fs_sync_timer_init()
{
//    TimerHandle_t hdl;
//    hdl=OS_TIMER_CREATE(NULL,
//        OS_MS_2_TICKS(10*1000),
//        pdTRUE,
//        NULL,
//        fs_sync_timer_callback
//        );
//
//    if(OS_TIMER_START(hdl,0)!= pdPASS)
//    {
//        FS_LOG("fs sync timer create err! \r\n");
//        return FS_RET_UNKNOW_ERR;
//    }
    FS_LOG("fs sync timer create success! \r\n");
    return FS_RET_SUCCESS;
}
#endif


/*---------------------------------------------------------------------------------------------------*/

/**
@brief : ��ʽ��Ϊ jacefs
		
@param : ��

@retval:
- FS_RET_SUCCESS ��ʽ���ɹ� 
- FS_UNKNOW_ERR   ��ʽ��ʧ��
*/
__STATIC_INLINE jacefs_error_t format_to_jacefs(void)
{
    int i;
//    uint32_t space_block_page=SPACE_DESC_START_PAGE;
    page_desc_val_t desc_val;
    uint32_t page;
    uint32_t offset;
    
    //�ռ�顢�ļ������� ����...
    for(i=0;i<(SPACE_DESC_PAGE_NUM+FILE_DESC_PAGE_NUM_DEFAULT);i++)
    {
        page=SPACE_DESC_START_PAGE+i;
        if(fs_port_control(FS_CTL_ERASE_PAGE,&page)!=FS_RET_SUCCESS)
        {
            FS_LOG("erase page=%d error!\r\n",page);
            return FS_RET_UNKNOW_ERR;
        }
    }
    
    //�ռ�飺�ռ��д���ʶ
    desc_val=PAGE_SPACE_BLOCK;
    for(i=0;i<SPACE_DESC_PAGE_NUM;i++)
    {
        offset=sizeof(page_desc_val_t)*i;
        if(fs_port_write( PAGE_TO_ADDR(SPACE_DESC_START_PAGE)+offset,
            sizeof(page_desc_val_t),(uint8_t*)&desc_val)!=sizeof(page_desc_val_t))
        {
            FS_LOG("write err!\r\n");
            return FS_RET_UNKNOW_ERR;
        }
    }
    
    //�ռ�飺�ļ�������д��ʶ--ֻдϵͳĬ�ϵ��ļ�����ҳ
    desc_val=PAGE_FILE_DESC;
	for(i=SPACE_DESC_PAGE_NUM;i<(SPACE_DESC_PAGE_NUM+FILE_DESC_PAGE_NUM_DEFAULT);i++)
    {
        offset=sizeof(page_desc_val_t)*i;
        if(fs_port_write( PAGE_TO_ADDR(SPACE_DESC_START_PAGE)+offset,
            sizeof(page_desc_val_t),(uint8_t*)&desc_val)!=sizeof(page_desc_val_t))
        {
            FS_LOG("write err!\r\n");
            return FS_RET_UNKNOW_ERR;
        }
    }
    
    //�ļ�������У��
    jacefs_fd_hdr_t hd;
    hd.file_num=0;
    hd.crc16=crc16_compute((uint8_t*)&hd.file_num,sizeof(jacefs_fd_hdr_t)-2,0);
	for(i=0;i<FILE_DESC_PAGE_NUM_DEFAULT;i++)
    {
		if(fs_port_write( PAGE_TO_ADDR(FILE_DESC_START_PAGE+i),
			sizeof(jacefs_fd_hdr_t),(uint8_t*)&hd)!=sizeof(jacefs_fd_hdr_t))
		{
			FS_LOG("write err!\r\n");
			return FS_RET_UNKNOW_ERR;
		}
	}
    
    return FS_RET_SUCCESS;
}

/**
@brief : ����ʣ��ռ�
		
@param : ��

@retval:
- @jacefs_error_t
*/
__STATIC_INLINE jacefs_error_t calculate_remaining_size()
{
//    if(m_fs_ready!=true)
//    {
//        return FS_RET_NOT_READY;
//    }
    
    page_desc_val_t page_desc_val;
    uint32_t addr;
    uint32_t page;
    
    m_fs_remainig_size=0;
    page=0;
    
    for(int i=0;i<SPACE_DESC_PAGE_NUM;i++)
    {
#if !FS_USE_RAM_CACHE
        addr=PAGE_TO_ADDR(SPACE_DESC_START_PAGE+i);
#else
        addr=PAGE_TO_SPACE_DESC_ADDR(SPACE_DESC_START_PAGE+i);
#endif

        for(int j=0;j<(FS_PAGE_SIZE/SPACE_BYTE_PER_PAGE);j++)
        {
#if !FS_USE_RAM_CACHE
            if(fs_port_read(addr+sizeof(page_desc_val_t)*j,
                sizeof(page_desc_val_t),(uint8_t*)&page_desc_val)<=0)
            {
                FS_LOG("read error!\r\n");
                return FS_RET_UNKNOW_ERR;
            }
#else
            page_desc_val=m_space_desc_cache[addr+j];
#endif
            if(page_desc_val==PAGE_NOT_USED)
            {
                m_fs_remainig_size+=FS_PAGE_SIZE;
            }
            
			//�ռ������鲻һ���õ��ֻ꣬�ж���ҳ���ڵĿռ�
            page++;
            if(page>=FS_TOTAL_PAGE)
                break;
        }
    }
    
    OS_LOG("total size=%d (%dKB),remaining=%d(%d KB) ,used=%d(%d KB)\r\n",
        FS_TOTAL_SIZE,FS_TOTAL_SIZE/1024,
        m_fs_remainig_size,m_fs_remainig_size/1024,
        FS_TOTAL_SIZE-m_fs_remainig_size,(FS_TOTAL_SIZE-m_fs_remainig_size)/1024);
    
    return FS_RET_SUCCESS;
}


/**
@brief : Ѱ���ļ�����ҳ
		
@param : 
- start_page ���� SPACE_DESC_START_PAGE ��ҳ�����Ӹ�ҳ��ʼѰ�Һ�����ļ�����ҳ

@retval:
- >0 �ļ�����ҳ�±�
- ==0 û���ҵ�
*/
__STATIC_INLINE uint32_t find_file_desc_page(uint32_t start_page)
{
    page_desc_val_t page_desc_val;
    uint32_t addr;
	
    for(uint32_t i=start_page;
        i<FILE_DESC_START_PAGE+FILE_DESC_PAGE_NUM_DEFAULT;//i<FS_TOTAL_PAGE;
        i++)
    {
#if !FS_USE_RAM_CACHE
        addr=PAGE_TO_ADDR(SPACE_DESC_START_PAGE)+i*sizeof(page_desc_val_t);
        if(fs_port_read(addr,sizeof(page_desc_val_t),(uint8_t*)&page_desc_val)
            !=sizeof(page_desc_val_t))
        {
            break;
        }
#else
        addr=PAGE_TO_SPACE_DESC_ADDR(SPACE_DESC_START_PAGE)+i;
        page_desc_val=m_space_desc_cache[addr];
#endif

		if(page_desc_val==PAGE_FILE_DESC)
		{
			FS_LOG("found page=%d\r\n",i);
			return i;
		}
    }
    
//	FS_LOG("not found !\r\n");
    return 0;
}

/**
@brief : ��ȡ�ļ�ĳҳ���ӵ���һҳ
		
@param : 
- page �ļ�ҳ

@retval:
- >0 �ļ����ӵ���һҳ
- ==0 û���ҵ�
*/
__STATIC_INLINE page_desc_val_t get_file_next_page(uint32_t page)
{
    uint32_t addr;

#if !FS_USE_RAM_CACHE

    page_desc_val_t page_desc_val;
	addr=PAGE_TO_ADDR(SPACE_DESC_START_PAGE)+page*sizeof(page_desc_val_t);
	
	if(fs_port_read(addr,sizeof(page_desc_val_t),(uint8_t*)&page_desc_val)
		==sizeof(page_desc_val_t))
	{
		return page_desc_val;
	}
	return 0;

#else

	addr=PAGE_TO_SPACE_DESC_ADDR(SPACE_DESC_START_PAGE)+page;
	return m_space_desc_cache[addr];

#endif
}

/**
@brief : ����ռ�
		
@param : 
-req_size	����Ŀռ��С
-start_page	���뵽����ʼҳ��ַ

@retval:
- @jacefs_error_t
*/
__STATIC_INLINE jacefs_error_t alloc_page(uint32_t req_size,uint16_t *start_page)
{
    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }
    
    page_desc_val_t page_desc_val;
    uint32_t addr;
    uint32_t last_alloc_page;
	
    uint32_t alloc_size;
	alloc_size=0;
    *start_page=0;
	last_alloc_page=0;
	
	for(uint32_t i=0;i<FS_TOTAL_PAGE;i++)
    {
#if !FS_USE_RAM_CACHE
        addr=PAGE_TO_ADDR(SPACE_DESC_START_PAGE)+i*sizeof(page_desc_val_t);
        
		if(fs_port_read(addr,sizeof(page_desc_val_t),(uint8_t*)&page_desc_val)
			!=sizeof(page_desc_val_t))
		{
			FS_LOG("read error!\r\n");
            return FS_RET_UNKNOW_ERR;
		}
#else
		addr=PAGE_TO_SPACE_DESC_ADDR(SPACE_DESC_START_PAGE)+i;
		page_desc_val=m_space_desc_cache[addr];
#endif

		if(page_desc_val==PAGE_NOT_USED)
		{
			if(*start_page==0)
				*start_page=i;
			
			alloc_size+=FS_PAGE_SIZE;
			
			if(m_fs_remainig_size>=FS_PAGE_SIZE)
				m_fs_remainig_size-=FS_PAGE_SIZE;
			else
				m_fs_remainig_size=0;
			
			FS_LOG("alloc page=%d,rem=%d\r\n",i,m_fs_remainig_size);
			
			//��һ�η����ҳ���ӵ����η����ҳ
			if(last_alloc_page!=0)
			{
#if !FS_USE_RAM_CACHE
				addr=PAGE_TO_ADDR(SPACE_DESC_START_PAGE)+last_alloc_page*sizeof(page_desc_val_t);
				
				page_desc_val=i;
				if(fs_port_write(addr,sizeof(page_desc_val_t),(uint8_t*)&page_desc_val)
					!=sizeof(page_desc_val_t))
				{
					FS_LOG("write error!\r\n");
					return FS_RET_UNKNOW_ERR;
				}
#else
				addr=PAGE_TO_SPACE_DESC_ADDR(SPACE_DESC_START_PAGE)+last_alloc_page;
				m_space_desc_cache[addr]=(page_desc_val_t)i;
#endif
			}
			
			if(alloc_size>=req_size)
			{
#if !FS_USE_RAM_CACHE
				addr=PAGE_TO_ADDR(SPACE_DESC_START_PAGE)+i*sizeof(page_desc_val_t);
				
				//�ļ�����
				page_desc_val=PAGE_FILE_END;
				if(fs_port_write(addr,sizeof(page_desc_val_t),(uint8_t*)&page_desc_val)
					!=sizeof(page_desc_val_t))
				{
					FS_LOG("write error!\r\n");
					return FS_RET_UNKNOW_ERR;
				}
#else
				addr=PAGE_TO_SPACE_DESC_ADDR(SPACE_DESC_START_PAGE)+i;
				m_space_desc_cache[addr]=PAGE_FILE_END;
#endif
				break;
			}
			
			last_alloc_page=i;
		}
    }
    
    FS_LOG("remaining size=%d(%d KB),used=%d(%d KB),req=%d (alloc=%d),page=%d\r\n",
        m_fs_remainig_size,m_fs_remainig_size/1024,
        FS_TOTAL_SIZE-m_fs_remainig_size,(FS_TOTAL_SIZE-m_fs_remainig_size)/1024,
		req_size,alloc_size,*start_page);
    
    //���ʹ��RAM���棬RAM����ͬ����flash
#if FS_USE_RAM_CACHE
    m_fs_sync|=FS_SYNC_SPACE_DESC;
#endif

    return FS_RET_SUCCESS;
}
/**
@brief : ���տռ�
		
@param : 
-start_page	���յ���ʼҳ

@retval:
- @jacefs_error_t
*/
__STATIC_INLINE jacefs_error_t free_page(uint16_t start_page)
{
    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }

#if !FS_USE_RAM_CACHE
    uint32_t space_desc_addr,
			space_desc_next_addr;
	uint16_t page,next_page;
	uint16_t offset_in_page;
	page_desc_val_t *val;
	
	next_page=start_page;
	
	do
    {
        space_desc_addr=PAGE_TO_ADDR(SPACE_DESC_START_PAGE)
						+(next_page*SPACE_BYTE_PER_PAGE/FS_PAGE_SIZE*FS_PAGE_SIZE);
		
		FS_LOG("page=%d,space_desc_addr=%x!\r\n",next_page,space_desc_addr);
		
		//��ȡһ��ҳ
		if(fs_port_read( space_desc_addr,FS_PAGE_SIZE,m_swap_buf)
				!=FS_PAGE_SIZE )
		{
			FS_LOG("read err!\r\n");
			return FS_RET_UNKNOW_ERR;
		}
		
		//���տռ�����ҳ��һ�� �ռ�
		offset_in_page=(next_page*SPACE_BYTE_PER_PAGE)%(FS_PAGE_SIZE);
		val=(page_desc_val_t*)&m_swap_buf[offset_in_page];
		
		FS_LOG("free page=%d,offset_in_page=%d,",
			next_page,offset_in_page);
		
		next_page=*val;
		*val=PAGE_NOT_USED;
		m_fs_remainig_size+=FS_PAGE_SIZE;
		
		FS_INFO("next free page=%d\r\n",
			next_page);
		
		//����ͬһ�ռ�����ҳʣ�� �ռ�
free_page_rem:
		if(next_page!=PAGE_FILE_END)
		{
			space_desc_next_addr=PAGE_TO_ADDR(SPACE_DESC_START_PAGE)
						+(next_page*SPACE_BYTE_PER_PAGE/FS_PAGE_SIZE*FS_PAGE_SIZE);
			
			if( space_desc_next_addr == space_desc_addr)
			{
				offset_in_page=(next_page*SPACE_BYTE_PER_PAGE)%(FS_PAGE_SIZE);
				val=(page_desc_val_t*)&m_swap_buf[offset_in_page];
				
				FS_LOG("free page=%d,offset_in_page=%d,",
					next_page,offset_in_page);
				
				next_page=*val;
				*val=PAGE_NOT_USED;
				m_fs_remainig_size+=FS_PAGE_SIZE;
				
				FS_INFO("next free page=%d\r\n",
					next_page);
				
				goto free_page_rem;
			}
			
			//����ԭҳ
			page=(space_desc_addr-FS_HW_START_ADDR)/FS_PAGE_SIZE;
			if(fs_port_control(FS_CTL_ERASE_PAGE,&page)!=FS_RET_SUCCESS)
			{
				return FS_RET_UNKNOW_ERR;
			}
			
			//д��������
			if(fs_port_write( space_desc_addr,FS_PAGE_SIZE,m_swap_buf)
				!=FS_PAGE_SIZE )
			{
				FS_LOG("write error!\r\n");
				return FS_RET_UNKNOW_ERR;
			}
			
			//��ȡ��һ��ռ�����ҳ
			continue;
		}
		
		//����ԭҳ
		page=(space_desc_addr-FS_HW_START_ADDR)/FS_PAGE_SIZE;
		if(fs_port_control(FS_CTL_ERASE_PAGE,&page)!=FS_RET_SUCCESS)
		{
			return FS_RET_UNKNOW_ERR;
		}
		
		//д��������
		if(fs_port_write(space_desc_addr,FS_PAGE_SIZE,m_swap_buf)
				!=FS_PAGE_SIZE )
		{
			FS_LOG("write error!\r\n");
			return FS_RET_UNKNOW_ERR;
		}
		
		//���˿ռ�������
		break;
		
		
    }
	while(1);
#else
	uint16_t next_page;
	uint32_t addr;

	next_page=start_page;
    do
    {
        addr=PAGE_TO_SPACE_DESC_ADDR(SPACE_DESC_START_PAGE)+next_page;

        FS_LOG("free page=%d,",next_page);

        next_page=m_space_desc_cache[addr];

        FS_INFO("next free page=%d\r\n",next_page);

        m_space_desc_cache[addr]=PAGE_NOT_USED;
        m_fs_remainig_size+=FS_PAGE_SIZE;
        if(next_page==PAGE_FILE_END)
        {
            break;
        }

    }while(1);

    //���ʹ��RAM���棬RAM����ͬ����flash
#if FS_USE_RAM_CACHE
    m_fs_sync|=FS_SYNC_SPACE_DESC;
#endif

#endif

    FS_LOG("free space finish,remaining size=%d(%d KB),used=%d(%d KB),start_page=%d\r\n",
        m_fs_remainig_size,m_fs_remainig_size/1024,
        FS_TOTAL_SIZE-m_fs_remainig_size,(FS_TOTAL_SIZE-m_fs_remainig_size)/1024,
		start_page);

    return FS_RET_SUCCESS;
}
/**
@brief : ����ļ��Ƿ����
		
@param : 
-file_id 	���ҵ��ļ�ID
-app_id		���ҵ�APP ID

@retval:
- true �ļ�����
- false �ļ�������
*/
__STATIC_INLINE bool file_exist(jacefs_file_id_t file_id,uint16_t app_id)
{
#if !FS_USE_RAM_CACHE
    jacefs_fd_hdr_t fd_hdr;
#else
    jacefs_fd_hdr_t *fd_hdr;
#endif

    jacefs_fd_t *fd;
    uint32_t page,next_page;
	
    
	next_page=FILE_DESC_START_PAGE-SPACE_DESC_START_PAGE;
	page=0;
	
	while(1)
	{
		page=find_file_desc_page(next_page);
		
		if(page==0)
			break;
		next_page=page+1;
		
#if !FS_USE_RAM_CACHE
		if(fs_port_read( PAGE_TO_ADDR(page),
			sizeof(jacefs_fd_hdr_t),(uint8_t*)&fd_hdr)!=sizeof(jacefs_fd_hdr_t))
		{
			FS_LOG("read err!\r\n");
			continue;
		}
		
		if(fd_hdr.file_num==0 || fd_hdr.file_num>FILE_DESC_PER_PAGE)
			continue;
		
		
		//���ļ����� ���Ƶ����������ڴ棩
		if(fs_port_read( PAGE_TO_ADDR(page)+sizeof(jacefs_fd_hdr_t),
			fd_hdr.file_num*sizeof(jacefs_fd_t),m_swap_buf) 
				!=fd_hdr.file_num*sizeof(jacefs_fd_t) )
		{
			FS_LOG("read err!\r\n");
			continue;
		}
		
		fd=(jacefs_fd_t *)m_swap_buf;
		
		for(uint16_t i=0;i<fd_hdr.file_num;i++ )
		{
			if(fd[i].id==file_id && fd[i].app_id==app_id)
			{
				FS_LOG("found file=%d,app_id=%d in page=%d num=%d!\r\n",
					file_id,app_id,page,i);
				return true;
			}
		}
#else

		fd_hdr=(jacefs_fd_hdr_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)];

        if(fd_hdr->file_num==0 || fd_hdr->file_num>FILE_DESC_PER_PAGE)
            continue;

        fd=(jacefs_fd_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)+sizeof(jacefs_fd_hdr_t)];

        for(uint16_t i=0;i<fd_hdr->file_num;i++ )
        {
            if(fd[i].id==file_id && fd[i].app_id==app_id)
            {
                FS_LOG("found file=%d,app_id=%d in page=%d num=%d!\r\n",
                    file_id,app_id,page,i);
                return true;
            }
        }
#endif
		
	}
	
	FS_LOG("not found file=%d,app_id=%d !\r\n",file_id,app_id);
	return false;
}

/**
@brief : ��ȡ�ļ�����
		
@param : 
-file_id 	���ҵ��ļ�ID
-app_id		���ҵ�APP ID
-fd			���ز��ҵ����ļ�����

@retval:
- @jacefs_error_t
*/
__STATIC_INLINE jacefs_error_t get_file_desc(jacefs_file_id_t file_id,uint16_t app_id,jacefs_fd_t *fd)
{
#if !FS_USE_RAM_CACHE
    jacefs_fd_hdr_t fd_hdr;
#else
    jacefs_fd_hdr_t *fd_hdr;
#endif
    uint32_t page,next_page;
	jacefs_fd_t *search_fd;
	
	next_page=FILE_DESC_START_PAGE-SPACE_DESC_START_PAGE;
	page=0;
	
	while(1)
	{
		page=find_file_desc_page(next_page);
		
		if(page==0)
			break;
		next_page=page+1;
		
#if !FS_USE_RAM_CACHE
		if(fs_port_read( PAGE_TO_ADDR(page),
			sizeof(jacefs_fd_hdr_t),(uint8_t*)&fd_hdr)!=sizeof(jacefs_fd_hdr_t))
		{
			FS_LOG("read err!\r\n");
			continue;
		}
		
		if(fd_hdr.file_num==0 || fd_hdr.file_num>FILE_DESC_PER_PAGE)
			continue;
		
		
		//���ļ����� ���Ƶ����������ڴ棩
		if(fs_port_read( PAGE_TO_ADDR(page)+sizeof(jacefs_fd_hdr_t),
			fd_hdr.file_num*sizeof(jacefs_fd_t),m_swap_buf) 
				!=fd_hdr.file_num*sizeof(jacefs_fd_t) )
		{
			FS_LOG("read err!\r\n");
			continue;
		}
		
		search_fd=(jacefs_fd_t *)m_swap_buf;
		
		for(uint16_t i=0;i<fd_hdr.file_num;i++ )
		{
			if(search_fd[i].id==file_id && search_fd[i].app_id==app_id)
			{
				if(fd)
					*fd=search_fd[i];
				
				FS_LOG("found file=%d,app_id=%d in page=%d num=%d!\r\n",
					file_id,app_id,page,i);
				return FS_RET_SUCCESS;
			}
		}
#else
		fd_hdr=(jacefs_fd_hdr_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)];

        if(fd_hdr->file_num==0 || fd_hdr->file_num>FILE_DESC_PER_PAGE)
            continue;

        search_fd=(jacefs_fd_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)+sizeof(jacefs_fd_hdr_t)];

        for(uint16_t i=0;i<fd_hdr->file_num;i++ )
        {
            if(search_fd[i].id==file_id && search_fd[i].app_id==app_id)
            {
                if(fd)
                    *fd=search_fd[i];

                FS_LOG("found file=%d,app_id=%d in page=%d num=%d!\r\n",
                    file_id,app_id,page,i);
                return FS_RET_SUCCESS;
            }
        }
#endif
	}
	
	FS_LOG("not found file=%d,app_id=%d !\r\n",file_id,app_id);
	return FS_RET_FILE_NOT_EXIST;
}

/**
@brief : �޸��ļ�����
		
@param : 
-file_id 	�ļ���ӦID
-app_id		�ļ���ӦAPP ID
-fd			���ض�Ӧ���ļ�����

@retval:
- @jacefs_error_t
*/
__STATIC_INLINE jacefs_error_t set_file_desc(jacefs_fd_t fd)
{
#if !FS_USE_RAM_CACHE
    jacefs_fd_hdr_t fd_hdr;
#else
    jacefs_fd_hdr_t *fd_hdr;
#endif
    uint32_t page,next_page;
	jacefs_fd_t *search_fd;
	
	next_page=FILE_DESC_START_PAGE-SPACE_DESC_START_PAGE;
	page=0;
	
	while(1)
	{
		page=find_file_desc_page(next_page);
		
		if(page==0)
			break;
		next_page=page+1;
		
#if !FS_USE_RAM_CACHE
		if(fs_port_read( PAGE_TO_ADDR(page),
			sizeof(jacefs_fd_hdr_t),(uint8_t*)&fd_hdr)!=sizeof(jacefs_fd_hdr_t))
		{
			FS_LOG("read err!\r\n");
			continue;
		}
		
		if(fd_hdr.file_num==0 || fd_hdr.file_num>FILE_DESC_PER_PAGE)
			continue;
		
		if(fs_port_read( PAGE_TO_ADDR(page)+sizeof(jacefs_fd_hdr_t),
			fd_hdr.file_num*sizeof(jacefs_fd_t),m_swap_buf) 
				!=fd_hdr.file_num*sizeof(jacefs_fd_t) )
		{
			FS_LOG("read err!\r\n");
			continue;
		}
		
		search_fd=(jacefs_fd_t *)m_swap_buf;
		
		for(uint16_t i=0;i<fd_hdr.file_num;i++ )
		{
			if(search_fd[i].id==fd.id && search_fd[i].app_id==fd.app_id)
			{
				search_fd[i]=fd;
				
				FS_LOG("found & set file=%d,app_id=%d,start_page=%d ,ws=%d ,ts=%d in page=%d num=%d,!\r\n",
					fd.id,fd.app_id,fd.start_page,fd.wsize,fd.size,page,i);
				
				//���¼��飬��д
				fd_hdr.crc16=crc16_compute((uint8_t*)&fd_hdr.file_num,sizeof(jacefs_fd_hdr_t)-2,0);
				fd_hdr.crc16=crc16_compute(m_swap_buf,
							fd_hdr.file_num*sizeof(jacefs_fd_t),&fd_hdr.crc16);
				
				//���ļ������ӽ��������ڴ棩��д
				if(fs_port_control(FS_CTL_ERASE_PAGE,&page)!=FS_RET_SUCCESS)
				{
					return FS_RET_UNKNOW_ERR;
				}
				
				if(fs_port_write( PAGE_TO_ADDR(page),sizeof(jacefs_fd_hdr_t),(uint8_t*)&fd_hdr)
					!=sizeof(jacefs_fd_hdr_t))
				{
					FS_LOG("write err!\r\n");
					return FS_RET_UNKNOW_ERR;
				}
				
				if(fd_hdr.file_num!=0)
				{
					if(fs_port_write( PAGE_TO_ADDR(page)+sizeof(jacefs_fd_hdr_t),
						fd_hdr.file_num*sizeof(jacefs_fd_t),m_swap_buf)
						!=fd_hdr.file_num*sizeof(jacefs_fd_t))
					{
						FS_LOG("write err!\r\n");
						return FS_RET_UNKNOW_ERR;
					}
				}
				
				return FS_RET_SUCCESS;
			}
		}
#else
		fd_hdr=(jacefs_fd_hdr_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)];

        if(fd_hdr->file_num==0 || fd_hdr->file_num>FILE_DESC_PER_PAGE)
            continue;

        search_fd=(jacefs_fd_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)+sizeof(jacefs_fd_hdr_t)];

        for(uint16_t i=0;i<fd_hdr->file_num;i++ )
        {
            if(search_fd[i].id==fd.id && search_fd[i].app_id==fd.app_id)
            {
                //���»���
                search_fd[i]=fd;

                FS_LOG("found & set file=%d,app_id=%d,start_page=%d ,ws=%d ,ts=%d in page=%d num=%d,!\r\n",
                    fd.id,fd.app_id,fd.start_page,fd.wsize,fd.size,page,i);

                //���¼���
                fd_hdr->crc16=crc16_compute((uint8_t*)&fd_hdr->file_num,
                    sizeof(jacefs_fd_hdr_t)-2+fd_hdr->file_num*sizeof(jacefs_fd_t),0);

                //���ʹ��RAM���棬RAM����ͬ����flash
                m_fs_sync|=FS_SYNC_FILE_DESC;


                return FS_RET_SUCCESS;
            }
        }
#endif
	}
	
	FS_LOG("not found file=%d,app_id=%d !\r\n",fd.id,fd.app_id);
	return FS_RET_FILE_NOT_EXIST;
}

/**
@brief : ɾ���ļ�����
		
@param : 
-file_id 	ɾ�����ļ���ӦID
-app_id		ɾ�����ļ���ӦAPP ID
-fd			���ض�Ӧ���ļ�����
-delete_any_file	 trueΪʹ��ɾ������һ���ļ�����ɾ��һ��app_id��ͬ���ļ�����falseΪapp_id��app_id��ͬʱ��ɾ��
-del_except_file_id  ���delete_any_file==true����ɾ������ļ��� del_except_file_id==FS_INVALID_FS_ID ʱ��������ʹ��

@retval:
- @jacefs_error_t
*/
__STATIC_INLINE jacefs_error_t delete_file_desc(jacefs_file_id_t file_id,uint16_t app_id,jacefs_fd_t *fd,
    bool delete_any_file,
    jacefs_file_id_t del_except_file_id)
{
#if !FS_USE_RAM_CACHE
    jacefs_fd_hdr_t fd_hdr;
#else
    jacefs_fd_hdr_t *fd_hdr;
#endif
    uint32_t page,next_page;
	jacefs_fd_t *search_fd;
	
	next_page=FILE_DESC_START_PAGE-SPACE_DESC_START_PAGE;
	page=0;
	
	while(1)
	{
		page=find_file_desc_page(next_page);
		
		if(page==0)
			break;
		next_page=page+1;
		
#if !FS_USE_RAM_CACHE
		if(fs_port_read( PAGE_TO_ADDR(page),
			sizeof(jacefs_fd_hdr_t),(uint8_t*)&fd_hdr)!=sizeof(jacefs_fd_hdr_t))
		{
			FS_LOG("read err!\r\n");
			continue;
		}
		
		if(fd_hdr.file_num==0 || fd_hdr.file_num>FILE_DESC_PER_PAGE)
			continue;
		
		if(fs_port_read( PAGE_TO_ADDR(page)+sizeof(jacefs_fd_hdr_t),
			fd_hdr.file_num*sizeof(jacefs_fd_t),m_swap_buf) 
				!=fd_hdr.file_num*sizeof(jacefs_fd_t) )
		{
			FS_LOG("read err!\r\n");
			continue;
		}
		
		search_fd=(jacefs_fd_t *)m_swap_buf;
		
		for(uint16_t i=0;i<fd_hdr.file_num;i++ )
		{
			if((search_fd[i].id==file_id || delete_any_file) && search_fd[i].app_id==app_id)
			{
				if(fd)
					*fd=search_fd[i];
				
				FS_LOG("found & delete file=%d,app_id=%d,start_page=%d in page=%d num=%d!\r\n",
					fd->id,fd->app_id,fd->start_page,page,i);
				
				//ɾ���ļ��������Ѻ�����ļ�������ǰ�ƶ�
				if(i<fd_hdr.file_num-1)
				{
					memcpy(&search_fd[i],&search_fd[i+1],(fd_hdr.file_num-i-1)*sizeof(jacefs_fd_t));
				}
				
				//���¼��飬��д
				fd_hdr.file_num--;
				fd_hdr.crc16=crc16_compute((uint8_t*)&fd_hdr.file_num,sizeof(jacefs_fd_hdr_t)-2,0);
				
				if(fd_hdr.file_num!=0)
				{
					fd_hdr.crc16=crc16_compute(m_swap_buf,
								fd_hdr.file_num*sizeof(jacefs_fd_t),&fd_hdr.crc16);
				}
				
				//���ļ������ӽ��������ڴ棩��д
				if(fs_port_control(FS_CTL_ERASE_PAGE,&page)!=FS_RET_SUCCESS)
				{
					return FS_RET_UNKNOW_ERR;
				}
				
				if(fs_port_write( PAGE_TO_ADDR(page),sizeof(jacefs_fd_hdr_t),(uint8_t*)&fd_hdr)
					!=sizeof(jacefs_fd_hdr_t))
				{
					FS_LOG("write err!\r\n");
					return FS_RET_UNKNOW_ERR;
				}
				
				if(fd_hdr.file_num!=0)
				{
					if(fs_port_write( PAGE_TO_ADDR(page)+sizeof(jacefs_fd_hdr_t),
						fd_hdr.file_num*sizeof(jacefs_fd_t),m_swap_buf)
						!=fd_hdr.file_num*sizeof(jacefs_fd_t))
					{
						FS_LOG("write err!\r\n");
						return FS_RET_UNKNOW_ERR;
					}
				}
				
				return FS_RET_SUCCESS;
			}
		}
#else
		fd_hdr=(jacefs_fd_hdr_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)];

        if(fd_hdr->file_num==0 || fd_hdr->file_num>FILE_DESC_PER_PAGE)
            continue;

        search_fd=(jacefs_fd_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)+sizeof(jacefs_fd_hdr_t)];

        for(uint16_t i=0;i<fd_hdr->file_num;i++ )
        {
            if((search_fd[i].id==file_id || delete_any_file) && search_fd[i].app_id==app_id)
            {
                //�����ɾ����һ�ļ������˲�ɾ�����ļ�
                if(delete_any_file && del_except_file_id==search_fd[i].app_id)
                {
                    continue;
                }

                if(fd)
                    *fd=search_fd[i];

                FS_LOG("found & delete file=%d,app_id=%d,start_page=%d in page=%d num=%d!\r\n",
                    fd->id,fd->app_id,fd->start_page,page,i);

                //ɾ���ļ��������Ѻ�����ļ�������ǰ�ƶ�
                if(i<fd_hdr->file_num-1)
                {
                    memcpy(&search_fd[i],&search_fd[i+1],(fd_hdr->file_num-i-1)*sizeof(jacefs_fd_t));
                }

                //���¼��飬��д
                fd_hdr->file_num--;
                fd_hdr->crc16=crc16_compute((uint8_t*)&fd_hdr->file_num,
                    sizeof(jacefs_fd_hdr_t)-2+fd_hdr->file_num*sizeof(jacefs_fd_t),0);

                //���ʹ��RAM���棬RAM����ͬ����flash
                m_fs_sync|=FS_SYNC_FILE_DESC;

                return FS_RET_SUCCESS;
            }
        }
#endif
	}
	
	FS_LOG("not found file=%d,app_id=%d !\r\n",file_id,app_id);
	return FS_RET_FILE_NOT_EXIST;
}

/**
@brief : ��ʼ���ļ�ϵͳ

	1 ��ʼ��Ӳ���ӿ�
	2 ���ռ��������Ƿ����
	3 ����ļ��������Ƿ���ڡ�У��ͨ��
	4 ����ʣ��ռ�

@param : ��

@retval:
- @jacefs_error_t
*/
jacefs_error_t jacefs_init(void)
{
    int i;
	jacefs_error_t ret;

    if(m_fs_ready==true)
    {
        return FS_RET_SUCCESS;
    }
    
    fs_lock();

    if(fs_port_init()!=FS_RET_SUCCESS)
    {
        FS_LOG("hw init error!\r\n");
        fs_unlock();
        return FS_RET_UNKNOW_ERR;
    }
    
    //�鿴�ļ�ϵͳ��ʼҳ�Ƿ��ѱ�ʶΪ�ռ��
    page_desc_val_t page_desc_val;
    for(i=0;i<SPACE_DESC_PAGE_NUM;i++)
    {
        if(fs_port_read( PAGE_TO_ADDR(SPACE_DESC_START_PAGE)+sizeof(page_desc_val_t)*i,
            sizeof(page_desc_val_t),(uint8_t*)&page_desc_val)<=0)
        {
            FS_LOG("read error!\r\n");
            fs_unlock();
            return FS_RET_UNKNOW_ERR;
        }
        
        if(PAGE_SPACE_BLOCK!=page_desc_val)
            break;
    }
    
    //ϵͳδ���ò���ȷ���ָ�Ĭ��
    if(i<SPACE_DESC_PAGE_NUM)
    {
fs_format:
        if(format_to_jacefs())
        {
            FS_LOG("format fs error!\r\n");
            fs_unlock();
            return FS_RET_UNKNOW_ERR;
        }
        FS_LOG("format fs success!\r\n");
		goto init_finish;
    }
	
    /* ����ļ��������Ƿ�������ȷ */
	jacefs_fd_hdr_t hd;
	jacefs_fd_t fd;
	uint16_t crc16;
	uint32_t page,next_page;
	
	page=0;
	next_page=FILE_DESC_START_PAGE-SPACE_DESC_START_PAGE;
	while(1)
	{
		page=find_file_desc_page(next_page);
		
		if(page==0)
			break;
		next_page=page+1;
        
        if(fs_port_read( PAGE_TO_ADDR(page),
            sizeof(jacefs_fd_hdr_t),(uint8_t*)&hd)!=sizeof(jacefs_fd_hdr_t))
        {
            FS_LOG("read err!\r\n");
            fs_unlock();
            return FS_RET_UNKNOW_ERR;
        }
        FS_LOG("page=%d,file_num=%d crc=%x\r\n",page,hd.file_num,hd.crc16);
        
        
        //ÿҳ�洢���ļ������ں���Χ��
        if(hd.file_num>FILE_DESC_PER_PAGE)
        {
            FS_LOG("file num=%d err,too large! \r\n",hd.file_num);
            goto fs_format;
        }
            
        crc16=crc16_compute((uint8_t*)&hd.file_num,sizeof(jacefs_fd_hdr_t)-2,0);
        for(i=0;i<hd.file_num;i++)
        {
            if(fs_port_read( PAGE_TO_ADDR(page)+sizeof(jacefs_fd_hdr_t)+sizeof(jacefs_fd_t)*i,
                sizeof(jacefs_fd_t),(uint8_t*)&fd)!=sizeof(jacefs_fd_t))
            {
                FS_LOG("read err!\r\n");
                fs_unlock();
                return FS_RET_UNKNOW_ERR;
            }
            
            crc16=crc16_compute((uint8_t*)&fd,sizeof(jacefs_fd_t),&crc16);
        }
        
        if(hd.crc16!=crc16)
        {
            FS_LOG("file desc crc16=%x err!\r\n",crc16);
            goto fs_format;//TODO��Σ�գ��ᵼ���ļ�ȫ����ʧ��
        }
        FS_LOG("page=%d file desc crc16 pass!\r\n",page);
	}
    
init_finish:

    //���ʹ��RAM���棬flash����ͬ����RAM
#if FS_USE_RAM_CACHE
    if(fs_port_read( PAGE_TO_ADDR(SPACE_DESC_START_PAGE),
                sizeof(m_info_cache),m_info_cache)!=sizeof(m_info_cache))
    {
        FS_LOG("read err!\r\n");
        fs_unlock();
        return FS_RET_UNKNOW_ERR;
    }
    if(fs_sync_timer_init()!=FS_RET_SUCCESS)
    {
        fs_unlock();
        return FS_RET_UNKNOW_ERR;
    }
#endif

    //����ϵͳʣ��ռ�
    ret=calculate_remaining_size();
    if(ret!=FS_RET_SUCCESS)
    {
        FS_LOG("calc size err!\r\n");
        fs_unlock();
        return ret;
    }
    
    FS_LOG("fs is ready!\r\n");
    m_fs_ready=true;

    fs_unlock();
    return FS_RET_SUCCESS;
}

/**
@brief : �ļ�����
		
@param : ��

@retval:
- @jacefs_error_t
*/
jacefs_error_t jacefs_create(jacefs_file_id_t *file_id,int size,uint16_t app_id)
{
    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }
    
    if(size>m_fs_remainig_size)
    {
        FS_LOG("no enough space!\r\n");
        return FS_RET_NO_ENOUGH_SPACE;
    }
    
    if(!file_id || size<=0 || *file_id==FS_INVALID_FS_ID)
    {
        return FS_RET_PARAM_ERR;
    }
	
    FS_LOG("try to create file_id=%d,app_id=%d,size=%d !\r\n",*file_id,app_id,size);
	
    fs_lock();

    //���ϵͳ�Ƿ��Ѵ��ڸ��ļ�
	if(file_exist(*file_id,app_id))
	{
	    fs_unlock();
		return FS_RET_FILE_EXIST;
	}
	
	//�����ļ�������ռ�
#if !FS_USE_RAM_CACHE
    jacefs_fd_hdr_t fd_hdr;
    jacefs_fd_t fd;
#else
    jacefs_fd_hdr_t *fd_hdr;
    jacefs_fd_t *fd;
#endif

    uint32_t page,next_page;
	jacefs_error_t ret;
    
	next_page=FILE_DESC_START_PAGE-SPACE_DESC_START_PAGE;
	ret=FS_RET_SUCCESS;
	
	while(1)
	{
		page=find_file_desc_page(next_page);
		
		if(page==0)
			break;
		next_page=page+1;
		
#if !FS_USE_RAM_CACHE
		if(fs_port_read( PAGE_TO_ADDR(page),
			sizeof(jacefs_fd_hdr_t),(uint8_t*)&fd_hdr)!=sizeof(jacefs_fd_hdr_t))
		{
			FS_LOG("read err!\r\n");
			fs_unlock();
			return FS_RET_UNKNOW_ERR;
		}
		FS_LOG("file_num=%d crc=%x\r\n",fd_hdr.file_num,fd_hdr.crc16);
		
		if(fd_hdr.file_num>=FILE_DESC_PER_PAGE )
		{
			FS_LOG("page=%d,file num=%d ,overflow !\r\n",page,fd_hdr.file_num);
			ret= FS_RET_NO_ENOUGH_FILE;
			continue;
		}
		
		//���ļ����� ���Ƶ����������ڴ棩
		if(fs_port_read( PAGE_TO_ADDR(page),
			sizeof(jacefs_fd_hdr_t)+fd_hdr.file_num*sizeof(jacefs_fd_t),m_swap_buf) 
				!=sizeof(jacefs_fd_hdr_t)+fd_hdr.file_num*sizeof(jacefs_fd_t) )
		{
			FS_LOG("read err!\r\n");
			fs_unlock();
			return FS_RET_UNKNOW_ERR;
		}
		
		//�µ��ļ�
		memset(&fd,0,sizeof(fd));
		fd.app_id=app_id;
		fd.id=*file_id;          //TODO����APP�����ļ�����������
		fd.size=size;
		fd.wsize=0;
		if(alloc_page(size,&fd.start_page)!=FS_RET_SUCCESS)
		{
			FS_LOG("alloc page error!\r\n");
			fs_unlock();
            return FS_RET_UNKNOW_ERR;
		}
		memcpy(&m_swap_buf[sizeof(jacefs_fd_hdr_t)+fd_hdr.file_num*sizeof(jacefs_fd_t)],&fd,sizeof(fd));
		
		fd_hdr.file_num++;
		memcpy(m_swap_buf,&fd_hdr,sizeof(jacefs_fd_hdr_t));
		
		fd_hdr.crc16=crc16_compute((uint8_t*)&fd_hdr.file_num,sizeof(jacefs_fd_hdr_t)-2,0);
		fd_hdr.crc16=crc16_compute(&m_swap_buf[sizeof(jacefs_fd_hdr_t)],
						fd_hdr.file_num*sizeof(jacefs_fd_t),&fd_hdr.crc16);
		
		//���ļ������ӽ��������ڴ棩��д
		if(fs_port_control(FS_CTL_ERASE_PAGE,&page)!=FS_RET_SUCCESS)
		{
		    fs_unlock();
			return FS_RET_UNKNOW_ERR;
		}
		
		if(fs_port_write( PAGE_TO_ADDR(page),
			sizeof(jacefs_fd_hdr_t)+fd_hdr.file_num*sizeof(jacefs_fd_t),m_swap_buf)
			!=sizeof(jacefs_fd_hdr_t)+fd_hdr.file_num*sizeof(jacefs_fd_t))
		{
			FS_LOG("write err!\r\n");
			fs_unlock();
			return FS_RET_UNKNOW_ERR;
		}
#else
        fd_hdr=(jacefs_fd_hdr_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)];
        FS_LOG("file_num=%d crc=%x\r\n",fd_hdr->file_num,fd_hdr->crc16);

        if(fd_hdr->file_num>=FILE_DESC_PER_PAGE )
        {
            FS_LOG("page=%d,file num=%d ,overflow !\r\n",page,fd_hdr->file_num);
            ret= FS_RET_NO_ENOUGH_FILE;
            continue;
        }

        fd=(jacefs_fd_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)
                                             +sizeof(jacefs_fd_hdr_t)+sizeof(jacefs_fd_t)*fd_hdr->file_num];

        //�µ��ļ�
        memset(fd,0,sizeof(jacefs_fd_t));
        fd->app_id=app_id;
        fd->id=*file_id;          //TODO����APP�����ļ�����������
        fd->size=size;
        fd->wsize=0;
        if(alloc_page(size,&fd->start_page)!=FS_RET_SUCCESS)
        {
            FS_LOG("alloc page error!\r\n");
            fs_unlock();
            return FS_RET_UNKNOW_ERR;
        }

        fd_hdr->file_num++;

        fd_hdr->crc16=crc16_compute((uint8_t*)&fd_hdr->file_num,
            sizeof(jacefs_fd_hdr_t)-2+fd_hdr->file_num*sizeof(jacefs_fd_t),0);

        //���ʹ��RAM���棬RAM����ͬ����flash
        m_fs_sync|=FS_SYNC_ALL;

#endif

		break;
	}
    
	fs_unlock();
    return ret;
}


/**
@brief : ɾ���ļ�
		
@param : 
-file_id 
-app_id 

@retval:
- @jacefs_error_t
*/
jacefs_error_t jacefs_delete(jacefs_file_id_t file_id,uint16_t app_id)
{
	if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }
    
	FS_LOG("delete file=%d,app_id=%d !\r\n",file_id,app_id);
	
	jacefs_fd_t fd;
	
	fs_lock();

    //ɾ���ļ� ����
	if(delete_file_desc(file_id,app_id,&fd,false,FS_INVALID_FS_ID)!=FS_RET_SUCCESS)
	{
	    fs_unlock();
		return FS_RET_FILE_NOT_EXIST;
	}
	
	//�ͷſռ�
	free_page(fd.start_page);
	
	fs_unlock();
    return FS_RET_SUCCESS;
}

/**
@brief : ɾ���ļ���ͬһ��APP ID���ļ���ɾ��������APPж��ʱʹ��
		
@param : 
-app_id         Ҫɾ�����ļ���ӦAPP ID
-except_file_id Ҫ�������ļ���ɾ����APP�������ļ���������ļ���

@retval:
- @jacefs_error_t
*/
jacefs_error_t jacefs_delete_all(uint16_t app_id,jacefs_file_id_t except_file_id)
{
	if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }
    
	FS_LOG("delete app_id=%d !\r\n",app_id);
	
	jacefs_fd_t fd;
	
	fs_lock();

	do{
		//ɾ���ļ� ����
		if(delete_file_desc(0,app_id,&fd,true,except_file_id)!=FS_RET_SUCCESS)
		{
			break;
		}
		
		//�ͷſռ�
		free_page(fd.start_page);
	}while(1);
	
	fs_unlock();
    return FS_RET_SUCCESS;
}

/**
@brief : ���¸�ʽ���ļ�ϵͳ

@param :
-app_id

@retval:
- @jacefs_error_t
*/
jacefs_error_t jacefs_clean_all(void)
{
    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }

    FS_LOG("delete all files !\r\n");

    jacefs_error_t ret;
    fs_lock();

    if(format_to_jacefs())
    {
        FS_LOG("format fs error!\r\n");
        fs_unlock();
        return FS_RET_UNKNOW_ERR;
    }
    FS_LOG("format fs success!\r\n");

    //���ʹ��RAM���棬flash����ͬ����RAM
#if FS_USE_RAM_CACHE
    m_fs_sync=FS_SYNC_NONE;
    if(fs_port_read( PAGE_TO_ADDR(SPACE_DESC_START_PAGE),
                sizeof(m_info_cache),m_info_cache)!=sizeof(m_info_cache))
    {
        FS_LOG("read err!\r\n");
        fs_unlock();
        return FS_RET_UNKNOW_ERR;
    }
#endif

    //����ϵͳʣ��ռ�
    ret=calculate_remaining_size();
    if(ret!=FS_RET_SUCCESS)
    {
        FS_LOG("calc size err!\r\n");
        fs_unlock();
        return ret;
    }

    FS_LOG("fs is ready!\r\n");
//    m_fs_ready=true;

    fs_unlock();
    return FS_RET_SUCCESS;
}

/**
@brief : �ļ�׷������
		
@param : 
-file_id
-app_id 
-dat 		����
-size		�ֽ�

@retval:
- <0��@jacefs_error_t
- >0��д�����ݴ�С���ֽ�
*/
int jacefs_append(jacefs_file_id_t file_id,uint16_t app_id,uint8_t *dat,int size)
{
    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }
	
    if(!dat || size<=0)
    {
        return FS_RET_PARAM_ERR;
    }
	
	jacefs_fd_t fd;

	fs_lock();

	if(get_file_desc(file_id,app_id,&fd)!=FS_RET_SUCCESS)
	{
	    fs_unlock();
		return FS_RET_FILE_NOT_EXIST;
	}
	
	FS_LOG("before write id=%d,app_id=%d,start_page=%d,wsize=%d,size=%d\r\n",
		fd.id,fd.app_id,fd.start_page,fd.wsize,fd.size);
	
	if(fd.wsize+size > fd.size)
	{
		FS_LOG("over size=%d\r\n",fd.wsize+size);
		fs_unlock();
		return FS_RET_FILE_OVER_SIZE;
	}
	
	//�ļ�д��
	uint32_t off_page,
			 off_bytes_in_page;
	uint32_t addr;
	int remaining_size,wsize,already_write;
	
	remaining_size=size;
	already_write=0;
	
	//�ҵ�����д������ݿ�ʼҳ
	off_page=fd.start_page;
    if(fd.wsize>=FS_PAGE_SIZE)
    {
        //�����ļ��洢�����������Ա��ζ�ȡ����ҳ�ô��ļ���ҳ˳�����
        for(int i=0;i<fd.wsize/FS_PAGE_SIZE;i++)
            off_page=get_file_next_page(off_page);
    }
    off_bytes_in_page=fd.wsize%FS_PAGE_SIZE;

    FS_LOG("write page=%d\r\n",off_page);

	do{
		wsize=FS_PAGE_SIZE-off_bytes_in_page;
		if(wsize>=remaining_size)
		{
			wsize=remaining_size;
		}
		
		if(off_bytes_in_page==0)
		{
			if(fs_port_control(FS_CTL_ERASE_PAGE,&off_page)!=FS_RET_SUCCESS)
			{
				FS_LOG("erase page=%d error!\r\n",off_page);
				fs_unlock();
				return FS_RET_UNKNOW_ERR;
			}
		}
		else
		{
			//TODO: ��� off_bytes_in_page ��Ϊ0��Ӧ�ð����ݶ������ٲ�����������д��
		}
		
		//д������
		addr=PAGE_TO_ADDR(off_page)+off_bytes_in_page;
		if(fs_port_write(addr,wsize,&dat[already_write])!=wsize)
		{
			FS_LOG("write add=%x error!\r\n",addr);
			fs_unlock();
			return FS_RET_UNKNOW_ERR;
		}
		
		remaining_size-=wsize;
		fd.wsize+=wsize;
		already_write+=wsize;
		
		FS_LOG("write size=%d,remain=%d \r\n",wsize,remaining_size);
		
		if(remaining_size>0)
        {
            off_page=get_file_next_page(off_page);
            off_bytes_in_page=fd.wsize%FS_PAGE_SIZE;
            FS_LOG("read page=%d\r\n",off_page);
        }
        else
        {
            break;
        }

	}while(1);
	
	//�����ļ�����
	set_file_desc(fd);
	
	FS_LOG("after write id=%d,app_id=%d,start_page=%d,wsize=%d,size=%d\r\n",
		fd.id,fd.app_id,fd.start_page,fd.wsize,fd.size);
	
	fs_unlock();
    return size;
}

/**
@brief : �ļ�����ƫ��д������
		
@param : 
-file_id
-app_id 
-dat 		����
-size		�ֽ�
-offset		ƫ���ֽ�

@retval:
- <0��@jacefs_error_t
- >0��д�����ݴ�С���ֽ�
*/
int jacefs_write(jacefs_file_id_t file_id,uint16_t app_id,uint8_t *dat,int size,int offset)
{
    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }
    
    return 0;
}  

/**
@brief : �ļ���ȡ
		
@param : 
-file_id
-app_id 
-dat 		����
-size		�ֽ�
-offset		ƫ���ֽ�

@retval:
- <0��@jacefs_error_t
- >0����ȡ���ݴ�С���ֽ�
*/
int jacefs_read(jacefs_file_id_t file_id,uint16_t app_id,uint8_t *dat,int size,int offset)
{
    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }
	
    if(!dat || size<=0)
    {
        return FS_RET_PARAM_ERR;
    }
	
	jacefs_fd_t fd;

	fs_lock();

	if(get_file_desc(file_id,app_id,&fd)!=FS_RET_SUCCESS)
	{
	    fs_unlock();
		return FS_RET_FILE_NOT_EXIST;
	}
	
	FS_LOG("read id=%d,app_id=%d,start_page=%d,wsize=%d,size=%d\r\n",
		fd.id,fd.app_id,fd.start_page,fd.wsize,fd.size);
	
	if(offset >= fd.wsize)
	{
		FS_LOG("offset =%d err !\r\n",offset);
		fs_unlock();
		return FS_RET_FILE_OVER_SIZE;
	}
	
	if(offset+size > fd.wsize)
	{
		size=fd.wsize-offset;
		FS_LOG("read size set to %d !\r\n",size);
	}
	
	/*
	 * �����洢���������ֱ�Ӷ�ȡ
	 */
	//if(fd.is_continues)
	{

	    if(fs_port_read(PAGE_TO_ADDR(fd.start_page)+offset,size,dat)!=size)
        {
            FS_LOG("read add=%x error!\r\n",PAGE_TO_ADDR(fd.start_page)+offset);
            fs_unlock();
            return FS_RET_UNKNOW_ERR;
        }
	    fs_unlock();
        return size;
	}
	
	//�ļ���ȡ
	uint32_t off_page,
			 off_bytes_in_page;
	uint32_t addr;
	int remaining_size,rsize,already_read;
	
	remaining_size=size;
	already_read=0;
	
	//�ҵ����ζ�ȡ�����ݿ�ʼҳ
	off_page=fd.start_page;
    if(offset>=FS_PAGE_SIZE)
    {
        //�����ļ��洢�����������Ա��ζ�ȡ����ҳ�ô��ļ���ҳ˳�����
        for(int i=0;i<offset/FS_PAGE_SIZE;i++)
          off_page=get_file_next_page(off_page);
    }
    off_bytes_in_page=offset%FS_PAGE_SIZE;
    FS_LOG("read page=%d\r\n",off_page);

	do{
		rsize=FS_PAGE_SIZE-off_bytes_in_page;
		if(rsize>=remaining_size)
		{
			rsize=remaining_size;
		}
		
		//��ȡ����
		addr=PAGE_TO_ADDR(off_page)+off_bytes_in_page;
		if(fs_port_read(addr,rsize,&dat[already_read])!=rsize)
		{
			FS_LOG("read add=%x error!\r\n",addr);
			fs_unlock();
			return FS_RET_UNKNOW_ERR;
		}
		
		remaining_size-=rsize;
		offset+=rsize;
		already_read+=rsize;
		
		FS_LOG("read size=%d,remain=%d \r\n",rsize,remaining_size);
		
		if(remaining_size>0)
		{
            off_page=get_file_next_page(off_page);
            off_bytes_in_page=offset%FS_PAGE_SIZE;
            FS_LOG("read page=%d\r\n",off_page);
		}
		else
		{
		    break;
		}
	}while(1);
	
	fs_unlock();
    return size;
}

/**
@brief : Ѱ��ͬһ�ļ�ID��Ӧ��APP ID

@param :
-file_id    Ҫ�ҵ��ļ�ID
-app_id     �ҵ������ص� app_id
-offset     ��ʼѰ�ҵ��ļ�ƫ����������ָ�������ļ��������е������±꣩�������ҵ�ʱ�ļ�ƫ��

@retval:
- @jacefs_error_t

*/
jacefs_error_t jacefs_find_app_id(jacefs_file_id_t file_id,uint16_t *app_id,int *offset)
{

    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }

    if(!app_id || !offset)
    {
        return FS_RET_PARAM_ERR;
    }

#if !FS_USE_RAM_CACHE
    jacefs_fd_hdr_t fd_hdr;
#else
    jacefs_fd_hdr_t *fd_hdr;
#endif
    uint32_t page,next_page;
    jacefs_fd_t *search_fd;
    uint16_t i;

    next_page=FILE_DESC_START_PAGE-SPACE_DESC_START_PAGE;
    page=0;

    fs_lock();

    while(1)
    {
        page=find_file_desc_page(next_page);

        if(page==0)
            break;
        next_page=page+1;

#if !FS_USE_RAM_CACHE

#else
        fd_hdr=(jacefs_fd_hdr_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)];

        if(fd_hdr->file_num==0 || fd_hdr->file_num>FILE_DESC_PER_PAGE)
            continue;

        search_fd=(jacefs_fd_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)+sizeof(jacefs_fd_hdr_t)];

        //TODO������ֻ����ֻ��һ��page�ļ�����ҳ��������������һҳѰ�ҽ�������Ҫ�޸���
        for(i=*offset;i<fd_hdr->file_num;i++ )
        {
            if(file_id==search_fd[i].id)
            {
                *app_id=search_fd[i].app_id;
                *offset=i;

                fs_unlock();
                return FS_RET_SUCCESS;
            }
        }

        *offset=i;
#endif
    }

    fs_unlock();
    return FS_RET_FILE_NOT_EXIST;
}

/**
@brief : �鿴�ļ��Ƿ����

@param :
-file_id    Ҫ�ҵ��ļ�ID
-app_id     Ҫ�ҵ��ļ�app_id

@retval:
- ���� FS_RET_FILE_EXIST
- ���� ������

*/
jacefs_error_t jacefs_file_exist(jacefs_file_id_t file_id,uint16_t app_id)
{

    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }

    fs_lock();
    if(file_exist(file_id,app_id))
    {
        fs_unlock();
        return FS_RET_FILE_EXIST;
    }
    fs_unlock();

    return FS_RET_FILE_NOT_EXIST;
}

/**
@brief : ��ȡ�ļ���Ӧ����ʵ�����ַ

                         �ļ��洢 ����������£�������ȷ�������ַ��������������·��� 0xffffffff

@param :
-file_id    Ҫ�ҵ��ļ�ID
-app_id     Ҫ�ҵ��ļ�app_id
-offset     �ļ���ƫ��
-phy_addr   ���ص������ַ

@retval:
- �ɹ�  FS_RET_SUCCESS
- ���� ���ɹ�

*/
jacefs_error_t jacefs_file_get_phy_addr(jacefs_file_id_t file_id,uint16_t app_id,int offset,uint32_t *phy_addr)
{
    if(m_fs_ready!=true)
    {
        return FS_RET_NOT_READY;
    }

    jacefs_fd_t fd;

    fs_lock();

    if(get_file_desc(file_id,app_id,&fd)!=FS_RET_SUCCESS)
    {
        fs_unlock();
        return FS_RET_FILE_NOT_EXIST;
    }

    FS_LOG("found phy_addr id=%d,app_id=%d,start_page=%d,wsize=%d,size=%d\r\n",
        fd.id,fd.app_id,fd.start_page,fd.wsize,fd.size);

    if(offset >= fd.wsize)
    {
        FS_LOG("offset =%d err !\r\n",offset);
        fs_unlock();
        return FS_RET_FILE_OVER_SIZE;
    }

    //if(fd.is_continues)
    {
        *phy_addr=PAGE_TO_ADDR(fd.start_page)+offset;

        FS_LOG("phy_addr =%d(%08x)\r\n",*phy_addr);

        fs_unlock();
        return FS_RET_SUCCESS;
    }

    *phy_addr=0xffffffff;
    return FS_RET_UNKNOW_ERR;
}

/*----------------------------------------------------------------------------------*/

//��ӡ�����ļ�
void print_all_file_desc(void)
{
#if USE_OS_DEBUG

#if !FS_USE_RAM_CACHE
    jacefs_fd_hdr_t fd_hdr;
#else
    jacefs_fd_hdr_t *fd_hdr;
#endif
    uint32_t page,next_page;
    jacefs_fd_t *search_fd;

    next_page=FILE_DESC_START_PAGE-SPACE_DESC_START_PAGE;
    page=0;

    while(1)
    {
        page=find_file_desc_page(next_page);

        if(page==0)
            break;
        next_page=page+1;

#if !FS_USE_RAM_CACHE

#else
        fd_hdr=(jacefs_fd_hdr_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)];

        if(fd_hdr->file_num==0 || fd_hdr->file_num>FILE_DESC_PER_PAGE)
            continue;

        search_fd=(jacefs_fd_t *)&m_file_desc_cache[PAGE_TO_FILE_DESC_ADDR(page)+sizeof(jacefs_fd_hdr_t)];

        for(uint16_t i=0;i<fd_hdr->file_num;i++ )
        {
            OS_LOG("page=%d,f[%d] fid=%d,app_id=%d,sp=%d,size=%d,ws=%d\r\n",
                page,i,
                search_fd[i].id,search_fd[i].app_id,search_fd[i].start_page,
                search_fd[i].size,search_fd[i].wsize);
        }
#endif
    }
#endif

}

//#include "ext_flash.h"

//�Բ�
void jacefs_self_test(void)
{
#if 1
    os_printk("\r\n");
    os_printk("SPACE_DESC_START_PAGE=%d\r\n",SPACE_DESC_START_PAGE);
    os_printk("FS_TOTAL_PAGE=%d\r\n",FS_TOTAL_PAGE);
    os_printk("FS_TOTAL_SIZE=%d (%dKB)\r\n",FS_TOTAL_SIZE,FS_TOTAL_SIZE/1024);
    os_printk("FS_PAGE_SIZE=%d\r\n\r\n",FS_PAGE_SIZE);
    
    os_printk("FILE_DESC_PER_PAGE=%d\r\n",FILE_DESC_PER_PAGE);
    os_printk("SPACE_BYTE_PER_PAGE=%d\r\n",SPACE_BYTE_PER_PAGE);
    os_printk("SPACE_DESC_PAGE_NUM=%d\r\n",SPACE_DESC_PAGE_NUM);
    os_printk("DESC_NUM_PER_PAGE=%d,need=%d\r\n",DESC_NUM_PER_PAGE,FS_TOTAL_PAGE+(DESC_NUM_PER_PAGE-1));
    os_printk("FS_MANAGE_PAGE_MAX=%d\r\n",FS_MANAGE_PAGE_MAX);
    os_printk("FS_MANAGE_PAGE_MIN=%d\r\n\r\n",FS_MANAGE_PAGE_MIN);

    os_printk("jacefs_fd_t size=%d\r\n",sizeof(jacefs_fd_t));
    os_printk("\r\n");
#endif


//���Զ�
#define __printf_all_()\
{\
    jacefs_sync();\
	page_desc_val_t rbuf[20];\
	for(i=0;i<FS_MANAGE_PAGE_MIN+10/*FS_TOTAL_PAGE*/;i++)\
	{\
		addr=PAGE_TO_ADDR(i);\
		os_printk("addr=%x ",addr);\
		fs_port_read(addr,sizeof(rbuf),(uint8_t*)rbuf);\
		for(j=0;j<sizeof(rbuf)/sizeof(page_desc_val_t);j++)\
		{\
			os_printk("%04x ",rbuf[j]);\
		}\
		os_printk("\r\n");\
	}\
}\
os_printk("\r\n");	
	
#if 0
	int i,j;
    uint32_t addr;

	//�ٳ�ʼ��
	jacefs_init();
    __printf_all_();
#endif

    //��ӡ�����ļ�
    print_all_file_desc();
#if 0
    {
        int offset=0;
        uint16_t app_id,file_id=0;
        do{
            if(jacefs_find_app_id(file_id,&app_id,&offset)!=FS_RET_SUCCESS)
            {
                break;
            }
            OS_LOG("app_id=%d,offset=%d\r\n",app_id,offset);
            offset++;
        }while(1);

        app_id=123;
        file_id=5;
        jacefs_create(&file_id,155,app_id);
    }
#endif


//    ext_flash_self_test();

#if 0

	//�����ļ� 1
	jacefs_file_id_t f_id;
	uint16_t app_id;
	
//
//���Դ��������
#if 0
	app_id=1;
	f_id=1;
	jacefs_create(&f_id,155,app_id);
	__printf_all_();
	
	//�����ļ�2
	f_id=2;
	jacefs_create(&f_id,596+1024,app_id);
	__printf_all_();
	
	//�����ļ�4
	f_id=4;
	jacefs_create(&f_id,596+1024+1024+1024,app_id);
	__printf_all_();
	
	//�����ļ�5
	f_id=5;
	jacefs_create(&f_id,20,app_id);
	__printf_all_();
	
	//�����ļ�6
	f_id=6;
	jacefs_create(&f_id,256,app_id);
	__printf_all_();
	
	//ɾ���ļ�1
	f_id=1;
	jacefs_delete(f_id,app_id);
	__printf_all_();
	
	//�����ļ�3
	f_id=3;
	jacefs_create(&f_id,596+1024+1024,app_id);
	__printf_all_();
	
	//ɾ���ļ�3
	f_id=3;
	jacefs_delete(f_id,app_id);
	__printf_all_();
	
	//ɾ���ļ�2
	f_id=2;
	jacefs_delete(f_id,app_id);
	__printf_all_();
	
	//ɾ���ļ�4
	f_id=4;
	jacefs_delete(f_id,app_id);
	__printf_all_();
	
	return;
#endif

//
//����ɾ��
#if 0
	app_id=2;
	f_id=1;
	jacefs_create(&f_id,155,app_id);
	__printf_all_();
	
	//�����ļ�2
	f_id=2;
	jacefs_create(&f_id,596,app_id);
	__printf_all_();
	
	//�����ļ�5
	f_id=5;
	jacefs_create(&f_id,20,app_id);
	__printf_all_();
	
	//�����ļ�6
	app_id=1;
	f_id=6;
	jacefs_create(&f_id,256,app_id);
	__printf_all_();
	
	//ɾ��
	jacefs_delete_by_appid(2);
	__printf_all_();
	
	jacefs_delete(6,1);
	__printf_all_();
	
	return;
#endif

//
//���Զ�д
#if 0
	app_id=3;

	//�����ļ�1
	f_id=1;
	jacefs_create(&f_id,155,app_id);
	__printf_all_();
	
	//�����ļ�2
	f_id=2;
	jacefs_create(&f_id,196,app_id);
	__printf_all_();
	
	//ɾ���ļ�1
	f_id=1;
	jacefs_delete(f_id,app_id);
	__printf_all_();
	
	//�����ļ�1
	f_id=1;
	jacefs_create(&f_id,155+1024,app_id);
	__printf_all_();
	
	f_id=1;
	
	//�ļ�1д
	uint8_t wdat[90]={1,2,3,4,5,6,7,8,9,10};
	for(i=0;i<sizeof(wdat);i++)
		wdat[i]=i;
	
	jacefs_append(f_id,app_id,wdat,sizeof(wdat));
	__printf_all_();
	
	jacefs_append(f_id,app_id,wdat,sizeof(wdat));
	__printf_all_();
	
	//�ļ�1��
	uint8_t rdat[180];
	jacefs_read(f_id,app_id,rdat,sizeof(rdat),0);
	for(i=0;i<sizeof(rdat);i+=2)
	{
		if(i%40==0 && i)
			os_printk("\r\n");
		uint16_t da=*(uint16_t*)&rdat[i];
		os_printk("%04x ",da);
		
	}
	os_printk("\r\n");
	
	return;
#endif

	//���Զ��ٶ� (���²�����96M��Ƶʱ����)
#if 1

//	uint32_t t1=0,t2=0;
	uint32_t tick1,tick2;

	int len;

	app_id=4; //�ֿ�
	f_id=0;

#define BUF_SIZE (100*1024)//(240*240*2)
//	static uint8_t rdat[BUF_SIZE];

	extern uint16_t *gui_get_back_fb() ;
	uint8_t *rdat=(uint8_t*)gui_get_back_fb();

//	GLOBAL_INT_DISABLE();

	OS_LOG("---->FS read\r\n");
	{
        {
            jacefs_create(&f_id,200*1024,app_id);
            jacefs_append(f_id,app_id,rdat,BUF_SIZE);
        }

        //FS ������ȡ100KB ����ʱ11ms--�Ż�(��RAM����)����ʱ17ms
        tick1=os_get_tick();
        len=jacefs_read(f_id,app_id,rdat,BUF_SIZE,0);
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //FS ��������ȡ 100KB ��ÿ�ζ�ȡ1KB����ʱ113ms --�Ż�(��RAM����)����ʱ19ms
        len=0;
        tick1=os_get_tick();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            len+=jacefs_read(f_id,app_id,rdat,1024,i);
        }
        tick2=os_get_tick();
        OS_LOG("read 1kB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //FS ��������ȡ100KB��ÿ�ζ�128B����ʱ650ms --�Ż�2(��RAM����)����ʱ33ms
        len=0;
        tick1=os_get_tick();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            len+=jacefs_read(f_id,app_id,rdat,128,i);
        }
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
	}

#if 0
    OS_LOG("---->NVMS read\r\n");
    {
        //������ȡ100KB����ʱ17ms
        tick1=os_get_tick();
        nvms_t part;
        part=ad_nvms_open(NVMS_BIN_PART);
        if (!part) {
            OS_LOG("open err!");
        }
        len=ad_nvms_read(part,0,rdat,BUF_SIZE);
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ18ms
        len=0;
        tick1=os_get_tick();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            len+=ad_nvms_read(part,i,rdat,1024);
        }
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�128B����ʱ26ms
        len=0;
        tick1=os_get_tick();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            len+=ad_nvms_read(part,i,rdat,128);
        }
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
    }
#endif

    OS_LOG("---->ad_flash read\r\n");
    {
        //������ȡ100KB����ʱ17ms
        tick1=os_get_tick();
        len=ad_flash_read(NVMS_BIN_PART_START+0,rdat,BUF_SIZE);
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ18ms
        len=0;
        tick1=os_get_tick();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            len+=ad_flash_read(NVMS_BIN_PART_START+i,rdat,1024);
        }
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�128B����ʱ22ms
        len=0;
        tick1=os_get_tick();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            len+=ad_flash_read(NVMS_BIN_PART_START+i,rdat,128);
        }
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
    }

    OS_LOG("---->MEMORY_QSPIF_S_BASE memcpy read\r\n");
    {
        const void *read_addr=(const void *) (MEMORY_QSPIF_S_BASE );

        //������ȡ100KB����ʱ17ms
        tick1=os_get_tick();

        len=BUF_SIZE;
        ad_flash_lock();
        memcpy(rdat,read_addr,BUF_SIZE);
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ17ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            len+=1024;
            memcpy(rdat,read_addr,1024);
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�128B����ʱ17ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            len+=128;
            memcpy(rdat,read_addr,128);
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
    }

    OS_LOG("---->MEMORY_QSPIF_BASE memcpy read\r\n");
    {
        const void *read_addr=(const void *) (MEMORY_QSPIF_BASE );

        //������ȡ100KB����ʱ15ms
        tick1=os_get_tick();

        len=BUF_SIZE;
        ad_flash_lock();
        memcpy(rdat,read_addr,BUF_SIZE);
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ11ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            len+=1024;
            memcpy(rdat,read_addr,1024);
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�128B����ʱ11ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            len+=128;
            memcpy(rdat,read_addr,128);
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
    }

    OS_LOG("---->MEMORY_QSPIF_S_BASE direct read\r\n"); //���ڲ��Գ������ַ����ٶ���죬���Զ�ȡflashʱ����SDK��nvms����ֱ�Ӷ�ȡ
    {
        const uint8_t *read_addr=(const uint8_t *) (MEMORY_QSPIF_S_BASE );

        //������ȡ100KB����ʱ17ms
        tick1=os_get_tick();
        ad_flash_lock();
        len=BUF_SIZE;
        for(int i=0;i<BUF_SIZE;i++)
        {
            rdat[i]=read_addr[i];
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ9ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            for(int i=0;i<1024;i++)
            {
                rdat[len]=read_addr[len];
            }
            len+=1024;
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�128B����ʱ8ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            for(int i=0;i<128;i++)
            {
                rdat[len]=read_addr[len];
            }
            len+=128;
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
    }

    OS_LOG("---->MEMORY_QSPIF_BASE direct read\r\n");
    {
        const uint8_t *read_addr=(const uint8_t *) (MEMORY_QSPIF_BASE );

        //������ȡ100KB����ʱ14ms
        tick1=os_get_tick();
        ad_flash_lock();
        len=BUF_SIZE;
        for(int i=0;i<BUF_SIZE;i++)
        {
            rdat[i]=read_addr[i];
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ10ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            for(int i=0;i<1024;i++)
            {
                rdat[len]=read_addr[len];
            }
            len+=1024;
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�128B����ʱ10ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            for(int i=0;i<128;i++)
            {
                rdat[len]=read_addr[len];
            }
            len+=128;

        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
    }

    OS_LOG("---->MEMORY_QSPIF_S_BASE direct read --4bytes\r\n"); //���ڲ��Գ������ַ����ٶ���죬���Զ�ȡflashʱ����SDK��nvms����ֱ�Ӷ�ȡ
    {
        const uint32_t *read_addr=(const uint32_t *) (MEMORY_QSPIF_S_BASE );
        uint32_t *read_dat=(uint32_t*)rdat;

        //������ȡ100KB����ʱ8ms
        tick1=os_get_tick();
        ad_flash_lock();
        len=BUF_SIZE;
        for(int i=0;i<BUF_SIZE/4;i++)
        {
            read_dat[i]=read_addr[i];
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ3ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            for(int i=0;i<1024/4;i++)
            {
                read_dat[len/4]=read_addr[len/4];
            }
            len+=1024;
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�128B����ʱ3ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            for(int i=0;i<128/4;i++)
            {
                read_dat[len/4]=read_addr[len/4];
            }
            len+=128;
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
    }

    OS_LOG("---->MEMORY_QSPIF_BASE direct read --4bytes\r\n");
    {
        const uint32_t *read_addr=(const uint32_t *) (MEMORY_QSPIF_BASE );
        uint32_t *read_dat=(uint32_t*)rdat;

        //������ȡ100KB����ʱ7ms
        tick1=os_get_tick();
        ad_flash_lock();
        len=BUF_SIZE;
        for(int i=0;i<BUF_SIZE/4;i++)
        {
            read_dat[i]=read_addr[i];
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ3ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            for(int i=0;i<1024/4;i++)
            {
                read_dat[len/4]=read_addr[len/4];
            }
            len+=1024;
        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));

        //������ȡ100KB��ÿ�ζ�128B����ʱ4ms
        len=0;
        tick1=os_get_tick();
        ad_flash_lock();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            for(int i=0;i<128/4;i++)
            {
                read_dat[len/4]=read_addr[len/4];
            }
            len+=128;

        }
        ad_flash_unlock();
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
    }

//    GLOBAL_INT_RESTORE();

    //DMA ��ȡflash ��ע�����ﲻ�ܹ��жϣ�����DMAû���жϵ��²���ʧ��
    {
        OS_LOG("---->MEMORY_QSPIF_BASE DMA read\r\n");

        #define BUF_SIZE (100*1024)

        extern uint16_t *gui_get_back_fb() ;
        uint8_t *rdat=(uint8_t*)gui_get_back_fb();

        uint32_t tick1,tick2;
        int len;

        const uint8_t *read_addr=(const uint8_t *) (MEMORY_QSPIF_S_BASE );

        //������ȡ100KB����ʱ14ms
        tick1=os_get_tick();

        len=BUF_SIZE;
        while(dma_start((uint32_t)(read_addr),(uint32_t)rdat,BUF_SIZE))
        {
            OS_LOG("DMA start err\r\n");
        }

        tick2=os_get_tick();
        OS_LOG("read 100KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
        {
            int i;
            for(i=0;i<BUF_SIZE;i++)
            {
                if(rdat[i]!=read_addr[i])
                {
                    OS_LOG("dma read data err!,i=%d\r\n",i);
                    break;
                }
            }
            if(i==BUF_SIZE)
                OS_LOG("dma read data success!\r\n");
        }


        //������ȡ100KB��ÿ�ζ�ȡ1KB����ʱ10ms
        len=0;
        tick1=os_get_tick();
        for(int i=0;i<BUF_SIZE;i+=1024)
        {
            while(dma_start((uint32_t)(&read_addr[len]),(uint32_t)&rdat[len],1024))
            {
                OS_LOG("DMA start err\r\n");
            }
            len+=1024;
        }
        tick2=os_get_tick();
        OS_LOG("read 1KB, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
        {
            int i;
            for(i=0;i<BUF_SIZE;i++)
            {
                if(rdat[i]!=read_addr[i])
                {
                    OS_LOG("dma read data err!,i=%d\r\n",i);
                    break;
                }
            }
            if(i==BUF_SIZE)
                OS_LOG("dma read data success!\r\n");
        }

        //������ȡ100KB��ÿ�ζ�128B����ʱ10ms
        len=0;
        tick1=os_get_tick();
        for(int i=0;i<BUF_SIZE;i+=128)
        {
            while(dma_start((uint32_t)(&read_addr[len]),(uint32_t)&rdat[len],128))
            {
                OS_LOG("DMA start err\r\n");
            }
            len+=128;

        }
        tick2=os_get_tick();
        OS_LOG("read 128B, len=%d,time=%d ms\r\n",len,os_get_tick_diff_to_ms(tick1,tick2));
        {
            int i;
            for(i=0;i<BUF_SIZE;i++)
            {
                if(rdat[i]!=read_addr[i])
                {
                    OS_LOG("dma read data err!,i=%d\r\n",i);
                    break;
                }
            }
            if(i==BUF_SIZE)
                OS_LOG("dma read data success!\r\n");
        }

        OS_DELAY_MS(1000);
    }



#if 0
    {//����������д (PSRAM)

        OS_LOG("\r\n---------------\r\nSPI PSRAM TEST\r\n");

        //����QSPI2 -- PSRAM
        uint32_t psram_addr=MEMORY_QSPIF_S_BASE;
        uint8_t rdat[4*100];
        memcpy(rdat,(void*)psram_addr,sizeof(rdat));
        OS_INFO("before:");

        for(int i=0;i<sizeof(rdat);i++)
        {
            OS_INFO("%02x ",rdat[i]);
        }
        OS_INFO("\r\n");


//        uint8_t wdat[4*100];
//        for(int i=0;i<sizeof(wdat);i++)
//        {
//            wdat[i]=i;
//        }
//        memcpy((void*)psram_addr,wdat,sizeof(wdat));

        uint8_t *psram_p = (uint8_t *)(MEMORY_QSPIR_BASE);
        *psram_p=1;
        *(psram_p+1)=2;
        *(psram_p+2)=3;
        *(psram_p+3)=4;

        hw_qspi_write8(HW_QSPIC2,1);
        hw_qspi_write8(HW_QSPIC2,2);
        hw_qspi_write8(HW_QSPIC2,3);
        hw_qspi_write8(HW_QSPIC2,4);


        memcpy(rdat,(void*)psram_addr,sizeof(rdat));
        OS_INFO("after:");
        for(int i=0;i<sizeof(rdat);i++)
        {
            OS_INFO("%02x ",rdat[i]);
        }
        OS_INFO("\r\n");
    }
#endif

#endif

#endif
}
