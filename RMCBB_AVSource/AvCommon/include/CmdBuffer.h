/**
 * @file CmdBuffer.h
 * @author jcxu
 * @data 2011-11-16
 * @brief 指令栈管理，先进后出
 * */
#ifndef __CMDBUFFER_H__
#define __CMDBUFFER_H__
#include "AutoCri.h"

struct CmdData_t
{
	int iCmd;
	unsigned short usDataLen;
	unsigned short usMagic; /**< 未枷锁的数据校验 */
	char* pszData;
	CmdData_t* pNext;
};

class CCmdBuffer
{
public:
	CCmdBuffer();
	~CCmdBuffer();

public:
	int PushCmd( int iCmd, const char* pszData, unsigned short usDataLen, bool bSameCover = false );
	int PullCmd( int* iCmd, char* pszData );
	int GetNextCmdInfo( int* iCmd, int* iDataLen );

	/**
	 * @brief 获取下一条指令的长度，-1表示无指令
	 * */
	int GetNextCmdInfo();

    int ClearAllCmd();

private:
	pthread_mutex_t m_cs;
	CmdData_t* m_pData;
//	uint8_t m_u8BadMagicCount;
};

#endif /* __CMDBUFFER_H__ */
