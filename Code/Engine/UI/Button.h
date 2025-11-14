#pragma once
#include "UIElement.h"

class Button : public UIElement
{
public:
    Button();
    Button(Widget* parent, AABB2 bound, Rgba8 hoveredColor = Rgba8::MAGENTA,Rgba8 textColor = Rgba8::TEAL, std::string onClickEvent = "",
        std::string text = "NO TEXT", Vec2 textAlignment = Vec2(0.5f, 0.5f), std::string texturePath = "");
    
    virtual ~Button() override;

    virtual void Update() override;
    virtual void Render() const override;

    bool UpdateIfClicked(); //假设目前只有button能click
    void SetOnClickEvent(std::string const& eventName);
    void OnClick();

    void UpdateHoveredColor();
    
    void SetHovered(bool hovered);
    void SetText(std::string text, Vec2 const& alignment);
    void SetBound(AABB2 bound);

public:
    Widget* m_parent;  //复写m_parent，指定只能有widget作为parent
    std::string m_onClickEventName;
    //bool m_isHovered = false;

    std::string m_displayText;

    Texture* m_texture;
    
    Rgba8 m_textColor;
};
