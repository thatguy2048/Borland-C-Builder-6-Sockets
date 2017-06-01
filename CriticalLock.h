#ifndef _CRITICAL_LOCK_H
#define _CRITICAL_LOCK_H

#include <windows>

//Class which provides the Entering and Exiting of a critical section.
//Leaves the critical section when the Lock goes out of scope, so it will be unlocked even if the scope leaves due to an exception.
class CriticalLock{
	protected:
		CRITICAL_SECTION * _cs;
	public:
		~CriticalLock(){	LeaveCriticalSection(_cs);	}
		CriticalLock(CRITICAL_SECTION * criticalSection):_cs(criticalSection){
			EnterCriticalSection(_cs);
		}
		
		void Lock(){	EnterCriticalSection(_cs);	}
		void Unlock(){	LeaveCriticalSection(_cs);	}
};


#define CRTLK(X)	CriticalLock _c_l_##X(&X)
#define INITLK(X)	InitializeCriticalSection(& X)
#define ENTRLK(X)	EnterCriticalSection(& X)
#define LEVLK(X)	LeaveCriticalSection(& X)
#define DELLK(X)	DeleteCriticalSection(& X) 

#endif //_CRITICAL_LOCK_H 