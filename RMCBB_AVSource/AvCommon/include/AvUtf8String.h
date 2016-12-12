/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVUTF8STRING_H__
#define __AVUTF8STRING_H__
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AvCommonType.h"


class CAvUtf8String{
    public:
       CAvUtf8String(const char *pUtf8_string = NULL, bool copy = false)
        {
            m_pUtf8_string = pUtf8_string;
            m_copy = copy;
            if((pUtf8_string != NULL) && (copy == true))
            {
                m_pUtf8_string = strdup(pUtf8_string);
            }
            m_pCur_ptr = m_pUtf8_string;
        }

       ~CAvUtf8String()
       {
            if((m_copy == true) && (m_pUtf8_string != NULL))
            {
                free((char *)m_pUtf8_string);
            }
       }

    public:
        int Number_char()
        {
            const char *pByte_ptr = m_pUtf8_string;
            if((pByte_ptr == NULL) || (*pByte_ptr == '\0'))
            {
                return 0;
            }

            int char_count = 0;
            int byte_count = 0;
            while(*pByte_ptr != '\0')
            {
                byte_count = Char_byte_number(*pByte_ptr);
                if((byte_count > 4) || ((int)strlen(pByte_ptr) < byte_count))
                {
                    break;
                }
                pByte_ptr += byte_count;
            }

            return char_count;
        }

        bool Next_char(av_ucs4_t *pUnicode)
        {
            if((m_pCur_ptr == NULL) || (*m_pCur_ptr == '\0'))
            {
                return false;
            }

            int byte_count = Char_byte_number(*m_pCur_ptr);
            if((byte_count > 4) || ((int)strlen(m_pCur_ptr) < byte_count))
            {
                return false;
            }

            av_ucs4_t iTmp = (av_ucs4_t)*m_pCur_ptr;
            *pUnicode = (iTmp & (~(0xff << (8 - byte_count)))) << ((byte_count - 1) * 6);
            m_pCur_ptr ++;
            byte_count --;
            while(byte_count >= 1)
            {
                iTmp = (av_ucs4_t)*m_pCur_ptr;
                *pUnicode |= ((iTmp & 0x3f) << ((byte_count - 1) * 6));
                m_pCur_ptr ++;
                byte_count --;
            }

            return true;
        }

        bool Reset(const char *pUtf8_string = NULL)
        {
            if((m_copy == false) && (pUtf8_string != NULL))
            {
                m_pUtf8_string = pUtf8_string;
            }

            m_pCur_ptr = m_pUtf8_string;

            return true;
        }
		
		int Chars_bytes_current()
		{
			return (int)(m_pCur_ptr-m_pUtf8_string);
		}
    private:
        int Char_byte_number(unsigned char first_byte)
        {
            int byte_count = 1;
            if((first_byte & 0x80) != 0)
            {
                int bit_index = 6;
                while((bit_index >= 0) && ((first_byte & (0x01 << bit_index)) != 0))
                {
                    byte_count ++;
                    bit_index --;
                }
            }

            return byte_count;
        }

    private:
        bool m_copy;
        const char *m_pUtf8_string;
        const char *m_pCur_ptr;
};


#endif/*__AVUTF8STRING_H__*/

