#pragma once
#include "UIElement.h"
#include "UIEvent.h"

class Button : public UIElement
{
public:
    Button();
    Button(Widget* parent, AABB2 bound, Rgba8 hoveredColor = Rgba8::MAGENTA, Rgba8 textColor = Rgba8::TEAL, 
           std::string onClickEvent = "", std::string text = "NO TEXT", Vec2 textAlignment = Vec2(0.5f, 0.5f), 
           std::string texturePath = "");
    
    virtual ~Button() override;

    virtual void Update() override;
    virtual void Render() const override;

    bool UpdateIfClicked();
    
    void SetOnClickEvent(std::string const& eventName);
    void OnClick();  // 会同时触发 EventSystem 和 UIEvent
    
    size_t OnClickEvent(UICallbackFunctionPointer const& callback);
    
    size_t OnHoverEvent(UICallbackFunctionPointer const& callback);
    size_t OnUnhoverEvent(UICallbackFunctionPointer const& callback);
    
    size_t OnPressedEvent(UICallbackFunctionPointer const& callback);
    
    void RemoveClickListener(size_t id) { m_onClickUIEvent.RemoveListener(id); }
    void RemoveHoverListener(size_t id) { m_onHoverUIEvent.RemoveListener(id); }
    void RemoveUnhoverListener(size_t id) { m_onUnhoverUIEvent.RemoveListener(id); }
    void RemovePressedListener(size_t id) { m_onPressedUIEvent.RemoveListener(id); }

    void UpdateHoveredColor();
    void SetHovered(bool hovered);
    void SetText(std::string text, Vec2 const& alignment);
    void SetBound(AABB2 bound);

public:
    Widget* m_parent;
    
    // 保留原有的 EventSystem 事件名称
    std::string m_onClickEventName;
    
    std::string m_displayText;
    Texture* m_texture;
    Rgba8 m_textColor;

protected:
    UIEvent m_onClickUIEvent;
    UIEvent m_onHoverUIEvent;
    UIEvent m_onUnhoverUIEvent;
    UIEvent m_onPressedUIEvent;
};