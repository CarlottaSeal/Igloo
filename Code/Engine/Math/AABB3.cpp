#include "AABB3.hpp"
#include "Engine/Math/Vec3.hpp"

AABB3::AABB3(AABB3 const& copyFrom)
	:m_mins(copyFrom.m_mins)
	,m_maxs(copyFrom.m_maxs)
{
}

AABB3::AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
	:m_mins(Vec3(minX, minY, minZ))
	,m_maxs(Vec3(maxX, maxY, maxZ))
{
}

AABB3::AABB3(Vec3 const& mins, Vec3 const& maxs)
	:m_mins(mins)
	,m_maxs(maxs)
{
}
