#include "InputHandler.h"

#include <Corrade/Containers/StringStl.h> // For implicit StringView to std::string conversion
#include <Magnum/Math/Functions.h>

using namespace Magnum;
using MouseEvent       = InputHandler::Application::MouseEvent;
using MouseMoveEvent   = InputHandler::Application::MouseMoveEvent;
using MouseScrollEvent = InputHandler::Application::MouseScrollEvent;
using KeyEvent         = InputHandler::Application::KeyEvent;

InputHandler::InputHandler() :
    _keymap_keyboard(),
    _keymap_mouse(),
    _mouse_pos(0,0),
    _mouse_pos_change(0,0),
    _callback_mouse_move()
{
}

void InputHandler::HandleMousePressEvent(MouseEvent& event)
{
    std::string name;
    switch (event.button()) {
    case MouseEvent::Button::Left:   name = "MButtonLeft";   break;
    case MouseEvent::Button::Right:  name = "MButtonRight";  break;
    case MouseEvent::Button::Middle: name = "MButtonMiddle"; break;
#ifndef DZSIM_WEB_PORT
    case MouseEvent::Button::X1:     name = "MButtonExtra1"; break;
    case MouseEvent::Button::X2:     name = "MButtonExtra2"; break;
#endif
    default: // Unknown button
        return;
    }
    event.setAccepted();

    InputState& state = _keymap_mouse[name];
    state._beingPressed = true;
    state._pressCount++;
    if (state._callback_pressed)
        state._callback_pressed();
}

void InputHandler::HandleMouseReleaseEvent(MouseEvent& event)
{
    std::string name;
    switch (event.button()) {
    case MouseEvent::Button::Left:   name = "MButtonLeft";   break;
    case MouseEvent::Button::Right:  name = "MButtonRight";  break;
    case MouseEvent::Button::Middle: name = "MButtonMiddle"; break;
#ifndef DZSIM_WEB_PORT
    case MouseEvent::Button::X1:     name = "MButtonExtra1"; break;
    case MouseEvent::Button::X2:     name = "MButtonExtra2"; break;
#endif
    default: // Unknown button
        return;
    }
    event.setAccepted();
    
    InputState& state = _keymap_mouse[name];
    state._beingPressed = false;
    if (state._callback_released)
        state._callback_released();
}

void InputHandler::HandleMouseMoveEvent(MouseMoveEvent& event)
{
    event.setAccepted();
    _mouse_pos = event.position();
    _mouse_pos_change += event.relativePosition();
    if (_callback_mouse_move)
        _callback_mouse_move();
}

void InputHandler::HandleMouseScrollEvent(MouseScrollEvent& event)
{
    event.setAccepted();
    
    Long vert = (Long)(event.offset().y());
    if (vert != 0) {
        InputState& state = vert > 0 ? _keymap_mouse["MWheelUp"] : _keymap_mouse["MWheelDown"];
        state._pressCount += Math::abs(vert);
        for (size_t i = Math::abs(vert); i != 0; --i) {
            if (state._callback_pressed ) state._callback_pressed();
            if (state._callback_released) state._callback_released();
        }
    }

    Long hori = (Long)(event.offset().x());
    if (hori != 0) {
        InputState& state = hori > 0 ? _keymap_mouse["MWheelRight"] : _keymap_mouse["MWheelLeft"];
        state._pressCount += Math::abs(hori);
        for (size_t i = Math::abs(hori); i != 0; --i) {
            if (state._callback_pressed ) state._callback_pressed();
            if (state._callback_released) state._callback_released();
        }
    }
}

void InputHandler::HandleKeyPressEvent(KeyEvent& event)
{
    std::string name = event.keyName();
    if (name.length() == 0) // No name for that key -> ignore
        return;
    event.setAccepted();

#ifndef DZSIM_WEB_PORT
    // When holding down a key, ignore repeated presses
    if (event.isRepeated())
        return;
#endif
    
    InputState& state = _keymap_keyboard[name];
    state._beingPressed = true;
    state._pressCount++;
    if(state._callback_pressed)
        state._callback_pressed();

    if (!state._callback_pressed) // Debug code
        Debug{} << name.c_str();
}

void InputHandler::HandleKeyReleaseEvent(KeyEvent& event)
{
    std::string name = event.keyName();
    if (name.length() == 0) // No name for that key -> ignore
        return;
    event.setAccepted();

    InputState& state = _keymap_keyboard[name];
    state._beingPressed = false;
    if (state._callback_released)
        state._callback_released();
}

InputHandler::Key InputHandler::GetKey(std::string name, bool from_mouse)
{
    return InputHandler::Key(name, from_mouse);
}

InputHandler::Key InputHandler::GetMouseKey(std::string name)
{
    return InputHandler::Key(name, true);
}

InputHandler::Key InputHandler::GetKeyboardKey(std::string name)
{
    return InputHandler::Key(name, false);
}

bool InputHandler::IsKeyBeingPressed(InputHandler::Key key)
{
    if (key.from_mouse) return IsKeyBeingPressed_mouse   (key.name);
    else                return IsKeyBeingPressed_keyboard(key.name);
}

bool InputHandler::IsKeyBeingPressed_mouse(std::string mouseKeyName)
{
    auto search = _keymap_mouse.find(mouseKeyName);
    if (search != _keymap_mouse.end())
        return search->second._beingPressed;
    return false;
}

bool InputHandler::IsKeyBeingPressed_keyboard(std::string keyboardKeyName)
{
    auto search = _keymap_keyboard.find(keyboardKeyName);
    if (search != _keymap_keyboard.end())
        return search->second._beingPressed;
    return false;
}

size_t InputHandler::GetKeyPressCountAndReset(InputHandler::Key key)
{
    if (key.from_mouse) return GetKeyPressCountAndReset_mouse   (key.name);
    else                return GetKeyPressCountAndReset_keyboard(key.name);
}

size_t InputHandler::GetKeyPressCountAndReset_mouse(std::string mouseKeyName)
{
    auto search = _keymap_mouse.find(mouseKeyName);
    if (search != _keymap_mouse.end()) {
        size_t ret = search->second._pressCount;
        search->second._pressCount = 0;
        return ret;
    }
    return 0;
}

size_t InputHandler::GetKeyPressCountAndReset_keyboard(std::string keyboardKeyName)
{
    auto search = _keymap_keyboard.find(keyboardKeyName);
    if (search != _keymap_keyboard.end()) {
        size_t ret = search->second._pressCount;
        search->second._pressCount = 0;
        return ret;
    }
    return 0;
}

void InputHandler::SetKeyPressedCallback(InputHandler::Key key, std::function<void()> cb)
{
    if (key.from_mouse) return SetKeyPressedCallback_mouse   (key.name, cb);
    else                return SetKeyPressedCallback_keyboard(key.name, cb);
}

void InputHandler::SetKeyPressedCallback_mouse(std::string mouseKeyName, std::function<void()> cb)
{
    _keymap_mouse[mouseKeyName]._callback_pressed = cb;
}

void InputHandler::SetKeyPressedCallback_keyboard(std::string keyboardKeyName, std::function<void()> cb)
{
    _keymap_keyboard[keyboardKeyName]._callback_pressed = cb;
}

void InputHandler::SetKeyReleasedCallback(InputHandler::Key key, std::function<void()> cb)
{
    if (key.from_mouse) return SetKeyReleasedCallback_mouse   (key.name, cb);
    else                return SetKeyReleasedCallback_keyboard(key.name, cb);
}

void InputHandler::SetKeyReleasedCallback_mouse(std::string mouseKeyName, std::function<void()> cb)
{
    _keymap_mouse[mouseKeyName]._callback_released = cb;
}

void InputHandler::SetKeyReleasedCallback_keyboard(std::string keyboardKeyName, std::function<void()> cb)
{
    _keymap_keyboard[keyboardKeyName]._callback_released = cb;
}

void InputHandler::ClearKeyPressedCallback(InputHandler::Key key)
{
    if (key.from_mouse) return ClearKeyPressedCallback_mouse   (key.name);
    else                return ClearKeyPressedCallback_keyboard(key.name);
}

void InputHandler::ClearKeyPressedCallback_mouse(std::string mouseKeyName)
{
    _keymap_mouse[mouseKeyName]._callback_pressed = std::function<void()>();
}

void InputHandler::ClearKeyPressedCallback_keyboard(std::string keyboardKeyName)
{
    _keymap_keyboard[keyboardKeyName]._callback_pressed = std::function<void()>();
}

void InputHandler::ClearKeyReleasedCallback(InputHandler::Key key)
{
    if (key.from_mouse) return ClearKeyReleasedCallback_mouse   (key.name);
    else                return ClearKeyReleasedCallback_keyboard(key.name);
}

void InputHandler::ClearKeyReleasedCallback_mouse(std::string mouseKeyName)
{
    _keymap_mouse[mouseKeyName]._callback_released = std::function<void()>();
}

void InputHandler::ClearKeyReleasedCallback_keyboard(std::string keyboardKeyName)
{
    _keymap_keyboard[keyboardKeyName]._callback_released = std::function<void()>();
}

Magnum::Vector2i InputHandler::GetMousePos()
{
    return _mouse_pos;
}

Magnum::Vector2i InputHandler::GetMousePosChangeAndReset()
{
    Magnum::Vector2i ret = _mouse_pos_change;
    _mouse_pos_change.x() = 0;
    _mouse_pos_change.y() = 0;
    return ret;
}

void InputHandler::SetMousePosChangeCallback(std::function<void()> cb)
{
    _callback_mouse_move = cb;
}

void InputHandler::ClearMousePosChangeCallback()
{
    _callback_mouse_move = std::function<void()>();
}
