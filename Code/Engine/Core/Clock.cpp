#include "Clock.hpp"
#include "Engine/Core/Time.hpp"

Clock* s_theSystemClock = new Clock();

Clock::Clock()
    //:m_parent(&g_theSystemClock)
{
    if (s_theSystemClock)
    {
        m_parent = s_theSystemClock;
        s_theSystemClock->m_children.push_back(this);
    }
    m_lastUpdateTimeInSeconds = GetCurrentTimeSeconds();
    /*for (Clock* child : m_children)
    {
        child = nullptr;
    }
    g_theSystemClock.AddChild(this);*/
}

Clock::~Clock()
{
    for (Clock* child : m_children)
    {
        if (child)
        {
            child->m_parent = nullptr;
        }        
    }
    if (m_parent)
    {
        m_parent->RemoveChild(this);
    }  
}

Clock::Clock(Clock& parent)
    //:m_parent(&parent)
{
    /*for (Clock* child : m_children)
    {
        child = nullptr;
    }*/
    //m_parent->AddChild(this);
    parent.AddChild(this);
	m_lastUpdateTimeInSeconds = GetCurrentTimeSeconds();
}

void Clock::Reset()
{
	m_lastUpdateTimeInSeconds = 0.f;
	m_totalSeconds = 0.f;
	m_deltaSeconds = 0.f;
	m_frameCount = 0;
    m_lastUpdateTimeInSeconds = s_theSystemClock->GetTotalSeconds();
}

bool Clock::IsPaused() const
{
    return m_isPaused;
}

void Clock::Pause()
{
	m_isPaused = true;
}

void Clock::Unpause()
{
    m_isPaused = false;  
}

void Clock::TogglePause()
{
    m_isPaused = !m_isPaused;
}

void Clock::StepSingleFrame()
{
	m_stepSingleFrame = true;
	m_isPaused = false;
}

void Clock::SetTimeScale(float timeScale)
{
    m_timeScale = (double)timeScale;
}

float Clock::GetTimeScale() const
{
    return (float)m_timeScale;
}

double Clock::GetDeltaSeconds() const
{
    return m_deltaSeconds;
}

double Clock::GetTotalSeconds() const
{
    return m_totalSeconds;
}

double Clock::GetFrameRate() const
{
    if (m_deltaSeconds != 0.f)
    {
        return 1.f / m_deltaSeconds;
    }

    return 0.0f;
}

int Clock::GetFrameCount() const
{
    return m_frameCount;
}

Clock& Clock::GetSystemClock()
{
    return *s_theSystemClock;
}

void Clock::TickSystemClock()
{
    s_theSystemClock->Tick();
}

void Clock::Tick()
{
	float currentTime = static_cast<float> (GetCurrentTimeSeconds());
	float deltaSeconds = (float)(currentTime - m_lastUpdateTimeInSeconds);

	if (deltaSeconds > m_maxDeltaSeconds)
	{
		deltaSeconds = m_maxDeltaSeconds;
	}

	m_lastUpdateTimeInSeconds = currentTime;
	Advance(deltaSeconds);
}

void Clock::Advance(double deltaTimeSeconds)
{
    if (m_isPaused)
    {
        deltaTimeSeconds = 0.f;
    }

    m_deltaSeconds = deltaTimeSeconds * m_timeScale;
    m_totalSeconds += m_deltaSeconds;
    m_frameCount++;

	if (m_stepSingleFrame)  // if stepping single frame, pause after this.
	{
		m_stepSingleFrame = false;
		m_isPaused = true;
		return; //avoid children's advance()
	}

    for (Clock* child : m_children)
    {
        if (child)
        {
            child->Advance(m_deltaSeconds);
        }       
    }
}

void Clock::AddChild(Clock* childClock)
{
    if(!childClock->m_parent)
    {
        m_children.push_back(childClock);
        childClock->m_parent = this;
    }
}

void Clock::RemoveChild(Clock* childClock)
{
    if (childClock && childClock->m_parent == this)
    {
		auto it = std::find(m_children.begin(), m_children.end(), childClock);
		if (it != m_children.end())
		{
			m_children.erase(it);
			childClock->m_parent = nullptr;
		}
    }
}
