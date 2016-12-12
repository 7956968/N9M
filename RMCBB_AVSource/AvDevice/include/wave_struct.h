#ifndef __WAVE_STRUCT_H__
#define __WAVE_STRUCT_H__
#include <stdlib.h>
#include <stdio.h>


/*
RIFF������ݽṹ
    ---------------------------------------------------
    ƫ����    ����      �ֽ���   ��������       ����
    ---------------------------------------------------
    00     ��־��   ���� 4       �ַ�     "RIFF"��Ascii��

    04     �ļ����� ���� 4       ������    �ļ������ֽ���

    08     WAV��־  ��   4       �ַ�     "WAVE"��Ascii��
    ---------------------------------------------------
*/
typedef struct _ws_riff_chunk_{
    char riff_chunk_id[4];
    unsigned long riff_chunk_size;
    char wave_chunk_id[4];
}ws_riff_chunk_t;


/*
��ʽ������ݽṹ
    ----------------------------------------------------------------------------
    ƫ�Ƶ�ַ ���ֽ���  ������������  ������������
    ----------------------------------------------------------------------------
    0C       ����4       �����ַ�     �������θ�ʽ��־"fmt "
    10       ����4       ����������   ������ʽ�鳤�ȣ�һ��=16����=18��ʾ�����2�ֽڸ�����Ϣ��
    14       ����2       ��������    ������ʽ���ֵ=1��ʾ���뷽ʽΪPCM���ɱ��룩
    16       ����2       ��������     ������������������=1��˫����=2��
    18       ����4       ����������  ��������Ƶ�ʣ�ÿ������������ʾÿ��ͨ���Ĳ����ٶȣ�
    1C       ����4       ����������   �������ݴ������ʣ�ÿ���ֽ�=����Ƶ�ʡ�ÿ�������ֽ�����
    20       ����2       ��������     ����ÿ�������ֽ������ֳƻ�׼��=ÿ������λ������������8��
    22       ����2      ��������    ����ÿ������λ�����ֳ�����λ����
    24       ����2       ��������    ����������Ϣ����ѡ��ͨ���鳤�����ж����ޣ�
    ----------------------------------------------------------------------------
*/
typedef struct _ws_fmt_chunk_{
    char fmt_chunk_id[4];
    unsigned long fmt_chunk_size;
#define _WAF_PCM_   1
    unsigned short audio_format;
    unsigned short channel_number;
    unsigned long sample_rate;/*������80000��44100��*/
    unsigned long byte_rate;/*SampleRate * NumChannels * BitsPerSample / 8*/
    unsigned short block_align;
    unsigned short bits_per_sample;
}ws_fmt_chunk_t;



typedef struct _ws_data_chunk_{
    char data_chunk_id[4];    
    unsigned long size;
}ws_data_chunk_t;


#endif/*__WAVE_STRUCT_H__*/

