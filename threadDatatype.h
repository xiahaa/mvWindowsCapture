#ifndef _THREAD_DATATYPE_H_
#define _THREAD_DATATYPE_H_

#include <windows.h>

using namespace std;

//-----------------------------------------------------------------------------
/// to get thread safe access to the std out and the display module
class CriticalSection
	//-----------------------------------------------------------------------------
{
	CRITICAL_SECTION m_criticalSection;
public:
	CriticalSection()
	{
		InitializeCriticalSection(&m_criticalSection);
	}
	~CriticalSection()
	{
		DeleteCriticalSection(&m_criticalSection);
	}
	void lock(void)
	{
		EnterCriticalSection(&m_criticalSection);
	}
	void unlock(void)
	{
		LeaveCriticalSection(&m_criticalSection);
	}
};
//-----------------------------------------------------------------------------
class LockedScope
	//-----------------------------------------------------------------------------
{
	CriticalSection c_;
public:
	explicit LockedScope(CriticalSection& c) : c_(c)
	{
		c_.lock();
	}
	~LockedScope()
	{
		c_.unlock();
	}
};
//-----------------------------------------------------------------------------
class ThreadData
	//-----------------------------------------------------------------------------
{
	volatile bool boTerminateThread_;
public:
	explicit ThreadData() : boTerminateThread_(false) {}
	bool terminated(void) const
	{
		return boTerminateThread_;
	}
	void terminateThread(void)
	{
		boTerminateThread_ = true;
	}
};



#endif
