#pragma once
#include "UISystem.h"

class Widget;

class UIElement
{
public:
    UIElement();
    virtual ~UIElement() = default;

    virtual void StartUp();
    virtual void ShutDown();
    virtual void Update();
    virtual void Render() const;

    bool AddChild(UIElement* child);
    bool RemoveChild(UIElement* child);
    void SetParent(UIElement* parent);

    bool IsOtherElementHavingFocus() const;

    const AABB2& GetBounds() const {return m_bound;}

    void SetEnabled(bool enabled);
    void SetFocused(bool focused);
    bool IsEnabled();
    bool HasFocus();

    void InitializeVerts();
    
protected:
    UIElement* m_parent;
    //Widget* m_parentWidget;
    std::vector<UIElement*> m_children;

    AABB2 m_bound;

    std::vector<Vertex_PCU> m_verts;
    std::vector<Vertex_PCU> m_textVerts;

    Rgba8 m_renderedColor;
    Rgba8 m_originalColor = Rgba8::GREY;

    bool m_interactive = true;
    bool m_isEnabled = true;
    bool m_isFocused = false;
};
