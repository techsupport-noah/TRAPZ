#include "TRAPZ_ActiveState.h"


TRAPZ_ActiveState::TRAPZ_ActiveState()
{
	m_llLimit = 5000000000;
	m_old = new TRAPZ_Timestamp();
	m_new = new TRAPZ_Timestamp();
}

TRAPZ_ActiveState::TRAPZ_ActiveState(long long limit) :
m_llLimit(limit)
{
	m_old = new TRAPZ_Timestamp();
	m_new = new TRAPZ_Timestamp();
}

TRAPZ_ActiveState::~TRAPZ_ActiveState()
{
}

bool TRAPZ_ActiveState::isInitialized()
{
	return m_bInitialized;
}

bool TRAPZ_ActiveState::isActive()
{
	if (m_bInitialized){
		tick();
		if ((m_new->getTimeAsULongLong() - m_old->getTimeAsULongLong()) > m_llLimit)
		{
			return false;
		}
		else{
			return true;
		}
	}
	return false;
}

void TRAPZ_ActiveState::tick()
{
	m_old = m_new;
	m_new = new TRAPZ_Timestamp();
}

void TRAPZ_ActiveState::setInitialized(bool val)
{
	m_bInitialized = val;
	tick();
}
