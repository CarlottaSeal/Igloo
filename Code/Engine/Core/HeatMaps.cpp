#include "HeatMaps.hpp"

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"

TileHeatMap::TileHeatMap(IntVec2 const& dimensions)
	:m_dimensions(dimensions)
{
	m_values.resize(m_dimensions.x * m_dimensions.y, 0.f);
}

void TileHeatMap::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, 
	FloatRange valueRange, Rgba8 lowColor, Rgba8 highColor, float specialValue, Rgba8 specialColor)
{
	float tileWidth = (totalBounds.m_maxs.x - totalBounds.m_mins.x) / m_dimensions.x;
	float tileHeight = (totalBounds.m_maxs.y - totalBounds.m_mins.y) / m_dimensions.y;
	for (int y = 0; y < m_dimensions.y; ++y)
	{
		for (int x = 0; x < m_dimensions.x; ++x)
		{
			Vec2 mins = totalBounds.m_mins + Vec2(x * tileWidth, y * tileHeight);
			Vec2 maxs = mins + Vec2(tileWidth, tileHeight);
			AABB2 tileBounds = AABB2(mins, maxs);
			
			IntVec2 coords(x, y);
			float value = GetValueAtCoords(coords);

			Rgba8 drawColor;
			if (value == specialValue) 
			{
				drawColor = specialColor;
			}
			else 
			{
				float drawFraction = RangeMapClamped(value, valueRange.m_min, valueRange.m_max, 0.0f, 1.0f);
				drawColor = InterpolateRgba8(lowColor, highColor, drawFraction);
			}

			verts.push_back(Vertex_PCU(Vec3(mins.x, mins.y, 0.f), drawColor, Vec2(0.f, 0.f)));
			verts.push_back(Vertex_PCU(Vec3(maxs.x, mins.y, 0.f), drawColor, Vec2(1.f, 0.f)));
			verts.push_back(Vertex_PCU(Vec3(maxs.x, maxs.y, 0.f), drawColor, Vec2(1.f, 1.f)));
			
			verts.push_back(Vertex_PCU(Vec3(mins.x, mins.y, 0.f), drawColor, Vec2(0.f, 0.f)));
			verts.push_back(Vertex_PCU(Vec3(maxs.x, maxs.y, 0.f), drawColor, Vec2(1.f, 1.f)));
			verts.push_back(Vertex_PCU(Vec3(mins.x, maxs.y, 0.f), drawColor, Vec2(0.f, 1.f)));
		}
	}
}

void TileHeatMap::SetAllValues(float defaultValue)
{
	for (size_t i = 0; i < m_values.size(); ++i) 
	{
		m_values[i] = defaultValue;
	}
}

float TileHeatMap::GetValueAtCoords(IntVec2 const& coords) const
{
	int index = coords.y * m_dimensions.x + coords.x;
	return m_values[index];
}

void TileHeatMap::SetValueAtCoords(IntVec2 const& coords, float value)
{
	int index = coords.y * m_dimensions.x + coords.x;
	m_values[index] = value;
}

void TileHeatMap::AddValueAtCoords(IntVec2 const& coords, float value)
{
	int index = coords.y * m_dimensions.x + coords.x;
	m_values[index] += value;
}
