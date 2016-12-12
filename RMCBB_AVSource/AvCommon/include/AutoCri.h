/**
 * @file AutoCri.h
 * @author jcxu
 * @date 2011-3-25
 * @brief 自动加解锁
 * */
#ifndef __AUTOCRI_H__
#define __AUTOCRI_H__
#include <pthread.h>
#include <stdio.h>

class CAutoCri
{
public:
	CAutoCri( pthread_mutex_t& cs )
	{
		m_pCs = &cs;
		m_bLocked = true;
		pthread_mutex_lock(m_pCs);
	}
	
	~CAutoCri()
	{
		Release();
	}
	
/*protected:
	CAutoCri();
	CAutoCri(const CAutoCri& cri);
	CAutoCri& operator= (const CAutoCri& cri);
	*/
public:	
	bool Release()
	{
		if( m_pCs && m_bLocked )
		{
			m_bLocked = false;
			pthread_mutex_unlock(m_pCs);
			m_pCs = NULL;
			return true;
		}

		return false;
	}

public:
	bool UnLock()
	{
		if( m_pCs && m_bLocked )
		{
			m_bLocked = false;
			pthread_mutex_unlock(m_pCs);
			return true;
		}
		printf("bad lock call\n");
		return false;
	}
	bool Lock()
	{
		if( m_pCs && !m_bLocked )
		{
			m_bLocked = true;
			pthread_mutex_lock(m_pCs);
			return true;
		}
		printf("bad lock call\n");
		return false;
	}

private:
	pthread_mutex_t* m_pCs;
	bool m_bLocked;
};

#endif

