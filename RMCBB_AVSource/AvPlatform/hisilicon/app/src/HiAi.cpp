#include "HiAi.h"
#include "mpi_ai.h"
#include "mpi_sys.h"
#include "AvDebug.h"
#include "AvDeviceInfo.h"

#include "HiAo.h"

#if defined(_AV_PRODUCT_CLASS_IPC_)
#include "hi_comm_aio.h"
#endif

const int AI_MAX_CHN_NUM = 16;

CHiAi::CHiAi()
{
#if defined(_AV_PRODUCT_CLASS_IPC_)
    m_u8InputVolume = 7;
#endif
}

CHiAi::~CHiAi()
{
}

int CHiAi::HiAi_get_fd(ai_type_e ai_type, int phy_chn) const
{
    int ai_fd = -1;
    AUDIO_DEV ai_dev = 0;
    AI_CHN ai_chn = 0;
    
    HiAi_get_ai_info(ai_type, phy_chn, &ai_dev, &ai_chn);
    
    if ((ai_fd = HI_MPI_AI_GetFd(ai_dev, ai_chn)) < 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_get_fd HI_MPI_AI_GetFd FAILED (ai_fd:%d) (ai_dev:%d) (ai_chn:%d)\n", ai_fd, ai_dev, ai_chn);
        return -1;
    }

    return ai_fd;
}

int CHiAi::HiAi_get_ai_info(ai_type_e ai_type, int phy_chn, AUDIO_DEV *pAi_dev, AI_CHN *pAi_chn/*=NULL*/, AIO_ATTR_S *pAi_attr/*=NULL*/) const
{
    if (phy_chn > pgAvDeviceInfo->Adi_max_channel_number())
    {
        DEBUG_ERROR( "CHiAi::HiAi_get_ai_info (phy_chn:%d) error! \n", phy_chn);
        return -1;
    }
#ifndef _AV_PRODUCT_CLASS_IPC_
    eProductType product_type = PTD_INVALID;
    product_type = pgAvDeviceInfo->Adi_product_type();
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name)); 

    char platform_name[32] = {0};
    pgAvDeviceInfo->Adi_get_platform_name(platform_name, sizeof(platform_name));
#endif
		
    if (pAi_dev != NULL || pAi_chn != NULL)
    {		
        switch (ai_type)
        {
            case _AI_NORMAL_:
            {
 #ifdef _AV_PLATFORM_HI3531_
                if( phy_chn < 8 )
                {
                    _AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
                    if( phy_chn < 4 )
                    {
                        _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn);
                    }
                    else
                    {
                        _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn+4);
                    }
                }
                else
                {
                    _AV_POINTER_ASSIGNMENT_(pAi_dev, 1);
                    if( phy_chn < 12 )
                    {
                        _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn-8);
                    }
                    else
                    {
                        _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn-8+4);
                    }
                }
 #elif defined(_AV_PLATFORM_HI3520D_V300_)	
        if(product_type == PTD_D5_35)
    	{
                 if( phy_chn < 4 )
                {
                    _AV_POINTER_ASSIGNMENT_(pAi_dev, 1);
                    _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn);
                }
                else
                {
                    _AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
                    _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn-4);
                }    
    	}
        else if(product_type == PTD_6A_II_AVX)
		{
               _AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
               _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn);
               if(phy_chn == 1)	//6AII_AV12机型需求第8通道音频绑定到第2通道，但硬件上将第8路音频绑定到了第1通道，所以此处将第1路音频绑定到第2通道///
               {
               		_AV_POINTER_ASSIGNMENT_(pAi_chn, 0);
               }
        }
		else if ((product_type == PTD_A5_AHD)||(product_type == PTD_M3))
		{
				_AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
				_AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn);
		}
		else if(product_type == PTD_ES8412)
		{
				_AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
				_AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn);
		}
		else if(product_type == PTD_M0026)
		{
				_AV_POINTER_ASSIGNMENT_(pAi_dev, 1);
				_AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn);
		}
		else if(product_type == PTD_P1)
		{
			//I2S0有4个通道，依次对应P1的MIC1, MIC2, MIC3, AUDIO IN3, 然后依次对应虚拟通道号为5,6,7,4///
			//I2S1有2个通道，依次对应P1的AUDIO IN1, AUDIO IN2, 然后依次对应虚拟通道号为2, 3///
			//此处将MIC通道放在纯音频通道后面///
			if(phy_chn == 2 || phy_chn == 3)
			{
				_AV_POINTER_ASSIGNMENT_(pAi_dev, 1);
				if(phy_chn == 2)
				{
					_AV_POINTER_ASSIGNMENT_(pAi_chn, 0);	 
				}
				else
				{
					_AV_POINTER_ASSIGNMENT_(pAi_chn, 1);
				}
			}
			else if(phy_chn == 4 || phy_chn == 5 || phy_chn == 6 || phy_chn == 7)
			{
				_AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
				if(phy_chn == 4)
				{
					_AV_POINTER_ASSIGNMENT_(pAi_chn, 3);
				}
				else if(phy_chn == 5)
				{
					_AV_POINTER_ASSIGNMENT_(pAi_chn, 1);
				}
				else if(phy_chn == 6)
				{
					_AV_POINTER_ASSIGNMENT_(pAi_chn, 0);
				}
				else if(phy_chn == 7)
				{
					_AV_POINTER_ASSIGNMENT_(pAi_chn, 2);
				}
			}
		}
		else
		{
               _AV_POINTER_ASSIGNMENT_(pAi_dev, 1);
               _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn);	
		}
 #else
                _AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAi_chn, phy_chn);
 #endif
                break;
            }
            case _AI_TALKBACK_:
            {
#ifdef _AV_PLATFORM_HI3521_
                _AV_POINTER_ASSIGNMENT_(pAi_dev, 2);
                _AV_POINTER_ASSIGNMENT_(pAi_chn, 0);
#elif defined(_AV_PLATFORM_HI3520D_)
                _AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAi_chn, 8);
#elif defined(_AV_PLATFORM_HI3520D_V300_)
		if((product_type == PTD_6A_II_AVX)||(product_type == PTD_ES8412))
		{
	               _AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
	               _AV_POINTER_ASSIGNMENT_(pAi_chn, 8);	
		}
		else if((product_type == PTD_A5_AHD)||(product_type == PTD_M3))
		{
			_AV_POINTER_ASSIGNMENT_(pAi_dev, 1);
            _AV_POINTER_ASSIGNMENT_(pAi_chn, 0);
		}
		else if(product_type == PTD_M0026)
		{
			_AV_POINTER_ASSIGNMENT_(pAi_dev, 1);
            _AV_POINTER_ASSIGNMENT_(pAi_chn, 4);
		}
		else
		{
			 _AV_POINTER_ASSIGNMENT_(pAi_dev, 1);
	                _AV_POINTER_ASSIGNMENT_(pAi_chn, 8);				
		}
#elif defined(_AV_PRODUCT_CLASS_IPC_)
                _AV_POINTER_ASSIGNMENT_(pAi_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAi_chn, 0);
#else
                _AV_POINTER_ASSIGNMENT_(pAi_dev, 4);
                _AV_POINTER_ASSIGNMENT_(pAi_chn, 0);
#endif
                break;
            }
            default:
                DEBUG_ERROR( "CHiAi::HiAi_get_ai_info invalid ai_type(%d) \n", ai_type);
                return -1;
            break;
        }     
    }

    if (pAi_attr != NULL)
    {
        pAi_attr->enSamplerate = AUDIO_SAMPLE_RATE_8000;
        pAi_attr->enBitwidth = AUDIO_BIT_WIDTH_16;
        pAi_attr->enSoundmode = AUDIO_SOUND_MODE_MONO;
        pAi_attr->u32EXFlag = 0;
        pAi_attr->u32FrmNum = 30;
        pAi_attr->u32ClkSel = 1;
        pAi_attr->u32PtNumPerFrm = 320;

#if defined(_AV_PLATFORM_HI3518C_)
        pAi_attr->enWorkmode = AIO_MODE_I2S_MASTER;
#elif defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
        /*inner codec the u32ClkSel must be setted to 0*/
        pAi_attr->enWorkmode = AIO_MODE_I2S_MASTER;
        pAi_attr->u32ClkSel = 1;
#elif defined(_AV_PLATFORM_HI3535_)
        pAi_attr->enWorkmode = AIO_MODE_I2S_MASTER;
        pAi_attr->u32EXFlag = 1;
        pAi_attr->u32ClkSel = 0;
#else
        pAi_attr->enWorkmode = AIO_MODE_I2S_SLAVE;
#endif

		if(PTD_P1 == pgAvDeviceInfo->Adi_product_type()) //飞机项目支持44.1K, 主模式///
		{
			pAi_attr->enSamplerate = AUDIO_SAMPLE_RATE_44100;
			pAi_attr->u32PtNumPerFrm = 2048;
			pAi_attr->enWorkmode = AIO_MODE_I2S_MASTER;
		}
#ifndef _AV_PRODUCT_CLASS_IPC_
        if ( (!strcmp(customer_name, "CUSTOMER_04.1062"))&&(_AI_TALKBACK_ == ai_type))
        {
            pAi_attr->u32PtNumPerFrm = 320;//160;
        }
        if (!strcmp(platform_name, "HONGXIN"))
        {
            pAi_attr->u32PtNumPerFrm = 160;
        }
#endif
        int audio_chn_num = 0;
        switch (ai_type)
        {
            case _AI_NORMAL_:
#if defined(_AV_PLATFORM_HI3520D_)||defined(_AV_PLATFORM_HI3520D_V300_)
			if ((product_type == PTD_A5_AHD)||(product_type == PTD_M0026)||\
                (product_type == PTD_ES4206)||(product_type == PTD_ES8412)||\
                (product_type == PTD_M3)||(product_type == PTD_XMD3104)) 
			{
				audio_chn_num = 8;
			}
			else
			{
            	audio_chn_num = 16;
			}
            if(PTD_P1 == pgAvDeviceInfo->Adi_product_type())
            {
            	if(pAi_dev != NULL && *pAi_dev == 0)	//I2S0有四个输入///
            	{
            		audio_chn_num = 4;
            	}
            	else if(pAi_dev != NULL && *pAi_dev == 1)
            	{
            		audio_chn_num = 2;	//I2S1有2个输入///
            	}
            }
#else
                if(pgAvDeviceInfo->Adi_get_audio_chn_number() <= 2)
                {
                    audio_chn_num = 2;
                }
                else if(pgAvDeviceInfo->Adi_get_audio_chn_number() <= 4)
                {
                    audio_chn_num = 4;
                }
                else if(pgAvDeviceInfo->Adi_get_audio_chn_number() <= 8)
                {
                    audio_chn_num = 8;
                }
                else
                {
                    audio_chn_num = 16;
                }
#endif
                break;
            case _AI_TALKBACK_:
                audio_chn_num = 2;
#if defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3531_)
                pAi_attr->enWorkmode = AIO_MODE_I2S_MASTER;
#endif
#if defined(_AV_PLATFORM_HI3520D_)||defined(_AV_PLATFORM_HI3520D_V300_)
                audio_chn_num = 16;
				if(product_type == PTD_A5_AHD || product_type == PTD_M0026)
				{
					audio_chn_num = 2;
				}
#endif
                break;
            default:
                DEBUG_ERROR( "CHiAi::HiAi_get_ai_info invalid ai_type (%d)!\n", ai_type);
                return -1;
        }
        pAi_attr->u32ChnCnt = audio_chn_num + (audio_chn_num % 2);
#ifdef _AV_PLATFORM_HI3520D_V300_
		pAi_attr->u32ClkChnCnt = audio_chn_num + (audio_chn_num % 2);
        pAi_attr->u32ClkSel = 0;
		if(PTD_P1 == pgAvDeviceInfo->Adi_product_type()) //飞机项目支持44.1K, 主模式///
		{
			pAi_attr->u32ClkSel = 1;
			if(pAi_dev != NULL && *pAi_dev == 0)
			{
				pAi_attr->u32ClkSel = 0;
			}
		}
        if  ((_AI_TALKBACK_ == ai_type)&&(PTD_A5_AHD == product_type))
        {
            pAi_attr->u32ClkSel = 1;  //!20160507
		}

#endif
    }

    return 0;
}

int CHiAi::HiAi_start_ai(ai_type_e ai_type)
{
#ifdef _AV_PLATFORM_HI3531_
    if( ai_type == _AI_NORMAL_ )
    {
        this->HiAi_start_preview_ai_2chip();
    }
#endif
    AUDIO_DEV ai_dev[2] = {0,0};
    AIO_ATTR_S ai_attr;
    eProductType product_type = PTD_INVALID;
    product_type = pgAvDeviceInfo->Adi_product_type();
    product_type = product_type;
	
    if (HiAi_get_ai_info(ai_type, 0, &ai_dev[0], NULL, &ai_attr) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_get_ai_info (ai_type:%d) FAILED!\n", ai_type);
        return -1;
    }
  
    //0717 主要处理与报站设置AI AO属性冲突的问题，设置属性前清除其属性
#ifdef _AV_PLATFORM_HI3521_   
    if(HI_MPI_AI_ClrPubAttr(ai_dev[0]) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_start_ai HI_MPI_AI_ClrPubAttr (ai_dev:%d) FAILED!\n", ai_dev[0]);
        return -1;
    }
#endif       
    if (HiAi_memory_balance(ai_dev[0]) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_memory_balance (ai_dev:%d) FAILED!\n", ai_dev[0]);
        return -1;
    }

#if defined(_AV_PLATFORM_HI3518C_) && defined(_AV_PRODUCT_CLASS_IPC_)
     this->HiAi_config_acodec(ai_attr.enSamplerate);
#endif

#ifdef _AV_PLATFORM_HI3520D_V300_
    printf("\nai_dev(%d),ai_attr:(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",ai_dev[0],ai_attr.enSamplerate,ai_attr.enBitwidth,ai_attr.enWorkmode,ai_attr.enSoundmode,
    		ai_attr.u32EXFlag,ai_attr.u32FrmNum,ai_attr.u32PtNumPerFrm,ai_attr.u32ChnCnt,ai_attr.u32ClkChnCnt,ai_attr.u32ClkSel);
//ai_attr:(8000,1,0,0,1,30,320,16,16,0)
#endif

    if (HiAi_start_ai_dev(ai_dev[0], &ai_attr) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_start_ai_dev (ai_dev:%d) FAILED! \n", ai_dev[0]);
        return -1;
    }
	
    for (unsigned int chn = 0; chn < ai_attr.u32ChnCnt; ++chn)
    {
        if (HiAi_start_ai_chn(ai_dev[0], chn) != 0)
        {
            DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_start_ai_chn (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev[0], chn);
            return -1;
        }
#if 0//def _AV_PLATFORM_HI3520D_V300_
        if (ai_type == _AI_NORMAL_ && HiAi_set_ai_chn_param(ai_dev[0], chn, 30) != 0)
#else
        if (ai_type == _AI_NORMAL_ && HiAi_set_ai_chn_param(ai_dev[0], chn, 5) != 0)
#endif
        {
            DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_set_ai_chn_param (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev[0], chn);
            return -1;
        }
#if defined(_AV_TEST_MAINBOARD_) && defined(_AV_PRODUCT_CLASS_NVR_)
        if (pgAvDeviceInfo->Adi_app_type() == APP_TYPE_TEST)
        {   
            if (ai_type == _AI_TALKBACK_ && HiAi_set_ai_chn_param(ai_dev[0], chn, 30) != 0)
            {
                DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_set_ai_chn_param (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev[0], chn);
                return -1;
            }
        }
#endif
    }

    if(product_type == PTD_D5_35)
    {
    	    ai_attr.u32ClkSel = 0;
	    if (HiAi_start_ai_dev(ai_dev[1], &ai_attr) != 0)
	    {
	        DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_start_ai_dev (ai_dev:%d) FAILED! \n", ai_dev[1]);
	        return -1;
	    }
		
	    for (unsigned int chn = 0; chn < ai_attr.u32ChnCnt; ++chn)
	    {
	        if (HiAi_start_ai_chn(ai_dev[1], chn) != 0)
	        {
	            DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_start_ai_chn (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev[1], chn);
	            return -1;
	        }
	        if (ai_type == _AI_NORMAL_ && HiAi_set_ai_chn_param(ai_dev[1], chn, 30) != 0)
	        {
	            DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_set_ai_chn_param (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev[1], chn);
	            return -1;
	        }
#if defined(_AV_TEST_MAINBOARD_) && defined(_AV_PRODUCT_CLASS_NVR_)
	        if (pgAvDeviceInfo->Adi_app_type() == APP_TYPE_TEST)
	        {   
	            if (ai_type == _AI_TALKBACK_ && HiAi_set_ai_chn_param(ai_dev[1], chn, 30) != 0)
	            {
	                DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_set_ai_chn_param (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev[1], chn);
	                return -1;
	            }
	        }
#endif
	    }
	}    
	if(product_type == PTD_P1)	//设置P1机型的I2S0///
	{
		if (HiAi_get_ai_info(ai_type, 2, &ai_dev[1], NULL, &ai_attr) != 0)
		{
			DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_get_ai_info (ai_type:%d) FAILED!\n", ai_type);
			return -1;
		}
		if (HiAi_start_ai_dev(ai_dev[1], &ai_attr) != 0)
		{
			DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_start_ai_dev (ai_dev:%d) FAILED! \n", ai_dev[1]);
			return -1;
		}
		
		for (unsigned int chn = 0; chn < ai_attr.u32ChnCnt; ++chn)
		{
			if (HiAi_start_ai_chn(ai_dev[1], chn) != 0)
			{
				DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_start_ai_chn (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev[1], chn);
				return -1;
			}
			if (ai_type == _AI_NORMAL_ && HiAi_set_ai_chn_param(ai_dev[1], chn, 5) != 0)
			{
				DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_set_ai_chn_param (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev[1], chn);
				return -1;
			}
		}
	}
    return 0;
}

int CHiAi::HiAi_stop_ai(ai_type_e ai_type)
{
#ifdef _AV_PLATFORM_HI3531_
    if( ai_type == _AI_NORMAL_ )
    {
        this->HiAi_stop_preview_ai_2chip();
    }
#endif
    
    AUDIO_DEV ai_dev = 0;
    AIO_ATTR_S ai_attr;

    HiAi_get_ai_info(ai_type, 0, &ai_dev, NULL, &ai_attr);
    
    for (unsigned int chn = 0; chn < ai_attr.u32ChnCnt; ++chn)
    {
        if (HiAi_stop_ai_chn(ai_dev, chn) != 0)
        {
            DEBUG_ERROR( "CHiAi::HiAi_stop_ai HiAi_stop_ai_chn (ai_dev:%d) (chn:%d) FAILED!\n", ai_dev, chn);
            return -1;
        }
    }

    if (HiAi_stop_ai_dev(ai_dev) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_stop_ai HiAi_stop_ai_dev (ai_dev:%d) FAILED!\n", ai_dev);
        return -1;
    }
    //0717
#ifdef _AV_PLATFORM_HI3521_ 
    if(HI_MPI_AI_ClrPubAttr(ai_dev) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_stop_ai HI_MPI_AI_ClrPubAttr (ai_dev:%d) FAILED!\n", ai_dev);
        return -1;
    }  
#endif
    
    return 0;
}

#ifdef _AV_PLATFORM_HI3531_
int CHiAi::HiAi_start_preview_ai_2chip()
{
    AUDIO_DEV ai_dev = 0;
    
    AIO_ATTR_S ai_attr;
    AI_CHN ai_chn = 0;
    unsigned int u32DevEnableMask = 0;
    for( int i=0; i<pgAvDeviceInfo->Adi_get_vi_chn_num(); i++ )
    {
        if (HiAi_get_ai_info(_AI_NORMAL_, i, &ai_dev, &ai_chn, &ai_attr) != 0)
        {
            DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_get_ai_info  FAILED!\n");
            return -1;
        }
        if( !((u32DevEnableMask>>ai_dev) & 0x01) )
        {
            if (HiAi_memory_balance(ai_dev) != 0)
            {
                DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_memory_balance (ai_dev:%d) FAILED!\n", ai_dev);
            }

            if (HiAi_start_ai_dev(ai_dev, &ai_attr) != 0)
            {
                DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_start_ai_dev (ai_dev:%d) FAILED! \n", ai_dev);
                return -1;
            }
            u32DevEnableMask |= (1<<ai_dev);
        }

        if (HiAi_start_ai_chn(ai_dev, ai_chn) != 0)
        {
            DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_start_ai_chn (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev, ai_chn);
            return -1;
        }
        
        if (HiAi_set_ai_chn_param(ai_dev, ai_chn, 5) != 0)
        {
            DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_set_ai_chn_param (ai_dev:%d) (ai_chn:%d) FAILED! \n", ai_dev, ai_chn);
            return -1;
        }
        //DEBUG_MESSAGE("start ai, channle=%d aidev=%d aichn=%d\n", i, ai_dev, ai_chn);
    }

    return 0;
}

int CHiAi::HiAi_stop_preview_ai_2chip()
{
    AUDIO_DEV ai_dev = 0;    
    AI_CHN ai_chn = 0;
    unsigned int u32DevEnableMask = 0;
    for( int i=0; i<pgAvDeviceInfo->Adi_get_vi_chn_num(); i++ )
    {
        if (HiAi_get_ai_info(_AI_NORMAL_, i, &ai_dev, &ai_chn, NULL) != 0)
        {
            DEBUG_ERROR( "CHiAi::HiAi_start_ai HiAi_get_ai_info  FAILED, phychn=%d\n", i);
            return -1;
        }
        u32DevEnableMask |= (1<<ai_dev);

        if (HiAi_stop_ai_chn(ai_dev, ai_chn) != 0)
        {
            DEBUG_ERROR( "CHiAi::HiAi_stop_ai HiAi_stop_ai_chn (ai_dev:%d) (chn:%d) FAILED!\n", ai_dev, ai_chn);
        }
    }

    for( int i=0; i<4; i++ )
    {
        if( (u32DevEnableMask>>i) & 0x1 )
        {
            if (HiAi_stop_ai_dev(i) != 0)
            {
                DEBUG_ERROR( "CHiAi::HiAi_stop_ai HiAi_stop_ai_dev (ai_dev:%d) FAILED!\n", ai_dev);
            }
        }
    }

    return 0;
}
#endif


int CHiAi::HiAi_start_ai_dev(AUDIO_DEV ai_dev, const AIO_ATTR_S *pAi_attr) const
{
    int ret_val = -1;

    if ((ret_val = HI_MPI_AI_SetPubAttr(ai_dev, pAi_attr)) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_start_ai_dev HI_MPI_AI_SetPubAttr FAILED(ai_dev:%d)(0x%08lx)\n", ai_dev, (unsigned long)ret_val);
        return -1;
    }

    if ((ret_val = HI_MPI_AI_Enable(ai_dev)) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_start_ai_dev HI_MPI_AI_Enable FAILED(ai_dev:%d)(0x%08lx)\n", ai_dev, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}

int CHiAi::HiAi_stop_ai_dev(AUDIO_DEV ai_dev) const
{
    int ret_val = -1;

    if ((ret_val = HI_MPI_AI_Disable(ai_dev)) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_stop_ai_dev HI_MPI_AI_Disable FAILED(ai_dev:%d)(0x%08lx)\n", ai_dev, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}

int CHiAi::HiAi_start_ai_chn(AUDIO_DEV ai_dev, AI_CHN ai_chn) const
{
    int ret_val = -1;
    
    if ((ret_val = HI_MPI_AI_EnableChn(ai_dev, ai_chn)) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_start_ai_chn HI_MPI_AI_EnableChn FAILED(ai_dev:%d)(ai_chn:%d)(0x%08lx)\n", ai_dev, ai_chn, (unsigned long)ret_val);
        return -1;
    }
#if defined(_AV_PRODUCT_CLASS_IPC_)

    AI_VQE_CONFIG_S vqe_Config;
    AUDIO_DEV dev;
    AI_CHN chn;
    AIO_ATTR_S ai_attr;
    
    memset(&ai_attr, 0, sizeof(AIO_ATTR_S));
    HiAi_get_ai_info(_AI_NORMAL_, 0, &dev, &chn, &ai_attr);
    memset(&vqe_Config, 0, sizeof(AI_VQE_CONFIG_S));

#if defined(_AV_PLATFORM_HI3518C_)
    /*open HPF*/
    AI_HPF_ATTR_S  hpf_attr;
    memset(&hpf_attr, 0, sizeof(AI_HPF_ATTR_S));

    /*open HPF*/
    hpf_attr.bHpfOpen = HI_TRUE;
    hpf_attr.stHpfCfg.enHpfFreq = AUDIO_HPF_FREQ_150;

    HiAi_start_hpf(ai_dev, ai_chn, &hpf_attr);
#endif

    /*start vqe*/
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
    vqe_Config.bHpfOpen = HI_TRUE;
    vqe_Config.bAecOpen = HI_TRUE;
    vqe_Config.bAnrOpen = HI_TRUE;
    vqe_Config.bRnrOpen = HI_FALSE;
    vqe_Config.bAgcOpen = HI_TRUE;
    vqe_Config.bEqOpen = HI_TRUE;
    vqe_Config.s32WorkSampleRate = ai_attr.enSamplerate;
    vqe_Config.enWorkstate = VQE_WORKSTATE_COMMON;
#else
    vqe_Config.bAecOpen = HI_FALSE;           //!<
    vqe_Config.bAnrOpen = HI_TRUE;           //!<denoising enable    

    vqe_Config.s32SampleRate = ai_attr.enSamplerate;
    vqe_Config.stAecCfg.enAecMode = AUDIO_AEC_MODE_RECEIVER;  
#endif
    
    vqe_Config.s32FrameSample = ai_attr.u32PtNumPerFrm;
    HiAi_start_vqe(ai_dev, ai_chn, &vqe_Config);
#endif
    return 0;
}

int CHiAi::HiAi_stop_ai_chn(AUDIO_DEV ai_dev, AI_CHN ai_chn) const
{
    int ret_val = -1;
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_PLATFORM_HI3518C_)
        ret_val = HiAi_stop_vqe(ai_dev, ai_chn);
        ret_val |= HiAi_stop_hpf(ai_dev, ai_chn);
#else
        ret_val = HiAi_stop_vqe(ai_dev, ai_chn);    
#endif
        if(0 != ret_val)
        {
            DEBUG_ERROR( "stop hpf or vqe (ai_dev:%d)(ai_chn:%d)(0x%08lx)\n", ai_dev, ai_chn, (unsigned long)ret_val);
            //return -1;
        }
#endif

    if ((ret_val = HI_MPI_AI_DisableChn(ai_dev, ai_chn)) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_start_ai_chn HI_MPI_AI_DisableChn FAILED(ai_dev:%d)(ai_chn:%d)(0x%08lx)\n", ai_dev, ai_chn, (unsigned long)ret_val);
        return -1;
    } 
    return 0;
}

int CHiAi::HiAi_set_ai_chn_param(AUDIO_DEV ai_dev, AI_CHN ai_chn, unsigned int usrFrmDepth) const
{
    int ret_val = -1;

    if (usrFrmDepth > 30)
    {
       DEBUG_ERROR( "CHiAi::HiAi_set_ai_chn_param parameter error!(usrFrmDepth:%d > 30)\n", usrFrmDepth);
       return -1;
    }
    
    AI_CHN_PARAM_S stChnParam;
    if ((ret_val=HI_MPI_AI_GetChnParam(ai_dev, ai_chn, &stChnParam)) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_set_ai_chn_param HI_MPI_AI_GetChnParam FAILED(ai_dev:%d)(ai_chn:%d)(0x%08lx)\n", ai_dev, ai_chn, (unsigned long)ret_val);
        return -1;
    }

    stChnParam.u32UsrFrmDepth = usrFrmDepth;
    if ((ret_val=HI_MPI_AI_SetChnParam(ai_dev, ai_chn, &stChnParam)) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_set_ai_chn_param HI_MPI_AI_SetChnParam FAILED(ai_dev:%d)(ai_chn:%d)(0x%08lx)\n", ai_dev, ai_chn, (unsigned long)ret_val);
        return -1;
    } 
    
    return 0;
}

int CHiAi::HiAi_get_ai_frame(ai_type_e ai_type, int phy_chn, AUDIO_FRAME_S *pAiframe, AEC_FRAME_S *pAecframe/*=NULL*/) const
{
    AUDIO_DEV ai_dev = 0;
    AI_CHN ai_chn = 0;
    int ret_val = -1;

    HiAi_get_ai_info(ai_type, phy_chn, &ai_dev, &ai_chn);
    
    if ((ret_val = HI_MPI_AI_GetFrame(ai_dev, ai_chn, pAiframe, pAecframe, HI_FALSE)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAi::HiAi_get_ai_frame HI_MPI_AI_GetFrame FAILED(ai_dev:%d)(ai_chn:%d)(0x%08lx)\n", ai_dev, ai_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}

int CHiAi::HiAi_release_ai_frame(ai_type_e ai_type, int phy_chn, AUDIO_FRAME_S *pAiframe, AEC_FRAME_S *pAecframe/*=NULL*/) const
{
    AUDIO_DEV ai_dev = 0;
    AI_CHN ai_chn = 0;
    int ret_val = -1;
   
    HiAi_get_ai_info(ai_type, phy_chn, &ai_dev, &ai_chn);
    
    if ((ret_val = HI_MPI_AI_ReleaseFrame(ai_dev, ai_chn, pAiframe, pAecframe)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAi::HiAi_release_ai_frame HI_MPI_AI_ReleaseFrame FAILED(ai_dev:%d)(ai_chn:%d)(0x%08lx)\n", ai_dev, ai_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}


int CHiAi::HiAi_ai_bind_aenc( ai_type_e eAiType, int phy_chn, int vencChn )
{
    AUDIO_DEV ai_dev = 0;
    AI_CHN ai_chn = 0;

    if( this->HiAi_get_ai_info( eAiType, phy_chn, &ai_dev, &ai_chn ) != 0 )
    {
        DEBUG_ERROR( "CHiAi::HiAi_ai_bind_aenc HiAi_get_ai_info FAILED aitype=%d phychn=%d venchn=%d\n", eAiType, phy_chn, vencChn );
        return -1;
    }

    MPP_CHN_S mpp_chn_src;
    MPP_CHN_S mpp_chn_dst;

    mpp_chn_src.enModId = HI_ID_AI;
    mpp_chn_src.s32DevId = ai_dev;
    mpp_chn_src.s32ChnId = ai_chn;
    mpp_chn_dst.enModId = HI_ID_AENC;
    mpp_chn_dst.s32DevId = 0;
    mpp_chn_dst.s32ChnId = vencChn;

    HI_S32 sRet = HI_MPI_SYS_Bind( &mpp_chn_src, &mpp_chn_dst );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_ai_ae_bind_control HI_MPI_SYS_Bind error sRet=0x%08x type:%d phychn:%d vench=%d ai_dev=%d ai_chn=%d\n", sRet, eAiType, phy_chn, vencChn, ai_dev, ai_chn);
        return -1;
    }

    return 0;
}

int CHiAi::HiAi_ai_unbind_aenc( ai_type_e eAiType, int phy_chn, int vencChn )
{
    AUDIO_DEV ai_dev = 0;
    AI_CHN ai_chn = 0;

    if( this->HiAi_get_ai_info( eAiType, phy_chn, &ai_dev, &ai_chn ) != 0 )
    {
        DEBUG_ERROR( "CHiAi::HiAi_ai_bind_aenc HiAi_get_ai_info FAILED aitype=%d phychn=%d venchn=%d\n", eAiType, phy_chn, vencChn );
        return -1;
    }

    MPP_CHN_S mpp_chn_src;
    MPP_CHN_S mpp_chn_dst;

    mpp_chn_src.enModId = HI_ID_AI;
    mpp_chn_src.s32DevId = ai_dev;
    mpp_chn_src.s32ChnId = ai_chn;
    mpp_chn_dst.enModId = HI_ID_AENC;
    mpp_chn_dst.s32DevId = 0;
    mpp_chn_dst.s32ChnId = vencChn;

    HI_S32 sRet = HI_MPI_SYS_UnBind( &mpp_chn_src, &mpp_chn_dst );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_ai_ae_bind_control HI_MPI_SYS_Bind error sRet=0x%08x type:%d phychn:%d vench=%d ai_dev=%d ai_chn=%d\n", sRet, eAiType, phy_chn, vencChn, ai_dev, ai_chn);
        return -1;
    }

    return 0;
}

int CHiAi::HiAi_memory_balance(AUDIO_DEV ai_dev)
{
#ifdef _AV_PLATFORM_HI3531_
    int ret_val = -1;
    MPP_CHN_S mpp_chn;
    const char * pDdrName = NULL;
    
    pDdrName = "ddr1";
    mpp_chn.enModId = HI_ID_AI;
    mpp_chn.s32DevId = ai_dev;
    mpp_chn.s32ChnId = 0;
    
    if ((ret_val = HI_MPI_SYS_SetMemConf(&mpp_chn, pDdrName)) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_memory_balance HI_MPI_SYS_SetMemConf FAILED(ai_dev:%d)(0x%08lx)\n", ai_dev, (unsigned long)ret_val);
        return -1;
    }
#endif

    return 0;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
#include "acodec.h"
#include <fcntl.h>
#include <sys/ioctl.h>
int CHiAi::HiAi_config_acodec( bool bMacIn, AUDIO_SAMPLE_RATE_E enSample, unsigned char u8Volum )
{
    HI_S32 fdAcodec = -1;
    unsigned int i2s_fs_sel = 0;
//    unsigned int mixer_mic_ctrl = 0;
    unsigned int gain_mic = 0;

    fdAcodec = open( "/dev/acodec", O_RDWR );
    if( fdAcodec < 0 )
    {
        DEBUG_ERROR( "CHiAi::HiAi_config_acodec opne /dev/acodec FAILED\n");
        return HI_FAILURE;     
    }
    
    if( ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL) )
    {
        DEBUG_ERROR("Reset audio codec error\n");
        close(fdAcodec);
        return HI_FAILURE;
    }
    usleep(100*1000);

    if ((AUDIO_SAMPLE_RATE_8000 == enSample)
        || (AUDIO_SAMPLE_RATE_11025 == enSample)
        || (AUDIO_SAMPLE_RATE_12000 == enSample))
    {
        i2s_fs_sel = 0x18;
    }
    else if ((AUDIO_SAMPLE_RATE_16000 == enSample)
        || (AUDIO_SAMPLE_RATE_22050 == enSample)
        || (AUDIO_SAMPLE_RATE_24000 == enSample))
    {
        i2s_fs_sel = 0x19;
    }
    else if ((AUDIO_SAMPLE_RATE_32000 == enSample)
        || (AUDIO_SAMPLE_RATE_44100 == enSample)
        || (AUDIO_SAMPLE_RATE_48000 == enSample))
    {
        i2s_fs_sel = 0x1a;
    }
    else 
    {
        DEBUG_ERROR("%s: not support enSample:%d\n", __FUNCTION__, enSample);
        close(fdAcodec);
        return HI_FAILURE;
    }
    
    if( ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel) )
    {
        DEBUG_ERROR("%s: set acodec sample rate failed\n", __FUNCTION__);
        close(fdAcodec);
        return HI_FAILURE;
    }

    if( HI_TRUE == bMacIn )
    {
        /*mixer_mic_ctrl = 1;
        if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &mixer_mic_ctrl))
        {
            DEBUG_ERROR("%s: set acodec micin failed\n", __FUNCTION__);
            close(fdAcodec);
            return HI_FAILURE;
        }*/

        /* set volume plus (0~0x1f,default 0) */
        gain_mic = 0x1f;
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain_mic))
        {
            DEBUG_ERROR("%s: set acodec micin volume failed\n", __FUNCTION__);
            close(fdAcodec);
            return HI_FAILURE;
        }
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &gain_mic))
        {
            DEBUG_ERROR("%s: set acodec micin volume failed\n", __FUNCTION__);
            close(fdAcodec);
            return HI_FAILURE;
        }

        ACODEC_VOL_CTRL stuCtl;
        stuCtl.vol_ctrl= u8Volum;
        stuCtl.vol_ctrl_mute = 0;
        DEBUG_MESSAGE("set voctrl=0x%x\n", stuCtl.vol_ctrl);
        if (ioctl(fdAcodec, ACODEC_SET_ADCL_VOL , &stuCtl))
        {
            DEBUG_ERROR("%s: set acodec micin volume failed\n", __FUNCTION__);
            close(fdAcodec);
            return HI_FAILURE;
        }
        if (ioctl(fdAcodec, ACODEC_SET_ADCR_VOL , &stuCtl))
        {
            DEBUG_ERROR("%s: set acodec micin volume failed\n", __FUNCTION__);
            close(fdAcodec);
            return HI_FAILURE;
        }

        /* set vi volum, maybe have some noise */
        DEBUG_MESSAGE("CHiAi::HiAi_config_acodec==============set audio gain overds  value2=%d\n", u8Volum);
#if 0
      /*  unsigned int dac_deemphasis; 
        dac_deemphasis = 0x1; 
        if (ioctl(fdAcodec, ACODEC_SET_DAC_DE_EMPHASIS, &dac_deemphasis)) 
        { 
            DEBUG_ERROR("ioctl err!\n"); 
        }*/
#endif    

#if !defined(_AV_PLATFORM_HI3518C_)
        unsigned int adc_hpf; 
        adc_hpf = 1;//0x1; 
        if (ioctl(fdAcodec, ACODEC_SET_ADC_HP_FILTER, &adc_hpf)) 
        { 
            DEBUG_ERROR("ioctl err!\n"); 
        }
        usleep(50*1000);
        DEBUG_MESSAGE("CHiAi::HiAi_config_acodec ACODEC_SET_ADC_HP_FILTER value=%d\n", adc_hpf);
#endif  
    }
    close(fdAcodec);    

    return HI_SUCCESS;
}

//!Added for IPC transplant on Nov 2104
int CHiAi::HiAi_config_acodec(AUDIO_SAMPLE_RATE_E enSample)
{
    HI_S32 fdAcodec = -1;
    unsigned int i2s_fs_sel = 0;
    int nRet = HI_FAILURE;
    
    fdAcodec = open( "/dev/acodec", O_RDWR );
    if( fdAcodec < 0 )
    {
        _AV_KEY_INFO_(_AVD_AI_, "CHiAi::HiAi_config_acodec opne /dev/acodec FAILED\n");
        return HI_FAILURE;     
    }

    do
    {
        if( ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL) )
        {
            _AV_KEY_INFO_(_AVD_AI_, "Reset audio codec error\n");
            break;
        }
        usleep(100*1000);

        if ((AUDIO_SAMPLE_RATE_8000 == enSample)
            || (AUDIO_SAMPLE_RATE_11025 == enSample)
            || (AUDIO_SAMPLE_RATE_12000 == enSample))
        {
            i2s_fs_sel = 0x18;
        }
        else if ((AUDIO_SAMPLE_RATE_16000 == enSample)
            || (AUDIO_SAMPLE_RATE_22050 == enSample)
            || (AUDIO_SAMPLE_RATE_24000 == enSample))
        {
            i2s_fs_sel = 0x19;
        }
        else if ((AUDIO_SAMPLE_RATE_32000 == enSample)
            || (AUDIO_SAMPLE_RATE_44100 == enSample)
            || (AUDIO_SAMPLE_RATE_48000 == enSample))
        {
            i2s_fs_sel = 0x1a;
        }
        else 
        {
            _AV_KEY_INFO_(_AVD_AI_, "%s: not support enSample:%d\n", __FUNCTION__, enSample);
            break;
        }

        if( ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel) )
        {
            _AV_KEY_INFO_(_AVD_AI_, "%s: set acodec sample rate failed\n", __FUNCTION__);
            break;
        }

        nRet = HI_SUCCESS;
    }while(0);

    if(0 < fdAcodec)
    {
        close(fdAcodec);
    }

    return nRet;
}


int CHiAi::HiAi_ai_set_input_volume( unsigned char u8Volume )
{
    if( u8Volume == m_u8InputVolume )
    {
        DEBUG_MESSAGE( "CHiAi::HiAi_ai_set_input_volume same volume %d=%d\n", m_u8InputVolume, u8Volume );
        return 0;
    }
#if defined(_AV_PLATFORM_HI3518C_)
    //volum [14-0], 14: min 0:max
    HI_U32 u32OldValue = 0;
    HI_U32 u32NewValue = 0;
    HI_S32 s32Ret;
    s32Ret = HI_MPI_SYS_GetReg( 0x20050078, &u32OldValue );
    if( s32Ret != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAi::HiAi_ai_set_input_volume HI_MPI_SYS_GetReg failed with 0x%08x\n", s32Ret);
        return -1;
    }

    unsigned int volume = (u8Volume & 0x7f);

    u32NewValue = ((u32OldValue & 0x8080FFFF) | (volume<<16) | (volume<<24));

    s32Ret = HI_MPI_SYS_SetReg( 0x20050078, u32NewValue );
    if( s32Ret != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAi::HiAi_ai_set_input_volume HI_MPI_SYS_GetReg failed with 0x%08x\n", s32Ret);
        return -1;
    }
    DEBUG_MESSAGE( "CHiAi::HiAi_ai_set_input_volume %d(0x%x)->%d(0x%x) \n", m_u8InputVolume, u32OldValue,  volume, u32NewValue );
#else
    HI_S32 fdAcodec = -1;
    int nRet = HI_FAILURE;

    fdAcodec = open( "/dev/acodec", O_RDWR );
    if( fdAcodec < 0 )
    {
        _AV_KEY_INFO_(_AVD_AI_, "CHiAi::HiAi_config_acodec opne /dev/acodec FAILED\n");
        return nRet;     
    }

    do
    {
        ACODEC_VOL_CTRL ctrl;
        memset(&ctrl, 0, sizeof(ACODEC_VOL_CTRL));

        ctrl.vol_ctrl = u8Volume;
        if (ioctl(fdAcodec, ACODEC_SET_ADCL_VOL, &ctrl))
        {
            _AV_KEY_INFO_(_AVD_AI_, " ACODEC_SET_GAIN_MICL failed\n");
            break;
        }
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &ctrl))
        {
            _AV_KEY_INFO_(_AVD_AI_, " ACODEC_SET_GAIN_MICR failed\n");
            break;
        }
    }while(0);

    if(0 < fdAcodec)
    {
        close(fdAcodec);
    }
#endif
        m_u8InputVolume = u8Volume;
    return 0;
}
int CHiAi::HiAi_config_acodec_volume()
{
    HI_S32 fdAcodec = -1;
    int nRet = HI_FAILURE;
     _AV_KEY_INFO_(_AVD_AI_, "HiAi_config_acodec_volume\n");
    fdAcodec = open( "/dev/acodec", O_RDWR );
    if( fdAcodec < 0 )
    {
        _AV_KEY_INFO_(_AVD_AI_, "CHiAi::HiAi_config_acodec opne /dev/acodec FAILED\n");
        return nRet;     
    }
    do
    {
        /* set volume plus (0~0x1f,default 0) */
        unsigned int gain_mic = 0;
        switch(pgAvDeviceInfo->Adi_product_type())
        {
            case PTD_712_VD:
            case PTD_712_VA:
            case PTD_712_VE:
            case PTD_712_VF:
            case PTD_712_VB:
            case PTD_712_VC:
            case PTD_714_VA:
            case PTD_714_VB:
            default:
                gain_mic = 0xf;
                break;
            case PTD_920_VA:
                gain_mic = 0x3;//-12db
                break;
        }
        
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain_mic))
        {
            _AV_KEY_INFO_(_AVD_AI_, " ACODEC_SET_GAIN_MICL failed\n");
            break;
        }
        if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &gain_mic))
        {
            _AV_KEY_INFO_(_AVD_AI_, " ACODEC_SET_GAIN_MICR failed\n");
            break;
        }
        _AV_KEY_INFO_(_AVD_AI_, "ACODEC_SET_GAIN_MICL over\n");
        
#if !defined(_AV_PLATFORM_HI3518C_)
        ACODEC_VOL_CTRL stuCtl;
        stuCtl.vol_ctrl= m_u8InputVolume;
        stuCtl.vol_ctrl_mute = 0;
        _AV_KEY_INFO_(_AVD_AI_, " set voctrl=0x%x \n", stuCtl.vol_ctrl);
        if (ioctl(fdAcodec, ACODEC_SET_ADCL_VOL , &stuCtl))
        {
             _AV_KEY_INFO_(_AVD_AI_, "ACODEC_SET_ADCL_VOL failed! \n");
            break;
        }
        if (ioctl(fdAcodec, ACODEC_SET_ADCR_VOL , &stuCtl))
        {
             _AV_KEY_INFO_(_AVD_AI_, " ACODEC_SET_ADCR_VOL failed! \n");
            break;
        }

        /* set vi volum, maybe have some noise */
         _AV_KEY_INFO_(_AVD_AI_, " set audio gain overds  value2=%d\n", m_u8InputVolume);
        unsigned int adc_hpf; 
        adc_hpf = 1;//0x1; 
        if (ioctl(fdAcodec, ACODEC_SET_ADC_HP_FILTER, &adc_hpf)) 
        { 
             _AV_KEY_INFO_(_AVD_AI_, " ACODEC_SET_ADC_HP_FILTER failed! \n");
            break;
        }

        usleep(50*1000);
        _AV_KEY_INFO_(_AVD_AI_, " ACODEC_SET_ADC_HP_FILTER value=%d\n", adc_hpf);
#endif
        nRet = HI_SUCCESS;
    }while(0);
    if(0 < fdAcodec)
    {
        close(fdAcodec);
    }
    return  nRet;
}
int CHiAi::HiAi_start_vqe(AUDIO_DEV ai_dev, AI_CHN ai_chn,AI_VQE_CONFIG_S* pVqe_config) const
{
    if(NULL == pVqe_config)
    {
        _AV_KEY_INFO_(_AVD_AI_, "the vqe config info is null! \n");
        return -1;
    }
    HI_S32 nRet = -1;

#if defined(_AV_PRODUCT_CLASS_IPC_)
    nRet = HI_MPI_AI_SetVqeAttr(ai_dev, ai_chn, 0, 0, pVqe_config);
#else
    AUDIO_DEV ao_dev;
    AO_CHN ao_chn;
    CHiAo ao;
    
    ao.HiAo_get_ao_info(_AO_TALKBACK_, &ao_dev, &ao_chn, NULL);
    nRet = HI_MPI_AI_SetVqeAttr(ai_dev, ai_chn, ao_dev, ao_chn, pVqe_config);
#endif
    if(0 != nRet)
    {
         _AV_KEY_INFO_(_AVD_AI_, "HI_MPI_AI_SetVqeAttr failed, error code:0x%8x \n", nRet);
         return -1;
    }
    nRet = HI_MPI_AI_EnableVqe(ai_dev, ai_chn);
    if(0 != nRet)
    {
        _AV_KEY_INFO_(_AVD_AI_, "HI_MPI_AI_EnableVqe failed! error code:0x%8x \n", nRet);
        return -1;
    }
    
    return 0;
}
int CHiAi::HiAi_stop_vqe(AUDIO_DEV ai_dev, AI_CHN ai_chn) const
{
    HI_S32 nRet = -1;
    nRet = HI_MPI_AI_DisableVqe(ai_dev, ai_chn);
    if(0 != nRet)
    {
        _AV_KEY_INFO_(_AVD_AI_, "HI_MPI_AI_DisableVqe failed! error code:0x%8x \n", nRet);
        return -1;
    }
    return 0;
}

#if defined(_AV_PLATFORM_HI3518C_)
int CHiAi::HiAi_start_hpf(AUDIO_DEV ai_dev, AI_CHN ai_chn,const  AI_HPF_ATTR_S *pstHpfAttr) const
{
    if(NULL == pstHpfAttr)
    {
        DEBUG_ERROR("the hpf attribute is NULL! \n");
        return -1;
    }
    
    HI_S32 s32Ret = HI_FAILURE;
    
    s32Ret = HI_MPI_AI_SetHpfAttr(ai_dev, ai_chn, const_cast<AI_HPF_ATTR_S *> (pstHpfAttr) );
    if (HI_SUCCESS != s32Ret)
    {
        DEBUG_ERROR("HI_MPI_AI_SetHpfAttr failed! errorcode:0x%x \n", s32Ret);
        return -1;
    }
    return 0;
}
int CHiAi::HiAi_stop_hpf(AUDIO_DEV ai_dev, AI_CHN ai_chn) const
{
    HI_S32 s32Ret = HI_FAILURE;
    AI_HPF_ATTR_S hpf_attr;

    memset(&hpf_attr, 0, sizeof(AI_HPF_ATTR_S));
    s32Ret = HI_MPI_AI_GetHpfAttr(ai_dev, ai_chn, &hpf_attr);
    if (HI_SUCCESS != s32Ret)
    {
        DEBUG_ERROR("HI_MPI_AI_GetHpfAttr failed! errorcode:0x%x \n", s32Ret);
    }
    
    hpf_attr.bHpfOpen = HI_FALSE;
    s32Ret = HI_MPI_AI_SetHpfAttr(ai_dev, ai_chn, &hpf_attr);
    if (HI_SUCCESS != s32Ret)
    {
        DEBUG_ERROR("HI_MPI_AI_SetHpfAttr failed! errorcode:0x%x \n", s32Ret);
        return -1;
    }

    return 0;
}
#endif
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
int CHiAi::HiAi_ai_init_aidev_mode(ai_type_e eAiType)
{   
    AUDIO_DEV ai_dev = 0;
    AIO_ATTR_S ai_attr;
    
    if (HiAi_get_ai_info(eAiType, 0, &ai_dev, NULL, &ai_attr) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_ai_init_aidev_mode HiAi_get_ai_info (ai_type:%d) FAILED!\n", eAiType);
        return -1;
    }
    DEBUG_MESSAGE( "CHiAi::HiAi_ai_init_aidev_mode mode=%d\n", ai_attr.enWorkmode);
    if (HiAi_start_ai_dev(ai_dev, &ai_attr) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_ai_init_aidev_mode HiAi_start_ai_dev (ai_dev:%d) FAILED! \n", ai_dev);
        return -1;
    }

    if (HiAi_stop_ai_dev(ai_dev) != 0)
    {
        DEBUG_ERROR( "CHiAi::HiAi_ai_init_aidev_mode HiAi_stop_ai_dev (ai_dev:%d) FAILED! \n", ai_dev);
        return -1;
    }

    return 0;
}
#endif

