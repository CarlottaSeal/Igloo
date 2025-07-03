#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include <cstdlib>
#include <stdexcept>
#include <math.h>

constexpr float PI = 3.1415926535897932384626433832795f;

Vec3::Vec3(float initialX, float initialY, float initialZ)
    : x(initialX)
    , y(initialY)
    , z(initialZ)
{
}

Vec3::Vec3(Vec3 const& copy)
    : x(copy.x)
    , y(copy.y)
    , z(copy.z)
{
}

float Vec3::GetLength() const
{
    return sqrtf((x * x) + (y * y) + (z*z));
}

float Vec3::GetLengthXY() const
{
    return sqrtf((x * x) + (y * y));
}

float Vec3::GetLengthSquared() const
{
    return ((x * x) + (y * y) + (z * z));
}

float Vec3::GetLengthXYSquared() const
{
    return ((x * x) + (y * y));
}

float Vec3::GetAngleAboutZRadians() const
{
    return atan2f(y, x);
}

float Vec3::GetAngleAboutZDegrees() const
{
    float d = atan2f(y, x);
    return d * (180.f / PI);
}

Vec3 const Vec3::GetRotatedAboutZRadians(float deltaRadians) const
{
    //x'= x*cosTheta - y*sinTheta; y' = x*sinTheta + y*cosTheta
    float cosTheta = cosf(deltaRadians);
    float sinTheta = sinf(deltaRadians);

    float rotatedX = (x * cosTheta) - (y * sinTheta);
    float rotatedY = (x * sinTheta) + (y * cosTheta);

    return Vec3(rotatedX, rotatedY, z);
}

Vec3 const Vec3::GetRotatedAboutZDegrees(float deltaDegrees) const
{
    float Theta = deltaDegrees * (PI / 180.f);
    float rotatedX = (x * cosf(Theta)) - (y * sinf(Theta));
    float rotatedY = (x * sinf(Theta)) + (y * cosf(Theta));

    return Vec3(rotatedX, rotatedY, z);
}

Vec3 const Vec3::GetClamped(float maxLength) const
{
    float len = GetLength();
    if (len <= maxLength)
    {
        return Vec3(x, y , z);
    }

    float scale = maxLength / len;
    return Vec3(x * scale, y * scale, z * scale);
}

Vec3 const Vec3::GetNormalized() const
{
    float len = GetLength();
    if (len == 0.f)
    {
        return Vec3(x, y, z);
    }
    float scale = 1.f / len;
    float newx = x * scale;
    float newy = y * scale;
    float newz = z * scale;
    return Vec3(newx, newy, newz);
}


Vec2 const Vec3::GetXY() const
{
    return Vec2(x, y);
}

EulerAngles Vec3::GetEulerAnglesFromVec3() const
{
    EulerAngles result;

    result.m_yawDegrees = Atan2Degrees(y, x);
    
    float xyLength = sqrtf(x * x + y * y);
    result.m_pitchDegrees = -Atan2Degrees(z, xyLength); // 注意：向上为负角度（可按你引擎标准调整）

    result.m_rollDegrees = 0.f;

    return result;
}

bool Vec3::IsNearlyZero(float epsilon) const
{
    return (fabsf(x) < epsilon) && (fabsf(y) < epsilon
        && fabsf(z) < epsilon);
}

void Vec3::SetFromText(char const* text)
{
	if (!text)
	{
		throw std::invalid_argument("Vec3::SetFromText: Input text is null.");
	}

	Strings parts = SplitStringOnDelimiter(text, ',');
	if (parts.size() != 3)
	{
		throw std::invalid_argument("Vec3 requires exactly one comma in the input text.");
	}

	/*x = static_cast<float>(atof(parts[0].c_str()));
	y = static_cast<float>(atof(parts[1].c_str()));
	z = static_cast<float>(atof(parts[2].c_str()));*/
	try
	{
		x = std::stof(parts[0]); 
		y = std::stof(parts[1]);
		z = std::stof(parts[2]);
	}
	catch (const std::exception&)
	{
		throw std::invalid_argument("Vec3::SetFromText: Failed to parse float values.");
	}
}

Vec3 const Vec3::MakeFromPolarRadians(float latitudeRadians, float longitudeRadians, float length)
{
	float latitudeDegrees = ConvertRadiansToDegrees(latitudeRadians);
	float longtitudeDegrees = ConvertRadiansToDegrees(longitudeRadians);
	return MakeFromPolarDegrees(latitudeDegrees, longtitudeDegrees, length);
}

Vec3 const Vec3::MakeFromPolarDegrees(float latitudeDegrees, float longitudeDegrees, float length)
{
	float cosLatitude = CosDegrees(latitudeDegrees);
	float sinLatitude = SinDegrees(latitudeDegrees);
	float cosLongitude = CosDegrees(longitudeDegrees);
	float sinLongitude = SinDegrees(longitudeDegrees);
	float z = length * sinLatitude;
	float xy = length * cosLatitude;
	float x = xy * cosLongitude;
	float y = xy * sinLongitude;

	return Vec3(x, y, z);
}

Vec3 const Vec3::operator+(Vec3 const& vecToAdd) const
{
    return Vec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

Vec3 const Vec3::operator-(Vec3 const& vecToSubtract) const
{
    return Vec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

Vec3 const Vec3::operator-() const
{
    return Vec3(-x, -y, -z);
}

Vec3 const Vec3::operator*(float uniformScale) const
{
    return Vec3(x * uniformScale, y * uniformScale, z * uniformScale);
}

Vec3 const Vec3::operator*(Vec3 const& vecToMultiply) const
{
    return Vec3(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z);
}

Vec3 const Vec3::operator/(float inverseScale) const
{
    return Vec3(x / inverseScale, y / inverseScale, z / inverseScale);
}

void Vec3::operator+=(Vec3 const& vecToAdd)
{
    x += vecToAdd.x;
    y += vecToAdd.y;
    z += vecToAdd.z;
}

void Vec3::operator-=(Vec3 const& vecToSubtract)
{
    x -= vecToSubtract.x;
    y -= vecToSubtract.y;
    z -= vecToSubtract.z;
}

void Vec3::operator*=(float uniformScale)
{
    x *= uniformScale;
    y *= uniformScale;
    z *= uniformScale;
}

void Vec3::operator/=(float uniformDivisor)
{
    x /= uniformDivisor;
    y /= uniformDivisor;
    z /= uniformDivisor;
}

void Vec3::operator=(Vec3 const& copyFrom)
{
    x = copyFrom.x;
    y = copyFrom.y;
    z = copyFrom.z;
}

bool Vec3::operator==(Vec3 const& compare) const
{
    return (x == compare.x) && (y == compare.y) && (z == compare.z);
}

bool Vec3::operator!=(Vec3 const& compare) const
{
    return (x != compare.x) || (y != compare.y) || (z != compare.z);
}

Vec3 const operator*(float uniformScale, Vec3 const& vecToScale)
{
    return Vec3(uniformScale * vecToScale.x, uniformScale * vecToScale.y, uniformScale * vecToScale.z);
}