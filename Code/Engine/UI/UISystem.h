#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Window/Window.hpp"

class UIElement;

struct UIConfig
{
    Window* m_window = nullptr;
    Renderer* m_renderer = nullptr;
    InputSystem* m_inputSystem = nullptr;
    //BitmapFont* m_bitmapFont = nullptr;
    std::string m_bitmapFontName;
};

class UISystem
{
public:
    UISystem(UIConfig config);

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void SetCamera(Camera* camera);

    Window* GetWindow();
    Renderer* GetRenderer();
    InputSystem* GetInputSystem();
    BitmapFont* GetBitmapFont();
    Camera* GetCamera();

private:
    UIConfig m_config;
    Window* m_window = nullptr;
    InputSystem* m_inputSystem = nullptr;
    BitmapFont* m_bitmapFont = nullptr;
    Renderer* m_renderer = nullptr;
    Camera* m_currentCamera = nullptr;

    std::vector<UIElement*> m_uiElements;
};
