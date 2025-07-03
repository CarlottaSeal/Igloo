#pragma once
#include "Vec3.hpp"

struct OBB3
{
public:
    Vec3 m_center;
    Vec3 m_iBasisNormal;
    Vec3 m_jBasisNormal;
    Vec3 m_kBasisNormal;
    Vec3 m_halfDimensions;

public:
    OBB3();
    OBB3(const Vec3& center, const Vec3& iBasisNormal, const Vec3& jBasisNormal, const Vec3& kBasisNormal, const Vec3& halfDimensions);

    void GetCornerPoints(Vec3* out_eightCornerWorldPositions) const;

    Vec3 const GetLocalPosForWorldPos(const Vec3& worldPos) const;

    Vec3 const GetWorldPosForLocalPos(const Vec3& localPos) const;
    
    float GetCircumscribedRadiusSquared() const;
    float GetCircumscribedRadius() const;
};
