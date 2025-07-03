#pragma once

struct Vec4
{
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;

public:
	Vec4() {};
	~Vec4() {};
	explicit Vec4(float initialX, float initialY, float initialZ, float initialW);
	Vec4(Vec4 const& copyFrom);

};