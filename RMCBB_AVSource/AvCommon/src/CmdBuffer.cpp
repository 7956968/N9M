#include "CmdBuffer.h"
#include <string.h>
#include <stdio.h>
//#define CMD_OK_MAGIC_VALUE 0x123456;

CCmdBuffer::CCmdBuffer()
{
    pthread_mutex_init(&m_cs, NULL);
    m_pData = NULL;
    //m_u8BadMagicCount = 0;
}

CCmdBuffer::~CCmdBuffer()
{
    pthread_mutex_destroy(&m_cs);
}

int CCmdBuffer::PushCmd( int iCmd, const char* pszData, unsigned short usDataLen, bool bSameCover )
{
    if( usDataLen > 1024 )
    {
        printf("##################################command is too long(%d>1024)\n", usDataLen);
        return -1;
    }

    if( !m_pData )
    {
	 pthread_mutex_lock(&m_cs); 
        CmdData_t* pTemp = new CmdData_t;
        pTemp->iCmd = iCmd;
        pTemp->pNext = NULL;
        pTemp->usDataLen = usDataLen;
        if( usDataLen > 0 )
        {
            pTemp->pszData = new char[usDataLen];
            memcpy( pTemp->pszData, pszData, usDataLen );
        }
        else
        {
            pTemp->pszData = NULL;
        }
        pthread_mutex_unlock(&m_cs); 
        m_pData = pTemp;
        return 0;
    }
	
    pthread_mutex_lock(&m_cs); 
    CmdData_t* pCmdData = m_pData;
    while( pCmdData )
    {
        if( bSameCover && pCmdData->iCmd == iCmd ) //if not one thread ,this need lock
        {
            printf("CCmdBuffer,same cmd is found cmd=%d len=%d oldlen=%d\n", iCmd, usDataLen, pCmdData->usDataLen );
            if( pCmdData->pszData && pCmdData->usDataLen != usDataLen )
            {
                if( pCmdData->usDataLen > 0 )
                {
                    delete [] pCmdData->pszData;
                    pCmdData->pszData = NULL;
                }
                if( usDataLen > 0 )
                {
                    pCmdData->pszData = new char[usDataLen];
                    pCmdData->usDataLen = usDataLen;
                }
            }
            memcpy( pCmdData->pszData, pszData, usDataLen );
            break;
        }
        if( pCmdData->pNext == NULL )
        {
            CmdData_t* pTemp = new CmdData_t;
            pTemp->iCmd = iCmd;
            pTemp->pNext = NULL;
            pTemp->usDataLen = usDataLen;
            if( usDataLen > 0 )
            {
                pTemp->pszData = new char[usDataLen];
                memcpy( pTemp->pszData, pszData, usDataLen );
            }
            else
            {
                pTemp->pszData = NULL;
            }
            pCmdData->pNext = pTemp;
            break;
        }
        pCmdData = pCmdData->pNext;
    }
    pthread_mutex_unlock(&m_cs); 
    return 0;
}

int CCmdBuffer::PullCmd( int* iCmd, char* pszData )
{
    if( m_pData )
    {
        pthread_mutex_lock(&m_cs); 
        *iCmd = m_pData->iCmd;
        if( m_pData->usDataLen > 0 )
        {
            memcpy( pszData, m_pData->pszData, m_pData->usDataLen );
        }

        CmdData_t* pTemp = m_pData;
        m_pData = m_pData->pNext;

        delete [] pTemp->pszData;
        delete pTemp;
        pthread_mutex_unlock(&m_cs); 
        return 0;
    }
    return -1;
}

int CCmdBuffer::GetNextCmdInfo( int* iCmd, int* iDataLen )
{
    if( m_pData )
    {
        *iCmd = m_pData->iCmd;
        *iDataLen = m_pData->usDataLen;

        return 0;
    }
    return -1;
}

int CCmdBuffer::GetNextCmdInfo()
{
    if( m_pData )
    {
        return m_pData->usDataLen;
    }
    return -1;
}

int CCmdBuffer::ClearAllCmd()
{
	if(m_pData == NULL)
	{
		return 0;
	}
    CmdData_t* pTemp = m_pData;
    pthread_mutex_lock(&m_cs); 	
    while( m_pData )
    {
        pTemp = m_pData;
        m_pData = m_pData->pNext;
        if (pTemp->pszData)
        {
            delete [] pTemp->pszData;
            pTemp->pszData = NULL;
        }
        
        delete pTemp;
    }
    pthread_mutex_unlock(&m_cs); 
    return 0;
}

