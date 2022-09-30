#ifndef _TRAPZ_Timestamp
#define _TRAPZ_Timestamp

#include <string>
#include <chrono>

using std::string;


//! \brief Helper class for high precision timestampts.
//!
//! The class uses std::chrono::high_resolution_clock.
//! Its resolution is in nanoseconds range.
//! It represents duration from timepoint given to constructor or timepoint now to epoch.
//!
//! \author Noah Wiederhold
//! \version 03.2022
class TRAPZ_Timestamp
{
public:

	//! Default constructor.
	//! Creates timestamp.
	TRAPZ_Timestamp(void);


	//! Alternative constructor.
	//! Creates timestamp based on given time since epoch.
	//!
	//! \param timepoint Time since epoch.
	TRAPZ_Timestamp(unsigned long long timepoint);

	//! Default destructor.
	~TRAPZ_Timestamp(void);

	//! Method resets timepoint and saves new timepoint.
	void renew(void);

	//! Method resets timepoint to store a new timepoint.
	//!
	//! \param timepoint Time since epoch.
	void renew(unsigned long long timepoint);

	//! Getter method.
	//!
	//! \return The time as a number. (time since epoch)
	unsigned long long getTimeAsULongLong(void);

	//! Getter method.
	//!
	//! \return The time of the timestamp as a string.
	string getTimeAsString(void);

	//! Getter method.
	//! \return The time pf the timestamp as a formatted string.
	string getTimeAsFormattedString(void);

private:

	//--------------------------------------------------------------------------------------------------------------
	// Private data.
	//--------------------------------------------------------------------------------------------------------------

	unsigned long long mTime;													/**< The storage of the timepoint */
};

#endif //_TRAPZ_Timestamp