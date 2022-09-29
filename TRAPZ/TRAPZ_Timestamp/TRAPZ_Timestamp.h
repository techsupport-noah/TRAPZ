#ifndef _TRAPZ_Timestamp
#define _TRAPZ_Timestamp

#include <string>
#include <chrono>

using std::string;


//! \brief Helper class for high precision timestampts.
//!
//! The class uses std::chrono::high_resolution_clock.
//! Its resolution is in nanoseconds range.
//!
//! \author Noah Wiederhold
//! \version 03.2022
class TRAPZ_Timestamp
{
public:

	//! Default constructor.
	//! Creates timestamp.
	TRAPZ_Timestamp();
	~TRAPZ_Timestamp();

	//! Method returns time from stored timestamp.
	//! \return unsigned long long time as number
	unsigned long long getTimeAsULongLong();

	//! Method returns time from stored timestamp.
	//1 \return const char* time as char*
	const char* getTimeAsChar();

	//! Method returns time from stored timestamp.
	//1 \return string time as string
	string getTimeAsString();

	//! Method returns formatted time from stored timestamp.
	//1 \return string time as formatted string
	string getTimeAsFormattedString();

private:
	std::chrono::high_resolution_clock::time_point mTime;								/**< this var stores the timepoint */
};


#endif