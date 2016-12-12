#ifndef __WAVE_DECODER_H__
#define __WAVE_DECODER_H__
#include "wave_struct.h"


class CWaveDecoder{
    public:
        CWaveDecoder(const char *pWave_file);
        ~CWaveDecoder();

    public:
/*get the information of the wave*/
        int Wd_get_info(int *pSample_rate, int *pBits_per_sample);
/*get next frame*/
#define _WDGD_error_    -1
#define _WDGD_ok_       0
#define _WDGD_over_     1
        int Wd_get_data(int point_num, unsigned char *pData_buffer, int *pGet_point_num);
/*reset, then you can get the first frame*/
        int Wd_reset();

    private:
        int Wd_parser();

    private:
        int m_wave_file_fd;
        int m_frame_buffer_len;
#define _WAVE_STATE_error_ 0
#define _WAVE_STATE_ok_ 1
        int m_wave_state;

    private:
        ws_riff_chunk_t *m_pWs_riff;
        ws_fmt_chunk_t *m_pWs_fmt;
        ws_data_chunk_t *m_pWs_data;
};


#endif/*__WAVE_DECODER_H__*/


