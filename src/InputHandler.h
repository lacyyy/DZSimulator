#ifndef INPUTHANDLER_H_
#define INPUTHANDLER_H_

#include <string>
#include <map>
#include <functional>

#ifdef DZSIM_WEB_PORT
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif

class InputHandler {
public:

    class Key {
    public:
        Key(std::string name, bool from_mouse) :
            name(name), from_mouse(from_mouse) {}

        std::string name;
        bool from_mouse; // when false, from keyboard
    };

private:

    class InputState {
    public:
        InputState() :
            _beingPressed(false),
            _pressCount(0),
            _callback_pressed(),
            _callback_released()
        {}

        bool _beingPressed;
        size_t _pressCount;
        std::function<void()> _callback_pressed;
        std::function<void()> _callback_released;
    };

public:
#ifdef DZSIM_WEB_PORT
    typedef Magnum::Platform::EmscriptenApplication Application;
#else
    typedef Magnum::Platform::Sdl2Application Application;
#endif

public:
    InputHandler();
    void HandleMousePressEvent  (Application::MouseEvent& event);
    void HandleMouseReleaseEvent(Application::MouseEvent& event);
    void HandleMouseMoveEvent   (Application::MouseMoveEvent& event);
    void HandleMouseScrollEvent (Application::MouseScrollEvent& event);
    void HandleKeyPressEvent    (Application::KeyEvent& event);
    void HandleKeyReleaseEvent  (Application::KeyEvent& event);

    InputHandler::Key GetKey(std::string name, bool from_mouse);
    InputHandler::Key GetMouseKey(std::string name);
    InputHandler::Key GetKeyboardKey(std::string name);

    bool IsKeyBeingPressed(InputHandler::Key key);
    bool IsKeyBeingPressed_mouse(std::string mouseKeyName);
    bool IsKeyBeingPressed_keyboard(std::string keyboardKeyName);

    size_t GetKeyPressCountAndReset(InputHandler::Key key);
    size_t GetKeyPressCountAndReset_mouse(std::string mouseKeyName);
    size_t GetKeyPressCountAndReset_keyboard(std::string keyboardKeyName);

    void SetKeyPressedCallback(InputHandler::Key key, std::function<void()> cb);
    void SetKeyPressedCallback_mouse(std::string mouseKeyName, std::function<void()> cb);
    void SetKeyPressedCallback_keyboard(std::string keyboardKeyName, std::function<void()> cb);

    void SetKeyReleasedCallback(InputHandler::Key key, std::function<void()> cb);
    void SetKeyReleasedCallback_mouse(std::string mouseKeyName, std::function<void()> cb);
    void SetKeyReleasedCallback_keyboard(std::string keyboardKeyName, std::function<void()> cb);

    void ClearKeyPressedCallback(InputHandler::Key key);
    void ClearKeyPressedCallback_mouse(std::string mouseKeyName);
    void ClearKeyPressedCallback_keyboard(std::string keyboardKeyName);

    void ClearKeyReleasedCallback(InputHandler::Key key);
    void ClearKeyReleasedCallback_mouse(std::string mouseKeyName);
    void ClearKeyReleasedCallback_keyboard(std::string keyboardKeyName);

    Magnum::Vector2i GetMousePos();
    Magnum::Vector2i GetMousePosChangeAndReset();
    void SetMousePosChangeCallback(std::function<void()> cb);
    void ClearMousePosChangeCallback();

private:
    std::map<std::string, InputState> _keymap_keyboard;
    std::map<std::string, InputState> _keymap_mouse;

    // mouse pos input
    Magnum::Vector2i _mouse_pos;
    Magnum::Vector2i _mouse_pos_change;
    std::function<void()> _callback_mouse_move;

};

#endif // !INPUTHANDLER_H_
