#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <cstdlib>
#include <stdexcept>
#include <math.h>

constexpr float PI = 3.1415926535897932384626433832795f;
float x;
float y;

const Vec2 Vec2::ONE = Vec2(1.0f, 1.0f);
const Vec2 Vec2::ZERO = Vec2(0.0f, 0.0f);

//-----------------------------------------------------------------------------------------------
Vec2::Vec2( Vec2 const& copy )
	: x( copy.x )
	, y( copy.y )
{

}


//-----------------------------------------------------------------------------------------------
Vec2::Vec2( float initialX, float initialY )
	: x( initialX )
	, y( initialY )
{
}


//Static methods (e.g. creation functions)
Vec2 const Vec2::MakeFromPolarRadians(float orientationRadians
									   ,float length)
{
	float newx = length * cosf(orientationRadians);
	float newy = length * sinf(orientationRadians);

	return Vec2(newx, newy);
}

Vec2 const Vec2::MakeFromPolarDegrees(float orientationDegrees
									   ,float length)
{
	float orientationRadians = orientationDegrees * ( PI / 180.f ); 
	float newx = length * cosf(orientationRadians);
	float newy = length * sinf(orientationRadians);
	
	return Vec2(newx, newy);
}


//Accessors(const methods)
float Vec2::GetLength() const
{
	return sqrtf((x * x) + (y * y));
}

float Vec2::GetLengthSquared() const
{
	return (x * x) + (y * y);
}

float Vec2::GetOrientationRadians() const
{
	return atan2f(y, x);
}

float Vec2::GetOrientationDegrees() const
{
	float d = atan2f(y, x);
	return d * (180.f / PI);

}

Vec2 const Vec2::GetRotated90Degrees() const
{
	float oldx = x;
	float newx = -y;
	float newy = oldx;
	return Vec2(newx, newy);
}

Vec2 const Vec2::GetRotatedMinus90Degrees() const
{
	float oldx = x;
	float newx = y;
	float newy = -oldx;
	return Vec2(newx, newy);
}

Vec2 const Vec2::GetRotatedRadians(float deltaRadians) const
{
	//x'= x*cosTheta - y*sinTheta; y' = x*sinTheta + y*cosTheta
	float cosTheta = cosf(deltaRadians);
	float sinTheta = sinf(deltaRadians);

	float rotatedX = (x * cosTheta) - (y * sinTheta);
	float rotatedY = (x * sinTheta) + (y * cosTheta);

	return Vec2(rotatedX, rotatedY);
}

Vec2 const Vec2::GetRotatedDegrees(float deltaDegrees) const
{
	float Theta = deltaDegrees * ( PI / 180.f );
	float rotatedX = (x * cosf(Theta)) - (y * sinf(Theta));
	float rotatedY = (x * sinf(Theta)) + (y * cosf(Theta));

	return Vec2(rotatedX, rotatedY);
}

Vec2 const Vec2::GetClamped(float maxLength) const
{
	float len = GetLength();
	if (len <= maxLength)
	{
		return Vec2(x,y);
	}

	float scale = maxLength / len;
	return Vec2( x * scale, y * scale );
}

Vec2 const Vec2::GetNormalized() const
{
	float len = GetLength();
	if(len == 0.f)
	{
		return Vec2(x,y);
	}
	float scale = 1.f / len;
	float newx = x * scale;
	float newy = y * scale;
	return Vec2(newx , newy);
	
}

//Mutators (non-const methods)
void Vec2::SetOrientationRadians(float newOrientationRadians)
{
	float len = GetLength();
	x = len * cosf(newOrientationRadians);
	y = len * sinf(newOrientationRadians);
}

void Vec2::SetOrientationDegrees(float newOrientationDegrees)
{
	float radians = ConvertDegreesToRadians(newOrientationDegrees);
	float len = GetLength();
	x = len * cosf(radians);
	y = len * sinf(radians);
}

void Vec2::SetPolarRadians(float newOrientationRadians, float newlength)
{
	x = newlength * cosf(newOrientationRadians);
	y = newlength * sinf(newOrientationRadians);
}
void Vec2::SetPolarDegrees(float newOrientationDegrees, float newlength)
{
	float radians = ConvertDegreesToRadians(newOrientationDegrees);
	x = newlength * cosf(radians);
	y = newlength * sinf(radians);
}

void Vec2::Rotate90Degrees()
{
	float oldx = x;
	x = -y;
	y = oldx;
}

void Vec2::RotateMinus90Degrees()
{
	float oldx = x;
	x = y;
	y = -oldx;
}

void Vec2::RotateRadians(float deltaRadians)
{
	float cosTheta = cosf(deltaRadians);
	float sinTheta = sinf(deltaRadians);

	float oldX = x;
	float oldY = y;

	x = (oldX * cosTheta) - (oldY * sinTheta);
	y = (oldX * sinTheta) + (oldY * cosTheta);
}

void Vec2::RotateDegrees(float deltaDegrees)
{
	float Theta = deltaDegrees * ( PI / 180.f);

	float oldX = x;
	float oldY = y;

	x = (oldX * cosf(Theta)) - (oldY * sinf(Theta));
	y = (oldX * sinf(Theta)) + (oldY * cosf(Theta));
}

void Vec2::SetLength(float newLength)
{
	Normalize();
	x *= newLength;
	y *= newLength;
}

void Vec2::ClampLength(float maxLength)
{
	float len = GetLength();
	if (len <= maxLength)
	{
		return;
	}

	float scale = maxLength / len;
	x *= scale;
	y *= scale;
}

void Vec2::Normalize()
{
	float len = GetLength();
	if (len == 0.f)
	{
		return;
	}
	float scale = 1.f / len;
	x *= scale;
	y *= scale;
}

float Vec2::NormalizeAndGetPreviousLength()
{
	float length = GetLength(); 

	if (length != 0.f) 
	{
		x /= length;  
		y /= length;  
	}

	return length;
}

Vec2 const Vec2::GetReflected(Vec2 const& normalOfSurfaceToReflectOffOf) const
{
	float dotProduct = DotProduct2D(*this, normalOfSurfaceToReflectOffOf);

	Vec2 reflected = *this - (2.0f * dotProduct * normalOfSurfaceToReflectOffOf);

	return reflected;
}


void Vec2::Reflect(Vec2 const& normalOfSurfaceToReflectOffOf)
{
	float dotProduct = DotProduct2D(*this, normalOfSurfaceToReflectOffOf);

	*this = *this - (2.0f * dotProduct * normalOfSurfaceToReflectOffOf);
}

void Vec2::SetFromText(char const* text)
{
	if (!text) 
	{
		throw std::invalid_argument("Vec2::SetFromText: Input text is null.");
	}

	Strings parts = SplitStringOnDelimiter(text, ',');
	if (parts.size() != 2) 
	{
		throw std::invalid_argument("Vec2 requires exactly one comma in the input text.");
	}

	// Trim whitespace and convert to floats
	x = static_cast<float>(atof(parts[0].c_str()));
	y = static_cast<float>(atof(parts[1].c_str()));
}

//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator + ( Vec2 const& vecToAdd ) const
{
	return Vec2( x + vecToAdd.x , y + vecToAdd.y );
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator-( Vec2 const& vecToSubtract ) const
{
	return Vec2( x - vecToSubtract.x , y - vecToSubtract.y );
}


//------------------------------------------------------------------------------------------------
Vec2 const Vec2::operator-() const
{
	return Vec2(-x ,-y );
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator*( float uniformScale ) const
{
	return Vec2( x * uniformScale , y * uniformScale );
}


//------------------------------------------------------------------------------------------------
Vec2 const Vec2::operator*( Vec2 const& vecToMultiply ) const
{
	return Vec2( x * vecToMultiply.x , y * vecToMultiply.y );
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator/( float inverseScale ) const
{
	return Vec2( x / inverseScale , y / inverseScale );
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator+=( Vec2 const& vecToAdd )
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	return;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator-=( Vec2 const& vecToSubtract )
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	return;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator*=( const float uniformScale )
{
	x *= uniformScale;
	y *= uniformScale;
	return;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator/=( const float uniformDivisor )
{
	x /= uniformDivisor;
	y /= uniformDivisor;
	return;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator=( Vec2 const& copyFrom )
{
	x = copyFrom.x;
	y = copyFrom.y;
	return;
}


//-----------------------------------------------------------------------------------------------
Vec2 const operator*( float uniformScale, Vec2 const& vecToScale )
{
	return Vec2(uniformScale * vecToScale.x , uniformScale * vecToScale.y );
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator==( Vec2 const& compare ) const
{
	return (x == compare.x) and (y == compare.y);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator!=( Vec2 const& compare ) const
{
	return (x != compare.x) or (y != compare.y);
}

