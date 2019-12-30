/*
******************************************************************************
- file    		fs_port.c 
- author  		jace
- version 		V1.0
- create date	2018.11.11
- brief 
	fs 文件系统硬件操作接口实现。
    
- release note:

2018.11.11 V1.0
        创建工程

2019.7.4
    使用 NVMS_GENERIC_PART 分区作为文件系统存储区

2019.12.22
    移植到LPC55s69平台上


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
@brief : 文件系统的硬件初始化，调用flash的初始化接口
		
@param : 无

@retval:
- FS_RET_SUCCESS	初始化成功 
- FS_UNKNOW_ERR		初始化失败
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
@brief : 文件系统的硬件关闭
		
@param : 无
 
@retval:
- FS_RET_SUCCESS      操作成功 
- FS_UNKNOW_ERR   操作失败
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
@brief : 文件系统读取硬件数据
		
@param : 无
- addr 读取的flash地址
- size 读取大小，字节
- rbuf 保存数据的内存

@retval:
- >=0 表示读取成功，返回读取到的实际大小
- <0  表示读取失败，具体错误看值
*/
int fs_port_read(uint32_t addr,int size,uint8_t *rbuf)
{
    //这里屏蔽参数是为了加快flash读速度，是有风险的！
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
    
	//TODO: 判断 size+addr < FS_HW_TOTAL_SIZE
	
#if FS_USE_RAM
    memcpy(rbuf,(uint8_t*)addr,size);
#else
//    size=ext_flash_read(addr,rbuf,size);
    memcpy(rbuf,(uint8_t*)addr,size);
#endif
    return size;
}

/**
@brief : 文件系统写入硬件数据
		
@param : 无
- addr 写入的flash地址
- size 写入大小，字节
- rbuf 写入数据的内存

@retval:
- >=0 表示写入成功，返回写入的实际大小
- <0  表示写入失败，具体错误看值
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
    
	//TODO: 判断 size+addr < FS_HW_TOTAL_SIZE
	
#if FS_USE_RAM
    memcpy((uint8_t*)addr,wbuf,size);
#else
//    size=ext_flash_write(addr,wbuf,size);
    //现在fs用的内部flash，只支持读资源，不支持写入
#endif
    
    return size;
}

/**
@brief : 文件系统的硬件控制，具体操作由 ctl 参数决定
		
@param : 
- ctl   操作类型，参考 fs_port_ctl_t
- param 操作数据

@retval:
- FS_RET_SUCCESS      操作成功 
- FS_UNKNOW_ERR   操作失败


note: 
	ctl == FS_CTL_ERASE_PAGE, param 是擦除页


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
            //现在fs用的内部flash，只支持读资源，不支持写入
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
   
    //测试正常操作
#if 1
	//测试 ioctl
    {
		FS_PORT_LOG("--------------------------\r\n");
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
			fs_port_control(FS_CTL_ERASE_PAGE,&i);
        }
    }
	
    //测试读
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
    
    
    //测试写
    {
        uint8_t wbuf[10]={1,2,3,4,5,6,7,8,9,10};
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE;
            memset(wbuf,i,sizeof(wbuf));
            fs_port_write(addr,sizeof(wbuf),wbuf);
        }
    }
    
    //测试读
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
    
    //测试 ioctl
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
	
	//测试单个读写
#if 0
	//测试 ioctl
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
	
	//测试写
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
    
    //测试读
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

    //跨页写读
#if 0
    //测试 ioctl
    {
        FS_PORT_LOG("--------------------------\r\n");
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            fs_port_control(FS_CTL_ERASE_PAGE,&i);
        }
    }

    //测试写
    {
        uint8_t wbuf[10]={1,2,3,4,5,6,7,8,9,10};
        for(i=0;i<10/*FS_HW_PAGE_NUM*/;i++)
        {
            addr=FS_HW_START_ADDR+i*FS_HW_PAGE_SIZE+250;
            fs_port_write(addr,sizeof(wbuf),wbuf);
        }
    }

    //测试读
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



