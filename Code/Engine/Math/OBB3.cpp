#include "OBB3.h"

#include "EulerAngles.hpp"
#include "MathUtils.hpp"

OBB3::OBB3()
{
}

OBB3::OBB3(const Vec3& center, const Vec3& iBasisNormal, const Vec3& jBasisNormal, const Vec3& kBasisNormal,
           const Vec3& halfDimensions)
        : m_center(center)
        , m_iBasisNormal(iBasisNormal)
        , m_jBasisNormal(jBasisNormal)
        , m_kBasisNormal(kBasisNormal)
        , m_halfDimensions(halfDimensions)
{
}

void OBB3::GetCornerPoints(Vec3* out_eightCornerWorldPositions) const
{
    Vec3 i = m_iBasisNormal * m_halfDimensions.x;
    Vec3 j = m_jBasisNormal * m_halfDimensions.y;
    Vec3 k = m_kBasisNormal * m_halfDimensions.z;
    
    out_eightCornerWorldPositions[0] = m_center - i - j - k; 
    out_eightCornerWorldPositions[1] = m_center - i - j + k; 
    out_eightCornerWorldPositions[2] = m_center - i + j - k; 
    out_eightCornerWorldPositions[3] = m_center - i + j + k; 
    out_eightCornerWorldPositions[4] = m_center + i - j - k; 
    out_eightCornerWorldPositions[5] = m_center + i - j + k; 
    out_eightCornerWorldPositions[6] = m_center + i + j - k; 
    out_eightCornerWorldPositions[7] = m_center + i + j + k; 
}

Vec3 const OBB3::GetLocalPosForWorldPos(const Vec3& worldPos) const
{
    Vec3 toWorld = worldPos - m_center;
    float x = DotProduct3D(toWorld, m_iBasisNormal);
    float y = DotProduct3D(toWorld, m_jBasisNormal);
    float z = DotProduct3D(toWorld, m_kBasisNormal);
    return Vec3(x, y, z);
}

Vec3 const OBB3::GetWorldPosForLocalPos(const Vec3& localPos) const
{
    return m_center
        + localPos.x * m_iBasisNormal
        + localPos.y * m_jBasisNormal
        + localPos.z * m_kBasisNormal;
}

float OBB3::GetCircumscribedRadiusSquared() const
{
    return m_halfDimensions.GetLengthSquared();
}

float OBB3::GetCircumscribedRadius() const
{
    return m_halfDimensions.GetLength();
}
