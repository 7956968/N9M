/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVFONTLIBRARY_H__
#define __AVFONTLIBRARY_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include "CFontLibShare.h"
#include <list>
#include "CommonDebug.h"
#include <ft2build.h>
#include "AvThreadLock.h"
#include FT_FREETYPE_H 


#define _AV_FONT_NAME_12_        0
#define _AV_FONT_NAME_18_        1
#define _AV_FONT_NAME_24_        2
#define _AV_FONT_NAME_36_        3
#define _AV_FONT_NAME_16_        4


typedef struct _av_char_lattice_info_{
    int width;/*pixel*/
    int height;
    int stride;/*byets for a row*/
    unsigned char *pLattice;
}av_char_lattice_info_t;


typedef struct _av_char_lattice_cache_item_{
    int font_name;
    unsigned short font_type;
    av_ucs4_t unicode;
    int width;/*pixel*/
    int height;
    int stride;/*bytes for a row*/
    unsigned char *pLattice;
}av_char_lattice_cache_item_t;


class CAvFontLibrary{
    private:
        CAvFontLibrary();
        ~CAvFontLibrary();
        CAvFontLibrary(const CAvFontLibrary&);
        CAvFontLibrary& operator=(const CAvFontLibrary&);

    public:
        static CAvFontLibrary* Afl_get_instance(); 
        int Afl_get_font_size(int font_name, int *pWidth, int *pHeight);
        int Afl_get_char_lattice(int font_name, av_ucs4_t char_unicode, av_char_lattice_info_t *pChar_info, unsigned short lib_mark = 1);
        
    public:
        /**
         * @brief for the machines without UI module. U gotta make sure that the osd thread has beed paused !
         * @param pszStr 
         * @return 0:success, -1:failed
         **/
        int CacheFontsToShareMem( const char* pszStr );
        int FontInit( char* pszTTFPath );
        
    private:
        int GetUtf8ByteNumber( unsigned char byte );

        int CacheOneFont( unsigned long unicode, unsigned char u8FontSize );

//        int FontInit( char* pszTTFPath );
        int FontUninit();

    private:
        FT_Face m_Face;
        FT_Library m_library;

    private:
        CFontLibShare *m_pFont_lib_share;
        CAvThreadLock *m_pFont_lock;
        static CAvFontLibrary * m_pInstance;
        
};


#endif/*__AVFONTLIBRARY_H__*/
