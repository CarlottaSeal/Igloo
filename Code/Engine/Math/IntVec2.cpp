#include "IntVec2.hpp"
#include "MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <cstdlib>
#include <stdexcept>
#include <math.h>

constexpr float PI = 3.1415926535897932384626433832795f;

IntVec2::IntVec2(const IntVec2& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
}

IntVec2::IntVec2(int initialX, int initialY)
{
	x = initialX;
	y = initialY;
}

float IntVec2::GetLength() const
{
	return sqrtf(static_cast<float>(x * x + y * y));
}

int IntVec2::GetTaxicabLength() const
{
	return abs(x) + abs(y);
}

int IntVec2::GetLengthSquared() const
{
	return x * x + y * y;
}

float IntVec2::GetOrientationRadians() const
{
	return atan2f(static_cast<float>(y), static_cast<float>(x));
}

float IntVec2::GetOrientationDegrees() const
{
	return GetOrientationRadians() * (180.0f / PI);
}

IntVec2 const IntVec2::GetRotated90Degrees() const
{
	return IntVec2(-y, x);
}

IntVec2 const IntVec2::GetRotatedMinus90Degrees() const
{
	return IntVec2(y, -x);
}

void IntVec2::Rotate90Degrees()
{
	int oldX = x;
	x = -y;
	y = oldX;
}

void IntVec2::RotateMinus90Degrees()
{
	int oldX = x;
	x = y;
	y = -oldX;
}

void IntVec2::SetFromText(char const* text)
{
	if (!text) 
	{
		throw std::invalid_argument("Input text is null.");
	}

	Strings parts = SplitStringOnDelimiter(text, ',');
	if (parts.size() != 2) 
	{
		throw std::invalid_argument("IntVec2 requires exactly one comma in the input text.");
	}

	// Trim whitespace and convert to integers
	x = atoi(parts[0].c_str());
	y = atoi(parts[1].c_str());
}

void IntVec2::operator=(const IntVec2& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
}

IntVec2 IntVec2::operator+(const IntVec2& other) const
{
	return IntVec2(x + other.x, y + other.y);
}

void IntVec2::operator+=(const IntVec2& other)
{
	x += other.x;
	y += other.y;
}

IntVec2 IntVec2::operator-(const IntVec2& other) const
{
	return IntVec2(x - other.x, y - other.y);
}

bool IntVec2::operator==(const IntVec2& other) const
{
	return  x == other.x&&
			y == other.y;
}

bool IntVec2::operator!=(const IntVec2& other) const
{
	return  x != other.x ||
			y != other.y;
}

const IntVec2 IntVec2::ZERO = IntVec2(0, 0);
const IntVec2 IntVec2::NEGATIVEONE = IntVec2(-1, -1);