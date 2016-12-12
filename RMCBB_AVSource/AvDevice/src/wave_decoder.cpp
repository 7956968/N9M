#include "wave_decoder.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


#define _WD_PRINT_(fmt, args...) do{\
            printf(fmt, ##args);\
    }while(0)


CWaveDecoder::CWaveDecoder(const char *pWave_file)
{
    m_pWs_fmt = NULL;
    m_pWs_riff = NULL;
    m_pWs_data = NULL;
    m_wave_state = _WAVE_STATE_error_;
    m_wave_file_fd = open(pWave_file, O_RDONLY);
    if(m_wave_file_fd >= 0)
    {
        if(Wd_parser() == 0)
        {
            m_wave_state = _WAVE_STATE_ok_;
        }
    }
    else
    {
        _WD_PRINT_("CWaveDecoder::CWaveDecoder Open [%s] file FAILED\n", pWave_file);
    }
}


CWaveDecoder::~CWaveDecoder()
{
    if(m_wave_file_fd >= 0)
    {
        close(m_wave_file_fd);
        m_wave_file_fd = -1;
    }

    if(m_pWs_fmt != NULL)
    {
        delete m_pWs_fmt;
        m_pWs_fmt = NULL;
    }

    if(m_pWs_riff != NULL)
    {
        delete m_pWs_riff;
        m_pWs_fmt = NULL;
    }

    if(m_pWs_data != NULL)
    {
        delete m_pWs_data;
        m_pWs_data = NULL;
    }
}


int CWaveDecoder::Wd_parser()
{
    if(m_wave_file_fd < 0)
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser (m_wave_file_fd < 0)\n");
        return -1;
    }

    if(lseek(m_wave_file_fd, 0, SEEK_SET) != 0)
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser lseek FAILED\n");
        return -1;
    }

    struct stat stat_buf;
    if(fstat(m_wave_file_fd, &stat_buf) != 0)
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser fstat FAILED\n");
        return -1;
    }

    /*read riff chunk*/
    if(m_pWs_riff == NULL)
    {
        m_pWs_riff = new ws_riff_chunk_t;
        memset(m_pWs_riff, 0, sizeof(ws_riff_chunk_t));
    }

    if(read(m_wave_file_fd, m_pWs_riff, sizeof(ws_riff_chunk_t)) != sizeof(ws_riff_chunk_t))
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser read FAILED\n");
        return -1;
    }

    if((m_pWs_riff->riff_chunk_id[0] != 'R') || (m_pWs_riff->riff_chunk_id[1] != 'I') || (m_pWs_riff->riff_chunk_id[2] != 'F') || (m_pWs_riff->riff_chunk_id[3] != 'F') || 
                (m_pWs_riff->wave_chunk_id[0] != 'W') || (m_pWs_riff->wave_chunk_id[1] != 'A') || (m_pWs_riff->wave_chunk_id[2] != 'V') || (m_pWs_riff->wave_chunk_id[3] != 'E') || 
                                    ((m_pWs_riff->riff_chunk_size + 8) != (unsigned long)stat_buf.st_size))
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser Invalid riff CHUNK\n");
        return -1;
    }

    /*read format chunk*/
    if(m_pWs_fmt == NULL)
    {
       m_pWs_fmt = new ws_fmt_chunk_t;
       memset(m_pWs_fmt, 0, sizeof(ws_fmt_chunk_t));
    }

    if(read(m_wave_file_fd, m_pWs_fmt, sizeof(ws_fmt_chunk_t)) != sizeof(ws_fmt_chunk_t))
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser read FAILED\n");
        return -1;
    }

    if((m_pWs_fmt->fmt_chunk_id[0] != 'f') || (m_pWs_fmt->fmt_chunk_id[1] != 'm') || (m_pWs_fmt->fmt_chunk_id[2] != 't') || 
            (m_pWs_fmt->fmt_chunk_id[3] != ' '))
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser Invalid fmt CHUNK1(%02x %02x %02x %02x)\n", m_pWs_fmt->fmt_chunk_id[0], m_pWs_fmt->fmt_chunk_id[1], 
                m_pWs_fmt->fmt_chunk_id[2], m_pWs_fmt->fmt_chunk_id[3]);
        return -1;
    }

    if(((m_pWs_fmt->sample_rate * m_pWs_fmt->channel_number * m_pWs_fmt->bits_per_sample / 8) != m_pWs_fmt->byte_rate) || 
                                        ((m_pWs_fmt->channel_number * m_pWs_fmt->bits_per_sample / 8) != m_pWs_fmt->block_align))
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser Invalid fmt CHUNK2\n");
        return -1;
    }

    if((m_pWs_fmt->channel_number != 1) || (m_pWs_fmt->audio_format != _WAF_PCM_))
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser NOT SUPPORT FORMAT(channel:%d)(format:%d)\n", m_pWs_fmt->channel_number, m_pWs_fmt->audio_format);
        return -1;
    }

    /*read data chunk*/
    if(m_pWs_data == NULL)
    {
        m_pWs_data = new ws_data_chunk_t;
        memset(m_pWs_data, 0, sizeof(ws_data_chunk_t));
    }

    if(read(m_wave_file_fd, m_pWs_data, sizeof(ws_data_chunk_t)) != sizeof(ws_data_chunk_t))
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser read FAILED\n");
        return -1;
    }

    if((m_pWs_data->data_chunk_id[0] != 'd') || (m_pWs_data->data_chunk_id[1] != 'a') || (m_pWs_data->data_chunk_id[2] != 't') || 
            (m_pWs_data->data_chunk_id[3] != 'a') || (m_pWs_data->size != (stat_buf.st_size - sizeof(ws_riff_chunk_t) - m_pWs_fmt->fmt_chunk_size - 16)))
    {
        _WD_PRINT_("CWaveDecoder::Wd_parser Invalid data CHUNK(%c%c%c%c)(%ld, %ld, %d, %ld)\n", m_pWs_data->data_chunk_id[0], m_pWs_data->data_chunk_id[1], 
                    m_pWs_data->data_chunk_id[2], m_pWs_data->data_chunk_id[3], 
                        m_pWs_data->size, stat_buf.st_size, sizeof(ws_riff_chunk_t), m_pWs_fmt->fmt_chunk_size);
        return -1;
    }

    return 0;
}
    

int CWaveDecoder::Wd_get_data(int point_num, unsigned char *pData_buffer, int *pGet_point_num)
{
    if(m_wave_state == _WAVE_STATE_error_)
    {
        return _WDGD_error_;
    }

    int read_len = read(m_wave_file_fd, pData_buffer, point_num * m_pWs_fmt->bits_per_sample / 8);

    if(read_len == 0)
    {
        return _WDGD_over_;
    }
    else if(read_len < 0)
    {
        return _WDGD_error_;
    }
    
    *pGet_point_num = read_len / (m_pWs_fmt->bits_per_sample / 8);

    return _WDGD_ok_;
}

int CWaveDecoder::Wd_reset()
{
    if(m_wave_state == _WAVE_STATE_error_)
    {
        return -1;
    }

    int seek_pos = sizeof(ws_riff_chunk_t) + m_pWs_fmt->fmt_chunk_size + 16;

    if(lseek(m_wave_file_fd, seek_pos, SEEK_SET) != seek_pos)
    {
        _WD_PRINT_("CWaveDecoder::Wd_reset ERROR\n");
        m_wave_state = _WAVE_STATE_error_;
        return -1;
    }
    
    return 0;
}


int CWaveDecoder::Wd_get_info(int *pSample_rate, int *pBits_per_sample)
{
    if(m_wave_state == _WAVE_STATE_error_)
    {
        return -1;
    }

    *pSample_rate = m_pWs_fmt->sample_rate;
    *pBits_per_sample = m_pWs_fmt->bits_per_sample;

    return 0;
}




