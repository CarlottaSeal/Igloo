#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"

#pragma pack(push, 1)

struct Vertex_PCU 
{
public:
    Vec3 m_position;
    Rgba8 m_color;
    Vec2 m_uvTextColors;
    
    Vertex_PCU() = default; 
    explicit Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTextColors);

    explicit Vertex_PCU(Vec2 const& position, Rgba8 const& color, Vec2 const& uvTextColors);
};

#pragma pack(pop)