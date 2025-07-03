#include "KeyButtonState.hpp"


// 更新按钮状态
void KeyButtonState::UpdateStatus(bool isPressed)
{
    m_wasPressedLastFrame = m_isPressed; // 保存上一帧的状态
    m_isPressed = isPressed;             // 更新为当前帧的状态
}

// 检查当前按钮是否按下
bool KeyButtonState::IsPressed() const
{
    return m_isPressed;
}

// 检查按钮是否在当前帧刚刚被按下
bool KeyButtonState::WasJustPressed() const
{
    return !m_isPressed && m_wasPressedLastFrame;
}

// 检查按钮是否在当前帧刚刚被释放
bool KeyButtonState::WasJustReleased() const
{
    return !m_isPressed && m_wasPressedLastFrame;
}

// 重置按钮状态
void KeyButtonState::Reset()
{
    m_isPressed = false;
    m_wasPressedLastFrame = false;
}

void KeyButtonState::EndFrame()
{
    m_wasPressedLastFrame = m_isPressed;
}
