#include "UISystem.h"
#include "UIElement.h"
#include "Engine/Core/EngineCommon.hpp"

UISystem::UISystem(UIConfig config)
    : m_config(config)
{
    m_window = config.m_window;
    m_renderer = config.m_renderer;
    m_inputSystem = config.m_inputSystem;
    //m_bitmapFont = config.m_bitmapFont;
}

void UISystem::Startup()
{
    m_bitmapFont = m_renderer->CreateOrGetBitmapFont(("Data/Fonts/" + m_config.m_bitmapFontName).c_str());
    for (UIElement* uiElement : m_uiElements)
    {
        uiElement->StartUp();
    }
}

void UISystem::Shutdown()
{
}

void UISystem::BeginFrame()
{
}

void UISystem::EndFrame()
{
}

void UISystem::SetCamera(Camera* camera)
{
    UNUSED(camera);
}

Window* UISystem::GetWindow()
{
    return m_window;
}

Renderer* UISystem::GetRenderer()
{
    return m_renderer;
}

InputSystem* UISystem::GetInputSystem()
{
    return m_inputSystem;
}

BitmapFont* UISystem::GetBitmapFont()
{
    return m_bitmapFont;
}

Camera* UISystem::GetCamera()
{
    return nullptr;
}
