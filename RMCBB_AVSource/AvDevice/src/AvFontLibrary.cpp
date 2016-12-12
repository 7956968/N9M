#include "AvFontLibrary.h"
#include "SystemConfig.h"
#if defined(_AV_PLATFORM_HI3515_)
#include "hi_math.h"
#endif

CAvFontLibrary* CAvFontLibrary::m_pInstance = new CAvFontLibrary();

CAvFontLibrary::CAvFontLibrary()
{
#ifdef _AV_PRODUCT_CLASS_IPC_
	_AV_FATAL_((m_pFont_lib_share = new CFontLibShare) == NULL, "OUT OF MEMORY\n");
#else
	_AV_FATAL_((m_pFont_lib_share = new CFontLibShare(false)) == NULL, "OUT OF MEMORY\n");   //!< use not sharememory 
#endif
    m_pFont_lock = new CAvThreadLock();

#ifdef _AV_PRODUCT_CLASS_IPC_
        this->FontInit((char*)"./fonts/DroidSansFallback.ttf");
#else

    Json::Value special_info;
    N9M_GetSpecInfo(special_info);

    //!config the font libs according to system_config file
    if (!special_info["UI_LANG_CFG"].empty() )
    {        
        
        for ( int i =(int)special_info["UI_LANG_CFG"].size() - 1; i >= 0;  i-- )
        {      
            if (!special_info["UI_LANG_CFG"][i]["SELECT"].empty()  
                &&(special_info["UI_LANG_CFG"][i]["SELECT"].isString())     
                &&(special_info["UI_LANG_CFG"][i]["SELECT"].asString() == "true") )
            {
                if (!special_info["UI_LANG_CFG"][i]["FONT_LIB_FILE"].empty() 
                    && (special_info["UI_LANG_CFG"][i]["FONT_LIB_FILE"].isString()) )
                {   
                    std::string language_dir = special_info["UI_LANG_CFG"][i]["FONT_LIB_FILE"].asString();

                    //this->FONT_Init((char*)language_dir.c_str()); 
                    
                    //!< absolute dir
                    
                    std::string ab_dir = "/usr/local/bin/";
                    ab_dir += language_dir;
                    //this->FontInit((char*)ab_dir.c_str()); 
                                  
                    DEBUG_MESSAGE(" select %d th language:  %s!\n", i, special_info["UI_LANG_CFG"][i]["FONT_LIB_FILE"].asString().c_str());
                }
                else
                {
                   // DEBUG_ERROR(" FONT_LIB_FILE invalid!\n");
                }

            }
            else
            {
               // DEBUG_ERROR("Not select this language!\n");

            }

        
        }

    }
    else
    {
        
        DEBUG_ERROR("UI_LANG_CFG invalid!\n");
    }

#endif
}

CAvFontLibrary::~CAvFontLibrary()
{
    FontUninit();
    _AV_SAFE_DELETE_(m_pFont_lock);
    _AV_SAFE_DELETE_(m_pFont_lib_share);
}

CAvFontLibrary* CAvFontLibrary::Afl_get_instance()
{
    return m_pInstance;
}



int CAvFontLibrary::Afl_get_font_size(int font_name, int *pWidth, int *pHeight)
{
    switch(font_name)
    {
        case _AV_FONT_NAME_12_:
        default:
            _AV_POINTER_ASSIGNMENT_(pWidth, 12);
            _AV_POINTER_ASSIGNMENT_(pHeight, 14);
            break;
            
        case _AV_FONT_NAME_16_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 16);
            _AV_POINTER_ASSIGNMENT_(pHeight, 16);
            break;

        case _AV_FONT_NAME_18_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 18);
            _AV_POINTER_ASSIGNMENT_(pHeight, 18);
            break;

        case _AV_FONT_NAME_24_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 24);
            _AV_POINTER_ASSIGNMENT_(pHeight, 24);
            break;

        case _AV_FONT_NAME_36_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 36);
            _AV_POINTER_ASSIGNMENT_(pHeight, 36);
            break;
    }

    return 0;
}


int CAvFontLibrary::Afl_get_char_lattice(int font_name, av_ucs4_t char_unicode, av_char_lattice_info_t *pChar_info, unsigned short lib_mark/* = 1*/)
{
    CharInfo_t char_info;

   // DEBUG_MESSAGE("get char fontname=%d unicode=%ld\n", font_name, char_unicode);
   m_pFont_lock->lock();
    switch(font_name)
    {
        case _AV_FONT_NAME_24_:
        default:
            pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 24, lib_mark, char_info);
            if(pChar_info->pLattice == NULL)
            {
                this->CacheOneFont(char_unicode, 24);
                pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 24, lib_mark, char_info);
            }
            break;

        case _AV_FONT_NAME_12_:
            pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 12, lib_mark, char_info);
            if(pChar_info->pLattice == NULL)
            {
                this->CacheOneFont(char_unicode, 12);
                pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 12, lib_mark, char_info);
            }
            break;

        case _AV_FONT_NAME_16_:
            pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 16, lib_mark, char_info);
            if(pChar_info->pLattice == NULL)
            {
                this->CacheOneFont(char_unicode, 16);
                pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 16, lib_mark, char_info);
            }

            break;
            
        case _AV_FONT_NAME_18_:
            pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 18, lib_mark, char_info);
            if(pChar_info->pLattice == NULL)
            {
                this->CacheOneFont(char_unicode, 18);
                pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 18, lib_mark, char_info);
            }
            break;

        case _AV_FONT_NAME_36_:
            pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 36, lib_mark, char_info);
            if(pChar_info->pLattice == NULL)
            {
                this->CacheOneFont(char_unicode, 36);
                pChar_info->pLattice = (unsigned char *)m_pFont_lib_share->GetCharFromShareMemory(char_unicode, 36, lib_mark, char_info);
            }
            break;
    }

    if(pChar_info->pLattice == NULL)
    {
        m_pFont_lock->unlock();
        return -1;
    }

    pChar_info->width = char_info.Width;
    pChar_info->height = char_info.Height;
    pChar_info->stride = char_info.stride;

    m_pFont_lock->unlock();
    return 0;
}

int CAvFontLibrary::GetUtf8ByteNumber( unsigned char byte )
{
    int count = 8, number = 0;

    if(byte & 0x80)
    {
        while(count > 0)
        {
            if(byte & 0x80)
            {
                number ++;
            }
            else
            {
                break;
            }
            byte = byte << 1;
            count --;
        }
    }
    else
    {
        number = 1;
    }

    return number;
}

int CAvFontLibrary::FontInit( char* pszTTFPath )
{
    FT_Error ret;
    ret = FT_Init_FreeType(&m_library);

    if( ret )
    {
        perror("init failed\n");
        return -1;
    }

    ret = FT_New_Face(m_library, pszTTFPath, 0, &m_Face);
    if ( ret == 1)
    {
        printf("font lib not exist \n");
        return -1;
    }
    if(ret == FT_Err_Unknown_File_Format)
    {
        perror("init failed\n");
        return -1;
    }

    return 0;
}

int CAvFontLibrary::CacheOneFont( unsigned long unicode, unsigned char u8FontSize )
{
    CharInfo_t stuInfo;
    if( (m_pFont_lib_share->GetCharFromShareMemory(unicode, u8FontSize, 0xffff, stuInfo) != NULL) && (stuInfo.LibMark == 0xffff))
    {
        return true;
    }
    int ret = 0;
    do
    {
        FT_Error ret;
        ret = FT_Set_Pixel_Sizes( m_Face, u8FontSize, u8FontSize-2 );
        if( ret != 0 )
        {
            perror("FT_Set_Pixel_Sizes\n");
            ret = -1;
            break;
        }

        FT_UInt gindex = FT_Get_Char_Index( m_Face, unicode );
        if( gindex == 0 )
        {
            //perror("FT_Get_Char_Index\n");
            ret = -1;
            break;
        }

        ret = FT_Load_Glyph( m_Face, gindex, FT_LOAD_MONOCHROME );
        if( ret != 0 )
        {
            perror("FT_Load_Glyph\n");
            ret = -1;
            break;
        }

        if( m_Face->glyph->format != FT_GLYPH_FORMAT_BITMAP )
        {
            ret = FT_Render_Glyph( m_Face->glyph, FT_RENDER_MODE_MONO );
            if(ret)
            {
                perror("render Glyph errr!\n");
                ret = -1;
                break;
            }
        }

        if( m_Face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO )
        {
            perror("Is not a monochrome bitmap!\n");
            ret = -1;
            break;
        }

    int width = m_Face->glyph->bitmap.width;
    int height = m_Face->glyph->bitmap.rows;
    int stride = abs(m_Face->glyph->bitmap.pitch);
    int xoffset = 0;
    char Temp;

    // CharInfo_t stuInfo;
    stuInfo.FontSize = u8FontSize;
    stuInfo.Width = width;
    if( stuInfo.Width > 0 )
    {
        stuInfo.Width += 2;
    }
    else
    {
        stuInfo.Width = 6;
    }
    stuInfo.Height = u8FontSize + 2;
    stuInfo.stride = (stuInfo.Width + 7) / 8; 
    stuInfo.DataLen = stuInfo.Height * stuInfo.stride;
    stuInfo.Unicode = unicode;
    stuInfo.LibMark = 0xffff;
    //DEBUG_MESSAGE("len=%d\n", stuInfo.DataLen);
    char *buffer = new char[stuInfo.DataLen];
    memset( buffer, 0, stuInfo.DataLen );

        int n = u8FontSize*m_Face->bbox.yMax/(ABS(m_Face->bbox.yMax)+ABS(m_Face->bbox.yMin)) - m_Face->glyph->bitmap_top;
        /*DEBUG_MESSAGE("unicode=%ld n=%d w=%d h=%d picth=%d top=%d fontsize=%d HbearingY=%d VbearingY=%d mw=%ld mh=%ld vA=%ld ha=%ld lineV=%ld orY=%ld\n"
        , unicode, n,width, height,m_Face->glyph->bitmap.pitch, m_Face->glyph->bitmap_top
        , u8FontSize, (int)m_Face->glyph->metrics.horiBearingY, (int)m_Face->glyph->metrics.vertBearingY
        , m_Face->glyph->metrics.width, m_Face->glyph->metrics.height
        , m_Face->glyph->metrics.horiAdvance, m_Face->glyph->metrics.vertAdvance, m_Face->glyph->linearVertAdvance
        , m_Face->glyph->advance.y);
        DEBUG_MESSAGE("bbox.yMax=%ld yMin=%ld\n", m_Face->bbox.yMax, m_Face->bbox.yMin);*/
        if( stuInfo.Height < height + n )
        {
            n = stuInfo.Height - height;
        }
        if( n < 0 )
        {
            n = 0;
        }
        for( int i = 0; i < height; i++ )
        {
            if( (n + i) >= stuInfo.Height )
            {
                break;
            }
            for( int j = 0; j < width; j++ )
            {
                if(m_Face->glyph->bitmap.pitch > 0)
                {
                    Temp = m_Face->glyph->bitmap.buffer[(i * stride) + (j / 8)];
                }
                else
                {
                    Temp = m_Face->glyph->bitmap.buffer[((height - i - 1) * stride) + (j / 8)];
                }
                if( (Temp >> (7 - (j%8))) & 0x01 )//CHK_BIT(&Temp, 7 - (j%8)) )
                {
                    buffer[((n + i) * stuInfo.stride) + ((xoffset + j + 1) / 8)] |= (1<<(7-(xoffset + j + 1)%8));
                    //SET_BIT(&buffer[((n + i) * stuInfo.stride) + ((xoffset + j + 1) / 8)], 7 - ((xoffset + j + 1)%8));
                    //DEBUG_MESSAGE("*");
                }
                else
                {
                    //DEBUG_MESSAGE(" ");
                }
            }
            //DEBUG_MESSAGE("\n");	
        }
        bool bRet = false;
        bRet = m_pFont_lib_share->CacheCharToShareMemory( buffer, stuInfo); 
        if ( bRet == false)
        {
            DEBUG_ERROR("CacheCharToShareMemory failed!\n");
            m_pFont_lib_share->ClearAllCacheChar();
            if (m_pFont_lib_share->CacheCharToShareMemory( buffer, stuInfo)== false)
            {
                DEBUG_ERROR("Again CacheCharToShareMemory failed!\n");;
            }
            
        }
        _AV_SAFE_DELETE_ARRAY_(buffer);
    }while(0);
    
    return ret;
}

int CAvFontLibrary::CacheFontsToShareMem( const char* pszStr )
{
    if( !m_pFont_lib_share || !pszStr )
    {
        DEBUG_ERROR( "CAvFontLibrary::CacheFontsToShareMem cachefonts failed\n");
        return -1;
    }

    unsigned char byte;
    int ByteNum;
    unsigned int unicode;
    unsigned u32Len = strlen(pszStr);
    const char* ptr = pszStr;
    while( u32Len > 0 )
    {
        byte = *ptr;
        ByteNum = this->GetUtf8ByteNumber(byte);
        if( (ByteNum > 0) && (ByteNum <= 4) )
        {
            u32Len -= ByteNum;
            unicode = ((0xff >> ByteNum) & byte) << ((ByteNum - 1) * 6);
            ByteNum--;
            ptr++;
            byte = *ptr;
            while(ByteNum > 0)
            {
                unicode |= ((byte & 0x3f) << ((ByteNum - 1) * 6));
                ByteNum--;
                ptr++;
                byte = *ptr;
            }
            m_pFont_lock->lock();
            CacheOneFont( unicode, 36 );
            CacheOneFont( unicode, 24 );
            CacheOneFont( unicode, 18 );
            //CacheOneFont( unicode, 16 );
            CacheOneFont( unicode, 12 );
            m_pFont_lock->unlock();
        }
        else
        {
            perror("invalid string!\n");
            break;
        }
    }

    return 0;
}

int CAvFontLibrary::FontUninit()
{
    FT_Done_Face(m_Face);
    FT_Done_FreeType(m_library);

    return 0;
}

