#include "Engine/Core/Vertex_PCU.hpp"

Vertex_PCU::Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTextColors)
    : m_position(position)
    , m_color(color)
    , m_uvTextColors(uvTextColors)
{

}

Vertex_PCU::Vertex_PCU(Vec2 const& position, Rgba8 const& color, Vec2 const& uvTextColors)
    : m_position(Vec3(position.x, position.y, 0.0f))
    , m_color(color)
    , m_uvTextColors(uvTextColors)
{
}
