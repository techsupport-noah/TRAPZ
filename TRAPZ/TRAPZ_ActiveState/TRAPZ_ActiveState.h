#ifndef _TRAPZ_ACTIVESTATE
#define _TRAPZ_ACTIVESTATE

#include "../TRAPZ_Timestamp/TRAPZ_Timestamp.h"

//! \brief Helper class to check for active connections.
//!
//! This class uses TRAPZ_Timestamps to check for the last notified interaction.
//!
//! \author Noah Wiederhold
//! \version 03.2022
class TRAPZ_ActiveState
{
public:
	//! Default constructor.
	//! Initializes State with a limit of 5 seconds.
	TRAPZ_ActiveState();

	//! Second constructor gives option to set the minimum of time being inactive for state to turn false.
	//! \param limit time in nanoseconds
	TRAPZ_ActiveState(long long limit);

	//! Default destructor.
	~TRAPZ_ActiveState();

	//! Method return initialized state.
	//! \return bool initialized state
	bool isInitialized();

	//! Method checks if state is still actively used.
	//! \return bool active state
	bool isActive();

	//! Method to register last seen TRAPZ_Timestamp.
	void tick();
	
	//! Method to register the connection.
	void setInitialized(bool val);

private:
	unsigned long long m_llLimit;

	bool m_bInitialized=false;

	TRAPZ_Timestamp* m_old;
	TRAPZ_Timestamp* m_new;
};

#endif
