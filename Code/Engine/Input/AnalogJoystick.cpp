#include "AnalogJoystick.hpp"

Vec2 AnalogJoystick::GetPosition() const
{
    return m_correctedPosition;
}

float AnalogJoystick::GetMagnitude() const
{
    return m_correctedPosition.GetLength();
}

float AnalogJoystick::GetOrientationDegrees() const
{
    return m_correctedPosition.GetOrientationDegrees();
}

Vec2 AnalogJoystick::GetRawUncorrectedPosition() const
{
	return m_rawPosition;
}

float AnalogJoystick::GetInnerDeadZoneFraction() const
{
	return m_innerDeadZoneFraction;
}

float AnalogJoystick::GetOuterDeadZoneFraction() const
{
	return m_outerDeadZoneFraction;
}

void AnalogJoystick::Reset()
{
    m_rawPosition = Vec2(0.0f, 0.0f);
    m_correctedPosition = Vec2(0.0f, 0.0f);
    m_innerDeadZoneFraction = 0.30f;
    m_outerDeadZoneFraction = 0.95f;
}

void AnalogJoystick::SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold)
{
    m_innerDeadZoneFraction = normalizedInnerDeadzoneThreshold;
    m_outerDeadZoneFraction = normalizedOuterDeadzoneThreshold;
}

void AnalogJoystick::UpdatePosition(float rawNormalizedX, float rawNormalizedY)
{
    m_rawPosition = Vec2(rawNormalizedX, rawNormalizedY);
    // 计算摇杆的长度（模），用于检测是否在内/外死区内
    float magnitude = m_rawPosition.GetLength();

    if (magnitude < m_innerDeadZoneFraction) 
    {
        // 如果在内死区内，将修正后的位置设为 (0, 0)
        m_correctedPosition = Vec2(0.0f, 0.0f);
    }
    else if (magnitude > m_outerDeadZoneFraction) 
    {
        // 如果在外死区外，将修正后的位置限制为最大范围
        m_correctedPosition = m_rawPosition.GetNormalized();
    }
    else 
    {
        // 否则按比例缩放修正位置
        float scaledMagnitude = (magnitude - m_innerDeadZoneFraction) / (m_outerDeadZoneFraction - m_innerDeadZoneFraction);
        m_correctedPosition = m_rawPosition.GetNormalized() * scaledMagnitude;
    }
}
