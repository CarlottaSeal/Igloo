#include "UIScreen.h"

#include "Canvas.hpp"

UIScreen::UIScreen(UISystem* uiSystem, UIScreenType type, bool blocksInput)
    : m_uiSystem(uiSystem)
    , m_type(type)
    , m_blocksInput(blocksInput)
{
    // 创建专属的 Camera
    m_camera = new Camera();
    m_camera->SetOrthographicView(Vec2(0, 0), Vec2(1600, 900));  // TODO: 灵活调整
    
    m_canvas = new Canvas(m_uiSystem, m_camera);
}

UIScreen::~UIScreen()
{
    if (m_canvas)
    {
        m_canvas->ShutDown();
        delete m_canvas;
        m_canvas = nullptr;
    }
    
    if (m_camera)
    {
        delete m_camera;
        m_camera = nullptr;
    }
}

void UIScreen::OnEnter()
{
    m_isActive = true;
    if (m_canvas)
    {
        m_canvas->SetActive(true);
    }
}

void UIScreen::OnExit()
{
    m_isActive = false;
    if (m_canvas)
    {
        m_canvas->SetActive(false);
    }
}

void UIScreen::OnPause()
{
    // 默认：暂停时禁用输入但继续渲染
    if (m_canvas)
    {
        for (auto* element : m_canvas->m_uiElementsList)
        {
            if (element)
            {
                element->SetFocused(false);
            }
        }
    }
}

void UIScreen::OnResume()
{
    // 默认：恢复时重新启用
    m_isActive = true;
}

void UIScreen::Update(float deltaSeconds)
{
    UNUSED(deltaSeconds)
    if (!m_isActive)
    {
        return;
    }
    
    if (m_canvas)
    {
        m_canvas->Update();
    }
}

void UIScreen::Render() const
{
    if (m_canvas)
    {
        m_canvas->Render();
    }
}

void UIScreen::HandleInput()
{
    // 子类可以重写此方法处理特殊输入
}

void UIScreen::SetActive(bool active)
{
    m_isActive = active;
    if (m_canvas)
    {
        m_canvas->SetActive(active);
    }
}