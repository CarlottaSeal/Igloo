#pragma once
#include "UIElement.h"

class Canvas;

class Panel : public UIElement
{
public:
    Panel(Canvas* canvas, AABB2 const& bounds, 
          Rgba8 backgroundColor = Rgba8::GREY,
          Texture* texture = nullptr,
          bool hasBorder = false,
          Rgba8 borderColor = Rgba8::BLACK);
    
    virtual ~Panel();

    // 生命周期（使用你的命名）
    void StartUp() override;
    void ShutDown() override;
    void Update() override;
    void Render() const override;

    // 设置方法
    void SetBounds(AABB2 const& bounds);
    void SetBackgroundColor(Rgba8 color);
    void SetTexture(Texture* texture);
    void SetBorder(bool hasBorder, Rgba8 borderColor = Rgba8::BLACK);

protected:
    Canvas* m_canvas = nullptr;
    
    Texture* m_texture = nullptr;
    bool m_hasBorder = false;
    Rgba8 m_backgroundColor = Rgba8::GREY;
    Rgba8 m_borderColor = Rgba8::BLACK;
    
    std::vector<Vertex_PCU> m_backgroundVerts;
    std::vector<Vertex_PCU> m_borderVerts;
};