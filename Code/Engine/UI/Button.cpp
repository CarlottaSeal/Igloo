#include "Button.h"

#include "Widget.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

//extern Renderer* g_theRenderer;
//extern InputSystem* g_theInput;

Button::Button()
{
    // g_theUISystem.
}

Button::Button(Widget* parent, AABB2 bound, Rgba8 hoveredColor,Rgba8 textColor, std::string onClickEvent, std::string text,
    Vec2 textAlignment)
{
    m_parent = parent;
    m_bound = bound;
    m_textColor = textColor;
    m_hoveredColor = hoveredColor;
    
    SetOnClickEvent(onClickEvent);

    AddVertsForAABB2D(m_verts, bound, m_originalColor);
    g_theUISystem->GetBitmapFont()->AddVertsForTextInBox2D(m_textVerts, text, bound, bound.GetHeight()*0.25f,
        m_textColor, 0.7f, textAlignment, TextBoxMode::OVERRUN);
}

Button::~Button()
{
}

void Button::Update()
{
    if (m_parent->IsEnabled()|| !m_parent)
    {
        UpdateIfClicked();
        UpdateHoveredColor();
    }
}

void Button::Render() const
{
    //UIElement::Render();
    if (m_parent->IsEnabled())
    {
        g_theUISystem->GetRenderer()->BeginCamera(m_parent->m_renderCamera);
        g_theUISystem->GetRenderer()->SetBlendMode(BlendMode::ALPHA);
        g_theUISystem->GetRenderer()->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
        g_theUISystem->GetRenderer()->SetDepthMode(DepthMode::DISABLED);
        g_theUISystem->GetRenderer()->BindShader(nullptr);
        g_theUISystem->GetRenderer()->BindTexture(nullptr); //TODO: button texture pictures~
        g_theUISystem->GetRenderer()->SetModelConstants(Mat44(), m_renderedColor);
        g_theUISystem->GetRenderer()->DrawVertexArray(m_verts);
        g_theUISystem->GetRenderer()->SetModelConstants();
        g_theUISystem->GetRenderer()->BindTexture(&g_theUISystem->GetBitmapFont()->GetTexture());
        g_theUISystem->GetRenderer()->DrawVertexArray(m_textVerts);
    }
}

bool Button::UpdateIfClicked()
{
    if (m_parent->IsEnabled())
    {
        //Vec2 mousePos = m_parent->GetBounds().GetPointAtUV(g_theUISystem->GetInputSystem()->GetCursorNormalizedPosition());
        Vec2 mousePos = m_parent->m_renderCamera.GetOrthographicBounds().GetPointAtUV(g_theUISystem->GetInputSystem()->GetCursorNormalizedPosition());
        if (IsPointInsideAABB2D(mousePos, m_bound))
        {
            SetHovered(true);
            if (g_theUISystem->GetInputSystem()->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
            {
                OnClick();
                return true;
            }
        }
        else
        {
            SetHovered(false);
            return false;
        }
    }
    return false;
}

void Button::SetOnClickEvent(std::string const& eventName)
{
    m_onClickEventName = eventName;
}

void Button::OnClick()
{
    EventArgs eventArgs;
    eventArgs.SetValue("value", m_onClickEventName);
    g_theEventSystem->FireEvent(m_onClickEventName, eventArgs);
}

void Button::UpdateHoveredColor()
{
    if (m_isHovered)
    {
        m_renderedColor = m_hoveredColor;
    }
    else
    {
        m_renderedColor = m_originalColor;
    }
}

void Button::SetHovered(bool hovered)
{
    m_isHovered = hovered;
}

void Button::SetText(std::string text, Vec2 const& alignment)
{
    UNUSED(alignment);
    m_displayText = text;
}

void Button::SetBound(AABB2 bound)
{
    m_bound = bound;
}
