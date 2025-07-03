#include "OBB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <math.h>

OBB2::OBB2()
	: m_center(Vec2(0.0f, 0.0f))
	, m_iBasisNormal(Vec2(1.0f, 0.0f))
	, m_halfDimensions(Vec2(1.0f, 1.0f))
{
}

OBB2::OBB2(const Vec2& center, const Vec2& iBasisNormal, const Vec2& halfDimensions)
	: m_center(center)
	, m_iBasisNormal(iBasisNormal)
	, m_halfDimensions(halfDimensions)
{
	m_circumscribedRadius = m_halfDimensions.GetLength();
}

OBB2::OBB2(const Vec2& center, const Vec2& iBasisNormal, const Vec2& halfDimensions, const float elasticity)
	: m_center(center)
	, m_iBasisNormal(iBasisNormal)
	, m_halfDimensions(halfDimensions)
	, m_elasticity(elasticity)
{
	m_circumscribedRadius = m_halfDimensions.GetLength();
}

void OBB2::GetCornerPoints(Vec2* out_fourCornerWorldPositions) const
{
	// j
	Vec2 jBasisNormal(-m_iBasisNormal.y, m_iBasisNormal.x);

	out_fourCornerWorldPositions[0] = m_center + m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[1] = m_center - m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[2] = m_center - m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[3] = m_center + m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y;
}

Vec2 const OBB2::GetLocalPosForWorldPos(const Vec2& worldPos) const
{
	Vec2 displacement = worldPos - m_center;
	Vec2 jBasisNormal(-m_iBasisNormal.y, m_iBasisNormal.x);

	float localX = DotProduct2D(displacement, m_iBasisNormal);
	float localY = DotProduct2D(displacement, jBasisNormal);

	return Vec2(localX, localY);
}

Vec2 const OBB2::GetWorldPosForLocalPos(const Vec2& localPos) const
{
	Vec2 jBasisNormal(-m_iBasisNormal.y, m_iBasisNormal.x);
	return m_center + m_iBasisNormal * localPos.x + jBasisNormal * localPos.y;
}

void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
{
	float radians = rotationDeltaDegrees * (3.14159265f / 180.0f);
	float cosTheta = cosf(radians);
	float sinTheta = sinf(radians);

	// rotate iBasisNormal
	m_iBasisNormal = Vec2(
		m_iBasisNormal.x * cosTheta - m_iBasisNormal.y * sinTheta,
		m_iBasisNormal.x * sinTheta + m_iBasisNormal.y * cosTheta
	);
	m_iBasisNormal.Normalize();
}

float OBB2::GetCircumscribedRadius() const
{
	return m_circumscribedRadius;
}
