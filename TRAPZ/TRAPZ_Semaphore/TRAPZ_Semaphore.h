#ifndef __TRAPZ_SEMAPHORE
#define __TRAPZ_SEMAPHORE

#ifdef _MSC_VER

#pragma comment(lib, "ws2_32.lib") 

#include <stdio.h>
#include <Windows.h>

//! \brief Wrapper class for semaphores.
//!
//! The class currently only supports windows.
//! It uses windows api handles and waitforsingleobject() function.
//!
//! \author Noah Wiederhold
//! \version 03.2022
class TRAPZ_Semaphore
{
public:
	//! Default constructor.
	//! Creates semaphore handle.
	//! \param int initial value of the semaphore (=already used ressources)
	//! \param int max value of the semaphore (=max number of ressources)
	//! \param char* name of semaphore
	TRAPZ_Semaphore(int initial, int max,const char* name);

	//! Closes semaphore handle.
	~TRAPZ_Semaphore();

	//! Method enters semaphore and allocates one ressource.
	//! Method blocks calling thread if no ressources available.
	void enter();
	//! Method leaves semaphore and returns one ressource.
	void leave();

private:
	HANDLE ghSemaphore;									/**< this handle is used to store the semaphore */
};

#else
#error Onyl MS supported for this class currently.
#endif


#endif