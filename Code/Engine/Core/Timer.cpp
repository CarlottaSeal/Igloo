#include "Timer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Clock.hpp"

extern Clock* s_theSystemClock;

Timer::Timer(double period, const Clock* clock)
{
    if (clock)
    {
        m_clock = clock;
    }
    else
    {
        m_clock = s_theSystemClock;
    }
    m_period = period;
}

void Timer::Start()
{
    m_startTime = m_clock->GetTotalSeconds();
}

void Timer::Stop()
{
    m_startTime = -1.f;
}

float Timer::GetElapsedTime() const
{
    if (m_startTime == 0.f)
    {
        return 0.f;
    }
    else
    {
        return (float)(m_clock->GetTotalSeconds() - m_startTime);
    }
}

float Timer::GetElapsedFraction() const
{
    return (float)(GetElapsedTime()/m_period);
}

bool Timer::IsStopped() const
{
    if (m_startTime < 0.f)
        return true;
    else
        return false;
}

bool Timer::HasPeriodElapsed() const
{
    if (m_startTime != 0.f)
    {
        float elapsedTime = GetElapsedTime();
        if (elapsedTime > m_period)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

bool Timer::DecrementPeriodIfElapsed()
{
    if (HasPeriodElapsed() && !IsStopped())//&& m_startTime != 0.f)
    {
        //m_startTime += RoundDownToInt(GetElapsedFraction())*m_period;
        m_startTime += m_period;
        return true;
    }
    return false;
}
