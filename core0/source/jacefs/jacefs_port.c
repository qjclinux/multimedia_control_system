/*
******************************************************************************
- file    		fs_port.c 
- author  		jace
- version 		V1.0
- create date	2018.11.11
- brief 
	fs �ļ�ϵͳӲ�������ӿ�ʵ�֡�
    
- release note:

2018.11.11 V1.0
        ��������

2019.7.4
    ʹ�� NVMS_GENERIC_PART ������Ϊ�ļ�ϵͳ�洢��

2019.12.22
    ��ֲ��LPC55s69ƽ̨��


******************************************************************************
*/
#include "stdbool.h"
#include "string.h"
#include "jacefs.h"
#include "jacefs_port.h"

#define DEBUG_FS_PORT 0

#if DEBUG_FS_PORT
#define FS_PORT_LOG(fmt,args...)	do{\
								/*os_printk("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
                                os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
								os_printk(fmt,##args);\
							}while(0)
#define FS_PORT_INFO(fmt,args...)	do{\
								os_printk(fmt,##args);\
							}while(0)								
#else
#define FS_PORT_LOG(fmt,args...)
#define FS_PORT_INFO(fmt,args...)
#endif

#if FS_USE_RAM
uint8_t m_fs_ram[FS_HW_TOTAL_SIZE];
#else
//static nvms_t m_fs_part ;
#endif
                            
static uint8_t m_fs_port_ready=false;

                            
/**
@brief : �ļ�ϵͳ��Ӳ����ʼ��������flash�ĳ�ʼ���ӿ�
		
@param : ��

@retval:
- FS_RET_SUCCESS	��ʼ���ɹ� 
- FS_UNKNOW_ERR		��ʼ��ʧ��
*/
int fs_port_init(void)
{
    if(m_fs_port_ready)
        return FS_RET_SUCCESS;
    
#if FS_USE_RAM
    memset((void*)FS_HW_START_ADDR,0x55,FS_HW_TOTAL_SIZE);
#else
//    m_fs_part=ad_nvms_open(NVMS_BIN_PART);
//    if (!m_fs_part) {
//        FS_PORT_LOG("open err!");
//        return FS_RET_UNKNOW_ERR;
//    }

#endif
    
    m_fs_port_ready=true;
    return FS_RET_SUCCESS;
}

/**
@brief : �ļ�ϵͳ��Ӳ���ر�
		
@param : ��
 
@retval:
- FS_RET_SUCCESS      �����ɹ� 
- FS_UNKNOW_ERR   ����ʧ��
*/
int fs_port_deinit(void)
{
    if(m_fs_port_ready==false)
        return FS_RET_SUCCESS;
    
#if FS_USE_RAM

#else
    
#endif
	
	m_fs_port_ready=false;
	
    return FS_RET_SUCCESS;
}

/**
@brief : �ļ�ϵͳ��ȡӲ������
		
@param : ��
- addr ��ȡ��flash��ַ
- size ��ȡ��С���ֽ�
- rbuf �������ݵ��ڴ�

@retval:
- >=0 ��ʾ��ȡ�ɹ������ض�ȡ����ʵ�ʴ�С
- <0  ��ʾ��ȡʧ�ܣ��������ֵ
*/
int fs_port_read(uint32_t addr,int size,uint8_t *rbuf)
{
    //�������β�����Ϊ�˼ӿ�flash���ٶȣ����з��յģ�
    /*if(m_fs_port_ready==false)
        return FS_RET_NOT_READY;
    
    if(addr<FS_HW_START_ADDR || addr>FS_HW_END_ADDR)
    {
        FS_PORT_LOG("addr=%x err!\r\n",addr);
        return FS_RET_PARAM_ERR;
    }
	
    if(size<=0 || size>FS_HW_TOTAL_SIZE)
    {
        FS_PORT_LOG("size=%x err!\r\n",size);
        return FS_RET_PARAM_ERR;
    }
    
    if(!rbuf)
    {
        FS_PORT_LOG("buf point err !\r\n");
        return FS_RET_PARAM_ERR;
    }*/
    
	//TODO: �ж� size+addr < FS_HW_TOTAL_SIZE
	
#if FS_USE_RAM
    memcpy(rbuf,(uint8_t*)addr,size);
#else
//    size=ext_flash_read(addr,rbuf,size);
    memcpy(rbuf,(uint8_t*)addr,size);
#endif
    return size;
}

/**
@brief : �ļ�ϵͳд��Ӳ������
		
@param : ��
- addr д���flash��ַ
- size д���С���ֽ�
- rbuf д�����ݵ��ڴ�

@retval:
- >=0 ��ʾд��ɹ�������д���ʵ�ʴ�С
- <0  ��ʾд��ʧ�ܣ��������ֵ
*/
int fs_port_write(uint32_t addr,int size,uint8_t *wbuf)
{
    if(m_fs_port_ready==false)
        return FS_RET_NOT_READY;
    
    if(addr<FS_HW_START_ADDR || addr>FS_HW_END_ADDR)
    {
        FS_PORT_LOG("addr=%x err!\r\n",addr);
        return FS_RET_PARAM_ERR;
    }
	
    if(size<=0 || size>FS_HW_TOTAL_SIZE)
    {
        FS_PORT_LOG("size=%x err!\r\n",size);
        return FS_RET_PARAM_ERR;
    }
    
    if(!wbuf)
    {
        FS_PORT_LOG("buf point err !\r\n");
        return FS_RET_PARAM_ERR;
    }
    
	//TODO: �ж� size+addr < FS_HW_TOTAL_SIZE
	
#if FS_USE_RAM
    memcpy((uint8_t*)addr,wbuf,size);
#else
//    size=ext_flash_write(addr,wbuf,size);
    //����fs�õ��ڲ�flash��ֻ֧�ֶ���Դ����֧��д��
#endif
    
    return size;
}

/**
@brief : �ļ�ϵͳ��Ӳ�����ƣ���������� ctl ��������
		
@param : 
- ctl   �������ͣ��ο� fs_port_ctl_t
- param ��������

@retval:
- FS_RET_SUCCESS      �����ɹ� 
- FS_UNKNOW_ERR   ����ʧ��


note: 
	ctl == FS_CTL_ERASE_PAGE, param �ǲ���ҳ


*/
int fs_port_control(fs_port_ctl_t ctl,void *param)
{
    if(m_fs_port_ready==false)
        return FS_RET_NOT_READY;
    
    switch(ctl)
    {
        case FS_CTL_ERASE_PAGE:
        {
            uint16_t page=*(uint16_t*)param;
            uint32_t addr=page*FS_HW_PAGE_SIZE+FS_HW_START_ADDR;
            
            if(addr<FS_HW_START_ADDR || addr>FS_HW_END_ADDR )
            {
                FS_PORT_LOG("erase addr=%x page=%d err!\r\n",addr,page);
                return FS_RET_PARAM_ERR;
            }
            
            FS_PORT_LOG("erase addr=%x page=%d success!\r\n",addr,page);
#if FS_USE_RAM
            memset((void*)addr,0xff,FS_HW_PAGE_SIZE);
#else
//            ext_flash_erase_region(addr,FS_HW_PAGE_SIZE);
            //����fs�õ��ڲ�flash��ֻ֧�ֶ���Դ����֧��д��
#endif
            
            break;
        }
        default:
            break;
            
    }
    
    return FS_RET_SUCCESS;
}


void fs_port_self_test(void )
{
    int i,j;
    uint32_t addr;

    FS_PORT_LOG("\r\naddr={%x ,%x}\r\n",FS_HW_START_ADDR,FS_HW_END_ADDR);

    fs_port_init();
   
    //������������
#if 1
	//���� ioctl
    {
		FS_PORT_LOG("--------------------------\r\n");
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
			fs_port_control(FS_CTL_ERASE_PAGE,&i);
        }
    }
	
    //���Զ�
    {
		FS_PORT_LOG("--------------------------\r\n");
        uint8_t rbuf[10];
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE;
            FS_PORT_INFO("addr=%x ",addr);
            fs_port_read(addr,sizeof(rbuf),rbuf);
            for(j=0;j<sizeof(rbuf);j++)
            {
                FS_PORT_INFO("%d ",rbuf[j]);
            }
            FS_PORT_INFO("\r\n");
        }
    }
    
    
    //����д
    {
        uint8_t wbuf[10]={1,2,3,4,5,6,7,8,9,10};
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE;
            memset(wbuf,i,sizeof(wbuf));
            fs_port_write(addr,sizeof(wbuf),wbuf);
        }
    }
    
    //���Զ�
    {
		FS_PORT_LOG("--------------------------\r\n");
        uint8_t rbuf[10];
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE;
            FS_PORT_INFO("addr=%x ",addr);
            fs_port_read(addr,sizeof(rbuf),rbuf);
            for(j=0;j<sizeof(rbuf);j++)
            {
                FS_PORT_INFO("%d ",rbuf[j]);
            }
            FS_PORT_INFO("\r\n");
        }
    }
    
    //���� ioctl
    {
		FS_PORT_LOG("--------------------------\r\n");
        uint32_t page;
        page=0;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        page=2;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        page=4;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        page=5;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        
        uint8_t rbuf[10];
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE;
            FS_PORT_INFO("addr=%x ",addr);
            fs_port_read(addr,sizeof(rbuf),rbuf);
            for(j=0;j<sizeof(rbuf);j++)
            {
                FS_PORT_INFO("%d ",rbuf[j]);
            }
            FS_PORT_INFO("\r\n");
        }
    }
    
	
#endif
	
	//���Ե�����д
#if 0
	//���� ioctl
    {
		FS_PORT_LOG("--------------------------\r\n");
        uint32_t page;
        page=0;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        page=1;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        page=2;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        page=3;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        page=4;
        fs_port_control(FS_CTL_ERASE_PAGE,&page);
        
        uint8_t rbuf[10];
        for(i=0;i<5;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE;
            FS_PORT_INFO("addr=%x ",addr);
            fs_port_read(addr,sizeof(rbuf),rbuf);
            for(j=0;j<sizeof(rbuf);j++)
            {
                FS_PORT_INFO("%d ",rbuf[j]);
            }
            FS_PORT_INFO("\r\n");
        }
    }
	
	//����д
    {
        static uint16_t wbuf[10]={0xfff1,0xfff2,0xfff3,0xfff4,5,6,7,8,9,10};
		i=0;
        for(i=0;i<5;i++)
		{
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE;
			
            fs_port_write(addr,6,(uint8_t*)wbuf);
            fs_port_write(addr+6,2,(uint8_t*)wbuf);
			fs_port_write(addr+8,2,(uint8_t*)wbuf);
		}
        
    }
    
    //���Զ�
    {
		FS_PORT_LOG("--------------------------\r\n");
        uint8_t rbuf[10];
        for(i=0;i<5;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE;
            FS_PORT_INFO("addr=%x ",addr);
            fs_port_read(addr,sizeof(rbuf),rbuf);
            for(j=0;j<sizeof(rbuf);j++)
            {
                FS_PORT_INFO("%x ",rbuf[j]);
            }
            FS_PORT_INFO("\r\n");
        }
    }




#endif

    //��ҳд��
#if 0
    //���� ioctl
    {
        FS_PORT_LOG("--------------------------\r\n");
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            fs_port_control(FS_CTL_ERASE_PAGE,&i);
        }
    }

    //����д
    {
        uint8_t wbuf[10]={1,2,3,4,5,6,7,8,9,10};
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE+250;
            fs_port_write(addr,sizeof(wbuf),wbuf);
        }
    }

    //���Զ�
    {
        FS_PORT_LOG("--------------------------\r\n");
        uint8_t rbuf[10];
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE+250;
            FS_PORT_INFO("addr=%x ",addr);
            fs_port_read(addr,sizeof(rbuf),rbuf);
            for(j=0;j<sizeof(rbuf);j++)
            {
                FS_PORT_INFO("%d ",rbuf[j]);
            }
            FS_PORT_INFO("\r\n");
        }
    }
#endif

}


