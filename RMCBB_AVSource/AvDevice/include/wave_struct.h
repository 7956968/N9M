#ifndef __WAVE_STRUCT_H__
#define __WAVE_STRUCT_H__
#include <stdlib.h>
#include <stdio.h>


/*
RIFF块的数据结构
    ---------------------------------------------------
    偏移量    名称      字节数   数据类型       内容
    ---------------------------------------------------
    00     标志符   　　 4       字符     "RIFF"的Ascii码

    04     文件长度 　　 4       长整形    文件的总字节数

    08     WAV标志  　   4       字符     "WAVE"的Ascii码
    ---------------------------------------------------
*/
typedef struct _ws_riff_chunk_{
    char riff_chunk_id[4];
    unsigned long riff_chunk_size;
    char wave_chunk_id[4];
}ws_riff_chunk_t;


/*
格式块的数据结构
    ----------------------------------------------------------------------------
    偏移地址 　字节数  　　数据类型  　　　　内容
    ----------------------------------------------------------------------------
    0C       　　4       　　字符     　　波形格式标志"fmt "
    10       　　4       　　长整型   　　格式块长度（一般=16，若=18表示最后有2字节附加信息）
    14       　　2       　　整型    　　格式类别（值=1表示编码方式为PCMμ律编码）
    16       　　2       　　整型     　　声道数（单声道=1，双声音=2）
    18       　　4       　　长整型  　　采样频率（每秒样本数，表示每个通道的播放速度）
    1C       　　4       　　长整型   　　数据传送速率（每秒字节=采样频率×每个样本字节数）
    20       　　2       　　整型     　　每个样本字节数（又称基准块=每个样本位数×声道数÷8）
    22       　　2      　　整型    　　每个样本位数（又称量化位数）
    24       　　2       　　整型    　　附加信息（可选，通过块长度来判断有无）
    ----------------------------------------------------------------------------
*/
typedef struct _ws_fmt_chunk_{
    char fmt_chunk_id[4];
    unsigned long fmt_chunk_size;
#define _WAF_PCM_   1
    unsigned short audio_format;
    unsigned short channel_number;
    unsigned long sample_rate;/*采样率80000，44100等*/
    unsigned long byte_rate;/*SampleRate * NumChannels * BitsPerSample / 8*/
    unsigned short block_align;
    unsigned short bits_per_sample;
}ws_fmt_chunk_t;



typedef struct _ws_data_chunk_{
    char data_chunk_id[4];    
    unsigned long size;
}ws_data_chunk_t;


#endif/*__WAVE_STRUCT_H__*/

