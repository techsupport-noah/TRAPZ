#include "TRAPZ_Timestamp.h"


TRAPZ_Timestamp::TRAPZ_Timestamp() 
{
	//mTime = __rdtsc();
	mTime = std::chrono::high_resolution_clock::time_point();
	mTime = std::chrono::high_resolution_clock::now();
}

TRAPZ_Timestamp::~TRAPZ_Timestamp()
{
}

//time since epoch in ns
unsigned long long TRAPZ_Timestamp::getTimeAsULongLong()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(mTime.time_since_epoch()).count();
}

const char* TRAPZ_Timestamp::getTimeAsChar()
{
	return getTimeAsString().c_str();
}

string TRAPZ_Timestamp::getTimeAsString()
{
	return std::to_string(getTimeAsULongLong());
}

string TRAPZ_Timestamp::getTimeAsFormattedString()
{
	string nanoseconds;
	string microseconds;
	string milliseconds;
	string seconds;

	//in nanoseconds
	unsigned long long unformatted = getTimeAsULongLong();
	
	int tmp;
	unsigned long long mal;
	unsigned long long minus;

	tmp = unformatted / 1000000000;
	seconds = std::to_string(tmp);
	mal = 1000000000;
	minus = static_cast<unsigned long long>(tmp) * mal;
	unformatted = unformatted - minus;


	tmp = unformatted / 1000000;
	milliseconds = std::to_string(tmp);
	mal = 1000000;
	minus = static_cast<unsigned long long>(tmp)* mal;
	unformatted = unformatted - minus;

	tmp = unformatted / 1000;
	microseconds = std::to_string(tmp);
	mal = 1000;
	minus = static_cast<unsigned long long>(tmp)* mal;
	unformatted = unformatted - minus;

	nanoseconds = std::to_string(unformatted);

	return "s: " + seconds + " ms: " + milliseconds + " ys: " + microseconds + " ns: " + nanoseconds;
}
