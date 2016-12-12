#include "AvDebug.h"


unsigned long gAv_debug_info = 0;
unsigned long gAv_key_info = 0;


void Av_debug_level(unsigned long debug_info, unsigned long key_info)
{
    gAv_debug_info = debug_info;
    gAv_key_info = key_info;
}

