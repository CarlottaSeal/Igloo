#include "Sprite.h"
#include "Canvas.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"

Sprite::Sprite(Canvas* canvas, AABB2 const& bounds, Texture* texture)
    : m_canvas(canvas)
    , m_texture(texture)
{
    m_bound = bounds;
    m_type = SPRITE;
    
    if (m_canvas)
    {
        m_canvas->AddElementToCanvas(this);
    }
    
    StartUp();
}

Sprite::~Sprite()
{
    ShutDown();
}

void Sprite::StartUp()
{
    InitializeVerts();
}

void Sprite::ShutDown()
{
    m_verts.clear();
    
    for (auto* child : m_children)
    {
        if (child)
        {
            child->ShutDown();
            delete child;
        }
    }
    m_children.clear();
}

void Sprite::Update()
{
    if (!IsEnabled())
    {
        return;
    }
    
    for (auto* child : m_children)
    {
        if (child)
        {
            child->Update();
        }
    }
}

void Sprite::Render() const
{
    if (!IsEnabled())
    {
        return;
    }
    
    if (!m_canvas)
    {
        return;
    }
    
    if (m_verts.empty())
    {
        return;
    }
    
    Renderer* renderer = m_canvas->GetSystemRenderer();
    Camera* camera = m_canvas->GetCamera();
    
    renderer->BeginCamera(*camera);
    renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
    renderer->SetBlendMode(BlendMode::ALPHA);
    
    renderer->BindTexture(m_texture);
    renderer->DrawVertexArray((int)m_verts.size(), m_verts.data());
    
    for (auto* child : m_children)
    {
        if (child)
        {
            child->Render();
        }
    }
    
    renderer->EndCamera(*camera);
}

void Sprite::SetTexture(Texture* texture)
{
    m_texture = texture;
}

void Sprite::SetBounds(AABB2 const& bounds)
{
    m_bound = bounds;
    InitializeVerts();
}

void Sprite::SetColor(Rgba8 color)
{
    if (m_originalColor != color)
    {
        m_originalColor = color;
        m_renderedColor = color;
        InitializeVerts();
    }
}