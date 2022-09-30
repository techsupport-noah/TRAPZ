#ifndef __TRAPZ_SEMAPHORE
#define __TRAPZ_SEMAPHORE

#ifdef _MSC_VER

#include <stdio.h>
#include <Windows.h>

#else
#error Onyl MS supported for this class currently.
#endif

//! \brief Wrapper class for semaphores.
//!
//! The class currently only supports windows.
//! It uses windows api handles and waitforsingleobject() function to block the thread while waiting.
//!
//! \author Noah Wiederhold
//! \version 07.2022
class TRAPZ_Semaphore
{
public:
	//! Default constructor.
	//! Creates semaphore handle.
	//!
	//! \param	 initial		 The value of the semaphore. (=already used ressources)
	//! \param	 max			 The value of the semaphore. (=max number of ressources)
	//! \param	 name			 The name of semaphore.	(max of 20 characters)
	TRAPZ_Semaphore(int initial, int max, const char* name = "unnamed semaphore");

	//! Default destructor.
	//! Closes semaphore handle.
	~TRAPZ_Semaphore(void);

	//! Method enters semaphore and allocates one ressource.
	//! Blocks calling thread if no ressources available.
	void enter(void);

	//! Method leaves semaphore and returns one ressource.
	void leave(void);

private:

	//--------------------------------------------------------------------------------------------------------------
	// Private data.
	//--------------------------------------------------------------------------------------------------------------
	HANDLE ghSemaphore;									/**< this handle is used to store the semaphore */

#ifdef _DEBUG
	bool locked = false;	//field to check semaphore state, only for debugging
#endif

};


#endif