#include "sim/PlayerInput.h"

#include <Corrade/Containers/StringView.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector3.h>

#include "common.h"

using namespace Corrade::Containers;
using namespace Corrade::Containers::Literals;
using namespace Magnum;
using namespace sim;
using namespace sim::PlayerInput;

// Game key bindings
constexpr StringView KEYNAME_FORWARD   = "W"_s;
constexpr StringView KEYNAME_BACK      = "S"_s;
constexpr StringView KEYNAME_MOVELEFT  = "A"_s;
constexpr StringView KEYNAME_MOVERIGHT = "D"_s;
constexpr StringView KEYNAME_USE       = "E"_s;
constexpr StringView KEYNAME_JUMP      = "Space"_s;
constexpr StringView KEYNAME_DUCK      = "Left Ctrl"_s;
constexpr StringView KEYNAME_SPEED     = "Left Shift"_s;

// Player input state of the current frame
static State cur_frame;


void PlayerInput::SetViewingAngles(const Magnum::Vector3& angles)
{
    cur_frame.viewing_angles = angles;
}

bool PlayerInput::HandleKeyPressEvent(Application::KeyEvent& event)
{
    StringView name = event.keyName();
    bool m = false; // Did we match the keyname?
    if (name == KEYNAME_FORWARD  ) { m = true; cur_frame.nButtons |= IN_FORWARD;   }
    if (name == KEYNAME_BACK     ) { m = true; cur_frame.nButtons |= IN_BACK;      }
    if (name == KEYNAME_MOVELEFT ) { m = true; cur_frame.nButtons |= IN_MOVELEFT;  }
    if (name == KEYNAME_MOVERIGHT) { m = true; cur_frame.nButtons |= IN_MOVERIGHT; }
    if (name == KEYNAME_USE      ) { m = true; cur_frame.nButtons |= IN_USE;       }
    if (name == KEYNAME_JUMP     ) { m = true; cur_frame.nButtons |= IN_JUMP;      }
    if (name == KEYNAME_DUCK     ) { m = true; cur_frame.nButtons |= IN_DUCK;      }
    if (name == KEYNAME_SPEED    ) { m = true; cur_frame.nButtons |= IN_SPEED;     }
    if (m)
        event.setAccepted();
    return event.isAccepted();
}

bool PlayerInput::HandleKeyReleaseEvent(Application::KeyEvent& event)
{
    StringView name = event.keyName();
    bool m = false; // Did we match the keyname?
    if (name == KEYNAME_FORWARD  ) { m = true; cur_frame.nButtons &= ~IN_FORWARD;   }
    if (name == KEYNAME_BACK     ) { m = true; cur_frame.nButtons &= ~IN_BACK;      }
    if (name == KEYNAME_MOVELEFT ) { m = true; cur_frame.nButtons &= ~IN_MOVELEFT;  }
    if (name == KEYNAME_MOVERIGHT) { m = true; cur_frame.nButtons &= ~IN_MOVERIGHT; }
    if (name == KEYNAME_USE      ) { m = true; cur_frame.nButtons &= ~IN_USE;       }
    if (name == KEYNAME_JUMP     ) { m = true; cur_frame.nButtons &= ~IN_JUMP;      }
    if (name == KEYNAME_DUCK     ) { m = true; cur_frame.nButtons &= ~IN_DUCK;      }
    if (name == KEYNAME_SPEED    ) { m = true; cur_frame.nButtons &= ~IN_SPEED;     }
    if (m)
        event.setAccepted();
    return event.isAccepted();
}

bool PlayerInput::HandleMousePressEvent(Application::MouseEvent& event)
{
    if (event.button() == Application::MouseEvent::Button::Left) {
        cur_frame.nButtons |= IN_ATTACK;
        event.setAccepted();
    }
    if (event.button() == Application::MouseEvent::Button::Right) {
        cur_frame.nButtons |= IN_TOGGLE_NOCLIP;
        event.setAccepted();
    }
    return event.isAccepted();
}

bool PlayerInput::HandleMouseReleaseEvent(Application::MouseEvent& event)
{
    if (event.button() == Application::MouseEvent::Button::Left) {
        cur_frame.nButtons &= ~IN_ATTACK;
        event.setAccepted();
    }
    if (event.button() == Application::MouseEvent::Button::Right) {
        cur_frame.nButtons &= ~IN_TOGGLE_NOCLIP;
        event.setAccepted();
    }
    return event.isAccepted();
}

bool PlayerInput::HandleMouseMoveEvent(Application::MouseMoveEvent& event)
{
    const float AIM_SENSITIVITY = 0.03f;
    Vector2 delta = AIM_SENSITIVITY * Vector2{ event.relativePosition() };

    cur_frame.viewing_angles[0] += delta.y(); // Pitch
    cur_frame.viewing_angles[1] -= delta.x(); // Yaw

    // Clamp pitch
    cur_frame.viewing_angles[0] =
        Math::clamp(cur_frame.viewing_angles[0], -89.0f, +89.0f);

    // Let yaw wrap around from -180 to +180 and vice versa
    if (Math::abs(cur_frame.viewing_angles[1]) > 180.0f) {
        float overturn;
        if (cur_frame.viewing_angles[1] > 180.0f)
            overturn = cur_frame.viewing_angles[1] - 180.0f;
        else
            overturn = cur_frame.viewing_angles[1] + 180.0f;
        int full_360s = (int)(overturn / 360.0f);
        overturn -= (float)full_360s * 360.0f;

        if (cur_frame.viewing_angles[1] > 180.0f)
            cur_frame.viewing_angles[1] = -180.0f + overturn;
        else
            cur_frame.viewing_angles[1] = +180.0f + overturn;
    }

    event.setAccepted();
    return true;
}

bool PlayerInput::HandleMouseScrollEvent(Application::MouseScrollEvent& event)
{
    long vert = (long)event.offset().y();
    if (vert != 0) // User scrolled up or down
        cur_frame.scrollwheel_jumped = true;
    event.setAccepted();
    return true;
}

void PlayerInput::ClearAllButtons()
{
    cur_frame.nButtons = 0;
}

State PlayerInput::FinishFrame()
{
    cur_frame.sample_time = WallClock::now();
    State ret = cur_frame;

    // Reset frame-local state
    cur_frame.scrollwheel_jumped = false;

    return ret;
}
