#include "TRAPZ_Semaphore.h"


//max length of name: 20
TRAPZ_Semaphore::TRAPZ_Semaphore(int initial, int max,const char* name = "unnamed semaphore")
{
#ifdef _MSC_VER
	//initialize semaphore
	ghSemaphore = CreateSemaphoreA(
		NULL,           // default security attributes
		initial,				// initial count
		max,				// maximum count
		name);	// name of semaphore
#endif
}

TRAPZ_Semaphore::~TRAPZ_Semaphore()
{
#ifdef _MSC_VER
	CloseHandle(ghSemaphore);
#endif
}

//semaphore -1; waits if count is 0
void TRAPZ_Semaphore::enter()
{
#ifdef _MSC_VER
	//block semaphore
	while (WaitForSingleObject(
		ghSemaphore,   // handle to semaphore
		1L) != WAIT_OBJECT_0);      // 1-second time-out interval
#endif
}

//semaphore +1
void TRAPZ_Semaphore::leave()
{
#ifdef _MSC_VER
	if (!ReleaseSemaphore(
		ghSemaphore,  // handle to semaphore
		1,            // increase count by one
		NULL))       // not interested in previous count
	{
		printf("ReleaseSemaphore error: %d\n", GetLastError());
	}
#endif
}

