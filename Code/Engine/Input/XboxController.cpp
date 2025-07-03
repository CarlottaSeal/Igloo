#include "XboxController.hpp"
#include "AnalogJoystick.hpp"
#include "KeyButtonState.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include < Windows.h> 
#include < Xinput.h >
#include < iostream >
#pragma comment( lib, "xinput" ) // Note: for Windows 7 and earlier support, use “xinput 9_1_0” explicitly instead

extern InputSystem* g_theInput;

XboxController::XboxController()
{
}

XboxController::~XboxController()
{
}

bool XboxController::IsConnected() const
{
	return m_isConnected;
}

int XboxController::GetControllerID() const
{
	return m_id;
}

AnalogJoystick const& XboxController::GetLeftStick() const
{
	return m_leftStick;
}

AnalogJoystick const& XboxController::GetRightStick() const
{
	return m_rightStick;
}

float XboxController::GetLeftTrigger() const
{
	return m_leftTrigger;
}

float XboxController::GetRightTrigger() const
{
	return m_rightTrigger;
}

KeyButtonState const& XboxController::GetButton(XboxButtonID buttonID) const
{
	return m_buttons[static_cast<int>(buttonID)];
}

bool XboxController::IsButtonDown(XboxButtonID buttonID) const
{
	return m_buttons[static_cast<int>(buttonID)].IsPressed();
}

bool XboxController::WasButtonJustPressed(XboxButtonID buttonID) const
{
    return  m_buttons[static_cast<int>(buttonID)].WasJustPressed() 
        && !m_buttons[static_cast<int>(buttonID)].IsPressed();
}

bool XboxController::WasButtonJustRealesed(XboxButtonID buttonID)
{
	return !m_buttons[static_cast<int>(buttonID)].WasJustReleased()
        && !m_buttons[static_cast<int>(buttonID)].IsPressed();
}

float XboxController::GetLeftStickX() const
{
    return m_leftStick.GetNormalizedX();
}

float XboxController::GetLeftStickY() const
{
    return m_leftStick.GetNormalizedY();
}

float XboxController::GetRightStickX() const
{
    return m_rightStick.GetNormalizedX();
}

float XboxController::GetRightStickY() const
{
    return m_rightStick.GetNormalizedY();
}

void XboxController::Update()
{
    // get the current state of the controller
    XINPUT_STATE state;
    memset(&state, 0, sizeof(XINPUT_STATE));

    DWORD result = XInputGetState(m_id, &state); 

	if (result != ERROR_SUCCESS)
	{
		Reset();
		m_isConnected = false;
		return;
	}
    
     m_isConnected = true;        // connected, so update the status

     XINPUT_GAMEPAD const& Xstate = state.Gamepad;

     UpdateJoystick(m_leftStick, Xstate.sThumbLX, Xstate.sThumbLY);
     UpdateJoystick(m_rightStick, Xstate.sThumbRX, Xstate.sThumbRY);

     UpdateTrigger(m_leftTrigger, Xstate.bLeftTrigger);
     UpdateTrigger(m_rightTrigger, Xstate.bRightTrigger);

     UpdateButton(XboxButtonID::A, Xstate.wButtons, XINPUT_GAMEPAD_A);
     UpdateButton(XboxButtonID::B, Xstate.wButtons, XINPUT_GAMEPAD_B);
     UpdateButton(XboxButtonID::X, Xstate.wButtons, XINPUT_GAMEPAD_X);
     UpdateButton(XboxButtonID::Y, Xstate.wButtons, XINPUT_GAMEPAD_Y);
     UpdateButton(XboxButtonID::LB, Xstate.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
     UpdateButton(XboxButtonID::RB, Xstate.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
     UpdateButton(XboxButtonID::BACK, Xstate.wButtons, XINPUT_GAMEPAD_BACK);
     UpdateButton(XboxButtonID::START, Xstate.wButtons, XINPUT_GAMEPAD_START);
     UpdateButton(XboxButtonID::LS, Xstate.wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
     UpdateButton(XboxButtonID::RS, Xstate.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
     UpdateButton(XboxButtonID::DPAD_UP, Xstate.wButtons, XINPUT_GAMEPAD_DPAD_UP);
     UpdateButton(XboxButtonID::DPAD_DOWN, Xstate.wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
     UpdateButton(XboxButtonID::DPAD_LEFT, Xstate.wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
     UpdateButton(XboxButtonID::DPAD_RIGHT, Xstate.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);

}

void XboxController::Reset()
{
    // 将所有状态重置为初始状态
    m_isConnected = false;
    m_leftTrigger = 0.0f;
    m_rightTrigger = 0.0f;
    m_leftStick.Reset();  // 假设 AnalogJoystick 有 Reset 函数
    m_rightStick.Reset();

    // 重置所有按钮状态
    for (int i = 0; i < static_cast<int>(XboxButtonID::NUM); ++i)
    {
        m_buttons[i].Reset();  // 假设 KeyButtonState 有 Reset 函数
    }
}

void XboxController::UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY)
{
	// 将输入值（rawX 和 rawY）进行归一化处理
	float normalizedX = RangeMapClamped(static_cast<float>(rawX), -32768.f, 32767.f, -1.0f, 1.0f);
	float normalizedY = RangeMapClamped(static_cast<float>(rawY), -32768.f, 32767.f, -1.0f, 1.0f);
	// 更新摇杆的位置
	out_joystick.UpdatePosition(normalizedX, normalizedY);
}

void XboxController::UpdateTrigger(float& out_triggerValue, unsigned char rawValue)
{
	out_triggerValue = static_cast<float>(rawValue) / 255.0f; // 假设原始值范围是 0-255
}

void XboxController::UpdateButton(XboxButtonID buttonID, unsigned short buttonFlags, unsigned short buttonFlag)
{
    m_buttons[static_cast<int>(buttonID)].m_wasPressedLastFrame = m_buttons[static_cast<int>(buttonID)].m_isPressed;
    m_buttons[static_cast<int>(buttonID)].m_isPressed = (buttonFlags & buttonFlag) == buttonFlag;
}

