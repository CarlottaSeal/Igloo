#include "EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include <math.h>
#include <cstdlib>
#include <stdexcept>
#include <math.h>

EulerAngles::EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees)
    :m_yawDegrees(yawDegrees)
    ,m_pitchDegrees(pitchDegrees)
    ,m_rollDegrees(rollDegrees)
{
}

void EulerAngles::GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis, Vec3& out_leftJBasis, Vec3& out_upKBasis) const
{
	Mat44 result = GetAsMatrix_IFwd_JLeft_KUp();

	out_forwardIBasis = Vec3(result.m_values[0], result.m_values[1], result.m_values[2]);
	out_leftJBasis = Vec3(result.m_values[4], result.m_values[5], result.m_values[6]);   
	out_upKBasis = Vec3(result.m_values[8], result.m_values[9], result.m_values[10]);    
}

Mat44 EulerAngles::GetAsMatrix_IFwd_JLeft_KUp() const
{
    Mat44 result;
    result.AppendZRotation(m_yawDegrees);
    result.AppendYRotation(m_pitchDegrees);
    result.AppendXRotation(m_rollDegrees);

    return result;
}

void EulerAngles::SetFromText(char const* text)
{
	if (!text)
	{
		throw std::invalid_argument("EulerAngles::SetFromText: Input text is null.");
	}

	Strings parts = SplitStringOnDelimiter(text, ',');
	if (parts.size() != 3)
	{
		throw std::invalid_argument("EulerAngles requires exactly one comma in the input text.");
	}

	try
	{
		m_yawDegrees = std::stof(parts[0]);
		m_pitchDegrees = std::stof(parts[1]);
		m_rollDegrees = std::stof(parts[2]);
	}
	catch (const std::exception&)
	{
		throw std::invalid_argument("EulerAngles::SetFromText: Failed to parse float values.");
	}
}

bool EulerAngles::operator==(const EulerAngles& other) const
{
	return m_yawDegrees == other.m_yawDegrees
		&& m_pitchDegrees == other.m_pitchDegrees
		&& m_rollDegrees == other.m_rollDegrees;
}

bool EulerAngles::operator!=(const EulerAngles& other) const
{
	return !(*this == other);
}