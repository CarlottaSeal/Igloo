#include "Vec4.hpp"
#pragma once

Vec4::Vec4(float initialX, float initialY, float initialZ, float initialW)
	: x(initialX)
	, y(initialY)
	, z(initialZ)
	, w(initialW)
{
}

Vec4::Vec4(Vec4 const& copyFrom)
	: x(copyFrom.x)
	, y(copyFrom.y)
	, z(copyFrom.z)
	, w(copyFrom.w)
{
}

Vec4 Vec4::operator-(const Vec4& other) const
{
	return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
}

Vec4& Vec4::operator*=(float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	w *= scalar;
	return *this;
}

Vec4& Vec4::operator+=(const Vec4& other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	w += other.w;
	return *this;
}
