#ifndef SIM_PLAYERINPUT_H_
#define SIM_PLAYERINPUT_H_

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "common.h"

#ifdef DZSIM_WEB_PORT
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif

// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/game/shared/in_buttons.h)
const unsigned int IN_ATTACK        = 1 <<  0;
const unsigned int IN_JUMP          = 1 <<  1;
const unsigned int IN_DUCK          = 1 <<  2;
const unsigned int IN_FORWARD       = 1 <<  3;
const unsigned int IN_BACK          = 1 <<  4;
const unsigned int IN_USE           = 1 <<  5;
const unsigned int IN_MOVELEFT      = 1 <<  9;
const unsigned int IN_MOVERIGHT     = 1 << 10;
const unsigned int IN_ATTACK2       = 1 << 11;
const unsigned int IN_SPEED         = 1 << 17; // Player is holding the speed key
const unsigned int IN_TOGGLE_NOCLIP = 1 << 18;
// --------- end of source-sdk-2013 code ---------

// Handles key and mouse events on a per-frame basis and produces
// game input state to be consumed by the game simulation.
namespace sim::PlayerInput {

#ifdef DZSIM_WEB_PORT
        using Application = Magnum::Platform::EmscriptenApplication;
#else
        using Application = Magnum::Platform::Sdl2Application;
#endif

    // Force the player's viewing angles to some direction
    void SetViewingAngles(const Magnum::Vector3& angles);

    // All of these return true if the given event was accepted (and shouldn't
    // be propagated further).
    bool HandleKeyPressEvent    (Application::KeyEvent& event);
    bool HandleKeyReleaseEvent  (Application::KeyEvent& event);
    bool HandleMousePressEvent  (Application::MouseEvent& event);
    bool HandleMouseReleaseEvent(Application::MouseEvent& event);
    bool HandleMouseMoveEvent   (Application::MouseMoveEvent& event);
    bool HandleMouseScrollEvent (Application::MouseScrollEvent& event);

    // Set all buttons to be unpressed.
    void ClearAllButtons();

    // A summary of all player game inputs during some time interval.
    struct State {
        // Real-time time point of when these inputs have been sampled.
        WallClock::time_point sample_time;

        // IN_* flags, indicating which button is currently being pressed down.
        unsigned int nButtons = 0;

        // Whether the player sent a jump input using the scrollwheel in _THIS_
        // time interval.
        bool scrollwheel_jumped = false;

        Magnum::Vector3 viewing_angles; // pitch, yaw, roll
    };

    // Call this once all inputs of the current frame have been handled.
    // This method measures the current wall-clock time and interprets it as the
    // sample time of the returned player inputs object.
    State FinishFrame();

} // namespace sim::PlayerInput

#endif // SIM_PLAYERINPUT_H_
