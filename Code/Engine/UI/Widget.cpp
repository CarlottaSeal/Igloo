#include "Widget.h"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

//extern Renderer* g_theRenderer;

Widget::Widget(Camera renderCamera, AABB2 bounds, std::string onClickEvent, Rgba8 textColor, std::string text,
    AABB2 textBox)
        :m_bounds(bounds)
        ,m_renderCamera(renderCamera)
        ,m_onClickEvent(onClickEvent)
        ,m_textBox(textBox)
{
    m_type = WIDGET;
    //m_originalColor = originalColor;

    //SetOnClickEvent(onClickEvent);

    std::string fixedText = ReplaceAll(text, "\\n", "\n");

    AddVertsForAABB2D(m_verts, bounds, m_originalColor);
    g_theUISystem->GetBitmapFont()->AddVertsForTextInBox2D(m_textVerts, fixedText, textBox, textBox.GetHeight()*0.5f,
        textColor, 0.7f, Vec2(0.f, 0.5f));
}

Widget::Widget(UIElement* myParent, Camera renderCamera, AABB2 bounds, std::string onClickEvent, Rgba8 textColor,
    std::string text, AABB2 textBox)
        :m_bounds(bounds)
        ,m_renderCamera(renderCamera)
        ,m_onClickEvent(onClickEvent)
        ,m_textBox(textBox)
{
    m_type = WIDGET;
    m_parent = myParent;
    m_originalColor = textColor;
    //SetOnClickEvent(onClickEvent);

    std::string fixedText = ReplaceAll(text, "\\n", "\n");

    AddVertsForAABB2D(m_verts, bounds, m_originalColor);
    g_theUISystem->GetBitmapFont()->AddVertsForTextInBox2D(m_textVerts, fixedText, textBox, textBox.GetHeight()*0.5f,
        textColor, 0.7f, Vec2(0.f,0.5f));
}

Widget::~Widget()
{
}

void Widget::Update()
{
    if (m_isEnabled)
    {
        //if (g_theUISystem->GetInputSystem()->WasKeyJustPressed())
        UpdateIfClicked();
		for (auto child : m_children)
		{
			child->Update();
		}
    }
}

void Widget::Render() const
{
    if (m_isEnabled)
    {
        g_theUISystem->GetRenderer()->BeginCamera(m_renderCamera);
        g_theUISystem->GetRenderer()->SetBlendMode(BlendMode::ALPHA);
        g_theUISystem->GetRenderer()->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
        g_theUISystem->GetRenderer()->SetDepthMode(DepthMode::DISABLED);
        g_theUISystem->GetRenderer()->BindShader(nullptr);
        g_theUISystem->GetRenderer()->BindTexture(nullptr); //TODO: button texture pictures~
        g_theUISystem->GetRenderer()->DrawVertexArray(m_verts);
        g_theUISystem->GetRenderer()->BindTexture(&g_theUISystem->GetBitmapFont()->GetTexture());
        g_theUISystem->GetRenderer()->DrawVertexArray(m_textVerts);

        for (auto child : m_children)
        {
            child->Render();
        }
    }
}

void Widget::SetEnabled(bool enabled)
{
    m_isEnabled = enabled;
    if (enabled)
    {
        m_isInteractive = false;
        g_theUISystem->QueueEnableInputNextFrame(this); // 延迟一帧启用交互
    }
    else
    {
        m_isInteractive = false;
    }
}

Widget& Widget::SetBounds(const AABB2& localBounds)
{
    m_bound = localBounds;
    return *this;
}

Widget& Widget::SetBounds(const Vec2& mins, const Vec2& maxs)
{
    m_bounds = AABB2(mins, maxs);
    return *this;
}

Widget& Widget::SetText(const char* text, BitmapFont* font, const Rgba8& color, float height)
{
    UNUSED(text);
    UNUSED(font);
    UNUSED(color);
    UNUSED(height);
    return *this;
}

void Widget::Reset()
{
    ResetStatus();
    for (auto* child : m_children)
    {
        child->ResetStatus();
    }
}

void Widget::AddChild(UIElement& uiElement)
{
    for (UIElement*& child : m_children)
    {
        if (child == nullptr)
        {
            child = &uiElement;
            return;
        }
    }
    m_children.push_back(&uiElement);
}

void Widget::SetOnClickEvent(std::string onClickEvent)
{
    if (onClickEvent == "")
    {
        return;
    }
    else
    {
        m_onClickEvent = onClickEvent;
    }
}

void Widget::OnClick()
{
    if (!m_children.empty())
    {
        return;   
    }
    else
    {
        if (m_onClickEvent != "")
        {
			EventArgs args;
			args.SetValue("value", m_onClickEvent);
			g_theEventSystem->FireEvent(m_onClickEvent, args);
        }        
    }
}

void Widget::UpdateIfClicked()
{
    if (m_children.empty()) //只会接收自己的事件
    {
        if (g_theUISystem->GetInputSystem()->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
        {
            OnClick();
        }
    }
}

void Widget::Build()
{
}
