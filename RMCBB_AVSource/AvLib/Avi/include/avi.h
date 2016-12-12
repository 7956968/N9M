/*
 * avi.h
 *
 *  Created on: 2012-03-12
 *      Author: gami
 */

#ifndef _AVI_H_
#define _AVI_H_

#include <boost/function.hpp>
#include <boost/bind.hpp>

typedef enum
{
    RS2AVI_IFRAME,
    RS2AVI_PFRAME,
    RS2AVI_AFRAME,
    RS2AVI_INVALID
} rs2avi_frametype_t;

typedef struct {
    struct videoinfo_s
    {
        signed char chno;
        char norm;//0:PAL  1:NTSC
        char resolution;//0-CIF 1-HD1 2-D1 3-QCIF 4-QVGA 5-VGA 6-720P 7-1080P 8-3MP(2048*1536) 9-5MP(2592*1920),10 WQCIF 11 WCIF 12 WHD1 13 WD1(960H)
        float framerate;
    }videoinfo;
    struct audioinfo_s
    {
        signed char chno;/*-1:û����Ƶ���*/
        unsigned char payloadtype;/*0:ADPCM  1:G726  2:G711_ALAW 3:G711_ULAW*/
        char bitwidth;
        unsigned int samplerate;
    }audioinfo;
    unsigned long long streamsize;
} rs2avi_streaminfo_t;

typedef struct
{
	boost::function<int(const char *, size_t)> NtfsOpen;
	boost::function<int(const char *, int flags, mode_t mode)> Open;
	boost::function<ssize_t(int, void*, size_t)> Read;
	boost::function<int(int, const void*, size_t)> Write;
	boost::function<off_t(int, off_t, int)> Seek;
	boost::function<void(int fd)> Close;
	boost::function<int(const char *)> Remove;
} rs2avi_FileOperation_t;

enum
{
	AVI_FSI_STANDARD = 0,
	AVI_FSI_NTFS,
};

typedef struct adpcm_state
{
    short valprev; /* Previous output value */
    char index; /* Index into stepsize table */
} adpcm_state_t;

class CAVI
{
    public:

        CAVI();

        ~CAVI();
        
        /**
        * @brief 设置流信息
        * @pRsInfo 流信息
        * @return
         */
        int SetRecordStreamInfo(rs2avi_streaminfo_t *pRsInfo);

        /**
        * @brief 设置文件系统接口
        * @param type AVI_FSI_STANDARD:使用标准I/O接口 AVI_FSI_NTFS:
        * @param pFSOperate 文件操作接口
         */
        int SetFSInterface(int type, rs2avi_FileOperation_t *pFileOperate);

        /**
        * @brief 创建一个AVI文件
        * @param pFileName 文件名
        * @return 失败:-1
         */
        int AVI_Open(char *pFileName);
        
        /**
        * @brief 写一帧数据到AVI文件
        * @param FileHandle
        * @param pData 帧数据
        * @param len 数据长度
        * @param type 数据类型
        * @return 失败 -1 否则返回写入数据长度
         */
        int AVI_Write(int FileHandle, unsigned char *pData, unsigned int len, rs2avi_frametype_t type);
        
        /**
        * @brief 关闭AVI文件
        * @param FileHandle
        * @param force true:强制close,不写索引 false:正常close,写索引
         */
        int AVI_Close(int FileHandle, bool force=false);
        
    private:

        int InitAVIFileHeader(bool bStreamVideo, bool bStreamAudio, int FileHandle, unsigned long FrameCounter,
		                           const unsigned long VideoTypeForFcc);
        
        void UpdateAVIFileHeader( int FileHandle);
        
        int WriteStream2AVIFile(int FileHandle, unsigned char *pStreamData, unsigned int len, rs2avi_frametype_t type);
        
        int AppendAVIIndex(int FileHandle);
        
        void UpdateAVIFrameRate( float *pfVideoFPS );

        int AdpcmDecoderInit();
        
        int AdpcmDecoder(char indata[], short outdata[], int len);

        int AdpcmDecoderDestroy();

    private:
        
        rs2avi_FileOperation_t m_FileOperate;

        int m_FSIType;

        rs2avi_streaminfo_t m_rsinfo;

        unsigned long m_streamlen;/*��ݲ�����ݳ���*/
        
        unsigned long m_streamlenoffset;/*��ݳ�������ļ�ͷ��ƫ����*/
        
        unsigned long m_frameoffset;/*���ָ������ļ�ͷ��ƫ����*/
        
        unsigned long m_framenumpos[3];/*֡��Ŀ���ļ��е�λ��*/
        
        unsigned int m_videoframenum;/*��Ƶ֡������*/
        
        unsigned int m_audioframenum;/*��Ƶ֡������*/
        
        int m_tmpindexfilefd;
        
        char m_tmpindexfile[64];

        char *m_pIndexBuf;

        int m_IndexLen;

        adpcm_state_t m_AdpcmState;

        short *m_pPCMData;
};
#endif


