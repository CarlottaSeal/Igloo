#pragma once
#include "Engine/Math/Vec2.hpp"

class AnalogJoystick
{
public:
	Vec2 GetPosition() const;
	float GetMagnitude() const;
	float GetOrientationDegrees() const;

	Vec2 GetRawUncorrectedPosition() const;
	float GetInnerDeadZoneFraction() const;
	float GetOuterDeadZoneFraction() const;

	//for use by xboxcontroller
	void Reset();
	void SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, 
							   float normalizedOuterDeadzoneThreshold);
	void UpdatePosition(float rawNormalizedX, float rawNormalizedY);

	float GetNormalizedX() const { return m_correctedPosition.x; }
	float GetNormalizedY() const { return m_correctedPosition.y; }

protected:
	Vec2 m_rawPosition;					//flaky; doesn't rest at zero(or consistently snap to rest position)
	Vec2 m_correctedPosition;			//deadzone-corrected position
	float m_innerDeadZoneFraction = 0.30f;		//if R<this%; "input range start" for corrective range map
	float m_outerDeadZoneFraction = 0.95f;		//if R> this%; "input range end" for corrective range map
};