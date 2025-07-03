#pragma once

struct KeyButtonState 
{
public:
    bool m_isPressed = false;
    bool m_wasPressedLastFrame = false;

    void UpdateStatus(bool isPressed);

    bool IsPressed() const;

    bool WasJustPressed() const;

    bool WasJustReleased() const;

    void Reset();

    void EndFrame();
};