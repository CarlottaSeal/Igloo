#include "Button.h"

#include "Widget.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

extern Renderer* g_theRenderer;

Button::Button()
{
}

Button::Button(Widget* parent, AABB2 bound, Rgba8 hoveredColor, Rgba8 textColor, std::string onClickEvent, 
               std::string text, Vec2 textAlignment, std::string texturePath)
{
    m_type = BUTTON;
    m_parent = parent;
    m_bound = bound;
    m_textColor = textColor;
    m_hoveredColor = hoveredColor;
    
    // 保持原有的 EventSystem 接口
    SetOnClickEvent(onClickEvent);

    AddVertsForAABB2D(m_verts, bound, m_originalColor);
    g_theUISystem->GetBitmapFont()->AddVertsForTextInBox2D(m_textVerts, text, bound, bound.GetHeight()*0.25f,
        m_textColor, 0.7f, textAlignment, TextBoxMode::OVERRUN);

    if (texturePath != "")
        m_texture = g_theRenderer->CreateOrGetTextureFromFile(texturePath.c_str());
}

Button::~Button()
{
    // UIEvent 会自动清理监听器
}

void Button::Update()
{
    if (m_parent->IsEnabled() || !m_parent)
    {
        UpdateIfClicked();
        UpdateHoveredColor();
    }
}

void Button::Render() const
{
    if (m_parent->IsEnabled())
    {
        g_theUISystem->GetRenderer()->BeginCamera(m_parent->m_renderCamera);
        g_theUISystem->GetRenderer()->SetBlendMode(BlendMode::ALPHA);
        g_theUISystem->GetRenderer()->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
        g_theUISystem->GetRenderer()->SetDepthMode(DepthMode::DISABLED);
        g_theUISystem->GetRenderer()->BindShader(nullptr);
#ifdef ENGINE_DX11_RENDERER
        g_theUISystem->GetRenderer()->BindTexture(m_texture); 
#endif
#ifdef ENGINE_DX12_RENDERER
        g_theUISystem->GetRenderer()->SetMaterialConstants(m_texture);
#endif
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
        Vec2 mousePos = m_parent->m_renderCamera.GetOrthographicBounds().GetPointAtUV(
            g_theUISystem->GetInputSystem()->GetCursorNormalizedPosition());
        
        if (IsPointInsideAABB2D(mousePos, m_bound))
        {
            // 首次进入悬停状态
            if (!m_isHovered)
            {
                SetHovered(true);
                m_onHoverUIEvent.Invoke();  // 触发新的 UIEvent
            }
            
            // 按下状态
            if (g_theUISystem->GetInputSystem()->IsKeyDown(KEYCODE_LEFT_MOUSE))
            {
                m_onPressedUIEvent.Invoke();  // 触发新的 UIEvent
            }
            
            // 点击完成
            if (g_theUISystem->GetInputSystem()->WasKeyJustReleased(KEYCODE_LEFT_MOUSE))
            {
                OnClick();  // 会同时触发 EventSystem 和 UIEvent
                return true;
            }
        }
        else
        {
            // 离开悬停状态
            if (m_isHovered)
            {
                m_onUnhoverUIEvent.Invoke();  // 触发新的 UIEvent
            }
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
    // 1. 触发原有的 EventSystem（向后兼容）
    if (!m_onClickEventName.empty())
    {
        EventArgs eventArgs;
        eventArgs.SetValue("value", m_onClickEventName);
        g_theEventSystem->FireEvent(m_onClickEventName, eventArgs);
    }
    
    // 2. 触发新的 UIEvent（新功能）
    m_onClickUIEvent.Invoke();
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

size_t Button::OnClickEvent(UICallbackFunctionPointer const& callback)
{
    return m_onClickUIEvent.AddListener(callback);
}

size_t Button::OnHoverEvent(UICallbackFunctionPointer const& callback)
{
    return m_onHoverUIEvent.AddListener(callback);
}

size_t Button::OnUnhoverEvent(UICallbackFunctionPointer const& callback)
{
    return m_onUnhoverUIEvent.AddListener(callback);
}

size_t Button::OnPressedEvent(UICallbackFunctionPointer const& callback)
{
    return m_onPressedUIEvent.AddListener(callback);
}