#pragma once
#include "Disc2D.h"
#include "Engine/Math/Vec2.hpp"

struct OBB2
{
public:
	Vec2 m_center;          
	Vec2 m_iBasisNormal;
	Vec2 m_halfDimensions;

	float m_elasticity = 0.f;
	float m_circumscribedRadius = 0.f;

public:
	OBB2();
	OBB2(const Vec2& center, const Vec2& iBasisNormal, const Vec2& halfDimensions);
	OBB2(const Vec2& center, const Vec2& iBasisNormal, const Vec2& halfDimensions, const float elasticity);

	void GetCornerPoints(Vec2* out_fourCornerWorldPositions) const;

	Vec2 const GetLocalPosForWorldPos(const Vec2& worldPos) const;

	Vec2 const GetWorldPosForLocalPos(const Vec2& localPos) const;

	void RotateAboutCenter(float rotationDeltaDegrees);

	float GetCircumscribedRadius() const;
};