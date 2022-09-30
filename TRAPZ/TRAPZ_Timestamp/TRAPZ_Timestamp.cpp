#include "TRAPZ_Timestamp.h"


TRAPZ_Timestamp::TRAPZ_Timestamp(void)
{
	auto time = std::chrono::high_resolution_clock::now();

	this->mTime = (time.time_since_epoch()).count();
}

TRAPZ_Timestamp::TRAPZ_Timestamp(unsigned long long timepoint)
{
	this->mTime = timepoint;
}

TRAPZ_Timestamp::~TRAPZ_Timestamp(void)
{
}

void TRAPZ_Timestamp::renew(void)
{
	auto time = std::chrono::high_resolution_clock::now();

	this->mTime = (time.time_since_epoch()).count();
}

void TRAPZ_Timestamp::renew(unsigned long long timepoint)
{
	this->mTime = timepoint;
}

//time since epoch in ns
unsigned long long TRAPZ_Timestamp::getTimeAsULongLong(void)
{
	return mTime;
}

string TRAPZ_Timestamp::getTimeAsString(void)
{
	return std::to_string(getTimeAsULongLong());
}

string TRAPZ_Timestamp::getTimeAsFormattedString(void)
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

	tmp = unformatted / 10000000;
	seconds = std::to_string(tmp);
	mal = 10000000;
	minus = static_cast<unsigned long long>(tmp) * mal;
	unformatted = unformatted - minus;


	tmp = unformatted / 10000;
	milliseconds = std::to_string(tmp);
	mal = 10000;
	minus = static_cast<unsigned long long>(tmp) * mal;
	unformatted = unformatted - minus;

	tmp = unformatted / 10;
	microseconds = std::to_string(tmp);
	mal = 10;
	minus = static_cast<unsigned long long>(tmp) * mal;
	unformatted = unformatted - minus;

	nanoseconds = std::to_string(unformatted);

	string result = "s: " + seconds + " ms: " + milliseconds + " ys: " + microseconds + " ns: " + nanoseconds;

	return result;
}
