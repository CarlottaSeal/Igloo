#pragma once

#pragma once
#include "Engine/UI/Widget.h"
#include <string>

class Canvas;
class UISystem;

enum class UIScreenType
{
    UNKNOWN,
    MAIN_MENU,
    PAUSE_MENU,
    INVENTORY,
    HUD,
    CHEST,
    FURNACE,
    CRAFTING_TABLE,
    OPTIONS,
    WORLD_SELECT,
    SERVER_LIST
};

class UIScreen
{
public:
    UIScreen(UISystem* uiSystem, UIScreenType type, bool blocksInput = true);
    virtual ~UIScreen();
    
    // 生命周期方法
    virtual void OnEnter();     // 屏幕被激活时调用
    virtual void OnExit();      // 屏幕被销毁时调用
    virtual void OnPause();     // 屏幕被覆盖时调用（但不销毁）
    virtual void OnResume();    // 屏幕重新激活时调用
    
    // 核心方法（子类必须实现）
    virtual void Build() = 0;   // 构建 UI 元素
    
    // 可选重写的方法
    virtual void Update(float deltaSeconds);
    virtual void Render() const;
    virtual void HandleInput();
    
    // 属性访问
    UIScreenType GetType() const { return m_type; }
    bool BlocksInput() const { return m_blocksInput; }
    bool IsActive() const { return m_isActive; }
    void SetActive(bool active);
    
    Canvas* GetCanvas() const { return m_canvas; }
    
protected:
    UISystem* m_uiSystem = nullptr;
    Camera* m_camera = nullptr;
    Canvas* m_canvas = nullptr;
    
    UIScreenType m_type = UIScreenType::UNKNOWN;
    bool m_blocksInput = true;  // 是否阻挡下层屏幕的输入
    bool m_isActive = false;
    
    std::vector<UIElement*> m_elements;  // 该屏幕拥有的所有 UI 元素
};

