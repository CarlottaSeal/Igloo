#pragma once
#include "UIElement.h"

class Canvas;
class Texture;

class Sprite : public UIElement
{
public:
    Sprite(Canvas* canvas, AABB2 const& bounds, Texture* texture = nullptr);
    virtual ~Sprite();

    void StartUp() override;
    void ShutDown() override;
    void Update() override;
    void Render() const override;

    void SetTexture(Texture* texture);
    void SetBounds(AABB2 const& bounds);
    void SetColor(Rgba8 color);

protected:
    Canvas* m_canvas = nullptr;
    Texture* m_texture = nullptr;
};