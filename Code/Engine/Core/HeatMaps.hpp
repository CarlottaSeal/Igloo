#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

#include <vector>

class TileHeatMap
{
public:
	TileHeatMap(IntVec2 const& dimensions);

	void AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, FloatRange valueRange = FloatRange(0.f, 100.f), 
		Rgba8 lowColor = Rgba8(0, 0, 0, 255), Rgba8 highColor = Rgba8(255, 255, 255, 255), float specialValue = 999999.f, Rgba8 specialColor = Rgba8(255, 0, 255));
	
	void SetAllValues(float defaultValue);
	float GetValueAtCoords(IntVec2 const& coords) const;
	void SetValueAtCoords(IntVec2 const& coords, float value);
	void AddValueAtCoords(IntVec2 const& coords, float value);

public:
	IntVec2 m_dimensions = IntVec2::ZERO;
	std::vector<float> m_values; //store "heat" float values
};