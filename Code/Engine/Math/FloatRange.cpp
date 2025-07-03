#include "FloatRange.hpp"
#include "Engine/Core/StringUtils.hpp"

const FloatRange FloatRange::ZERO = FloatRange(0.f, 0.f);
const FloatRange FloatRange::ONE = FloatRange(1.f, 1.f);
const FloatRange FloatRange::ZERO_TO_ONE = FloatRange(0.f, 1.f);

FloatRange::FloatRange()
	: m_min(0.0f), m_max(0.0f)
{

}

FloatRange::FloatRange(float min, float max)
	: m_min(min), m_max(max)
{
}

FloatRange::~FloatRange()
{
}

void FloatRange::SetFromText(char const* text)
{
	if (text == nullptr) return;

	std::string str = text;
	Strings parts = SplitStringOnDelimiter(str, '~');

	if (parts.size() == 1)
	{
		m_min = m_max = static_cast<float>(atof(parts[0].c_str()));
	}
	else if (parts.size() == 2)
	{
		m_min = static_cast<float>(atof(parts[0].c_str()));
		m_max = static_cast<float>(atof(parts[1].c_str()));
	}
}

FloatRange& FloatRange::operator=(const FloatRange& other)
{
	// TODO: insert return statement here
	if (this != &other) 
	{
		m_min = other.m_min;
		m_max = other.m_max;
	}
	return *this;
}

bool FloatRange::operator==(const FloatRange& other) const
{
	return (m_min == other.m_min) && (m_max == other.m_max);
}

bool FloatRange::operator!=(const FloatRange& other) const
{
	return !(*this == other);
}

bool FloatRange::IsInRange(float value) const
{
	return (value >= m_min) && (value <= m_max);
}

bool FloatRange::IsOverlappingWith(const FloatRange& other) const
{
	return (m_min <= other.m_max) && (m_max >= other.m_min);
}
