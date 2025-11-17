#pragma once
#include "UIScreen.h"
#include <map>

class UIManager
{
public:
    UIManager();
    ~UIManager();
    
    void Startup();
    void Shutdown();
    void Update();
    void Render() const;
    
    void PushScreen(ScreenType type);
    void PopScreen();
    void PopAllScreens();
    UIScreen* GetCurrentScreen() const;
    
    bool IsInputBlocked() const;
    bool IsGamePaused() const;
    
private:
    std::map<ScreenType, UIScreen*> m_screenRegistry;
    std::vector<UIScreen*> m_screenStack;  // 屏幕栈
    
    void RegisterScreen(ScreenType type, UIScreen* screen);
};

extern UIManager* g_theUIManager;