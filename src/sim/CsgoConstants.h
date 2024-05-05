#ifndef SIM_CSGOCONSTANTS_H_
#define SIM_CSGOCONSTANTS_H_

#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Vector3.h>

using namespace Magnum::Math::Literals;

namespace sim {

const float CSGO_TICKRATE = 64.0f; // Matchmaking Danger Zone server tickrate

// CSGO's horizontal FOV depends on resolution aspect ratio
// Testing indicates that CSGO's vertical FOV is between 73.7 and 73.8 degrees
// A precise FOV value here makes the image much sharper when DZSim is used as
// an overlay above CSGO!
const Magnum::Math::Deg CSGO_VERT_FOV = 73.75_degf; // CSGO's fixed vertical field of view

const float CSGO_PLAYER_HEIGHT_STANDING = 72.0f;
const float CSGO_PLAYER_HEIGHT_CROUCHED = 54.0f;
const float CSGO_PLAYER_WIDTH = 32.0f; // width and depth
const float CSGO_PLAYER_EYE_LEVEL_STANDING  = 64.062561f;
const float CSGO_PLAYER_EYE_LEVEL_CROUCHING = 46.044983f;

const float CSGO_BUMP_BOOST_SPEED = 1200.0f;
const float CSGO_BUMP_BOOST_COOLDOWN_SECS = 0.4f; // Cooldown per player
// How many units BELOW THE PLAYER'S EYE the Bump Mine spawns when thrown
const float CSGO_BUMP_THROW_SPAWN_OFFSET = 7.0625f;
const float CSGO_BUMP_THROW_SPEED = 500.0f;
const float CSGO_BUMP_THROW_INTERVAL_SECS = 0.5f; // Min time between bump throws
const float CSGO_BUMP_THINK_INTERVAL_SECS = 0.1f;
const float CSGO_BUMP_ELLIPSOID_WIDTH = 93.62f;
const float CSGO_BUMP_ELLIPSOID_HEIGHT = 138.11f;
const float CSGO_BUMP_ELLIPSOID_CENTER_Z_OFFSET = 51.415f;
const Magnum::Math::Vector3 CSGO_BUMP_AABB_MINS = { -81.0f, -81.0f, -81.0f };
const Magnum::Math::Vector3 CSGO_BUMP_AABB_MAXS = { +81.0f, +81.0f, +81.0f };


// Exojump gives player an upwards boost if they are pressing jump and their upwards velocity is in the correct range
const float CSGO_CONST_EXOJUMP_BOOST_RANGE_VEL_Z_MIN = 100.0f;
const float CSGO_CONST_EXOJUMP_BOOST_RANGE_VEL_Z_MAX = 500.0f;

// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/game/shared/gamemovement.cpp)

// https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/sp/src/game/shared/gamemovement.cpp#L4591-L4594
// This value is irrelevant to rampsliding: If you are currently in walk mode
// (on the ground) and start to move upwards faster than this, you enter air mode.
// In walk mode, your vertical speed always gets set to 0 and you stick to the ground
// (see gamemovement.cpp lines 2095-2097). All this does not impact the ground
// checks later on in CGameMovement::CategorizePosition(), because a separate
// threshold is used there to determine if the player is either certainly in the
// air or if ground checks must be made that could put the player back in ground mode,
// if ground is found beneath the player that is not too steep to walk on.
const float CSGO_MIN_LEAVE_GROUND_VEL_Z = 250.0f;

// https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/sp/src/game/shared/gamemovement.cpp#L3798-L3867
// If the player is moving up faster than this, they are guaranteed to not enter
// walk mode. Otherwise, the game attempts to find ground beneath the player
// that they can walk on (not too steep). If walkable ground is found, the player
// enters walk mode, otherwise they stay in air mode. Whether or not a surface
// is walkable is determined by the cvar sv_standable_normal
const float CSGO_MIN_NO_GROUND_CHECKS_VEL_Z = 140.0f;
// --------- end of source-sdk-2013 code ---------


//// TODO Move these constants and all their other occurrences somewhere else,
////      maybe into CollidableWorld?
// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/coordsize.h)
const unsigned int CSGO_COORD_INTEGER_BITS    = 14;
const unsigned int CSGO_COORD_FRACTIONAL_BITS = 5;
const size_t       CSGO_COORD_DENOMINATOR     = 1 << CSGO_COORD_FRACTIONAL_BITS;
const float        CSGO_COORD_RESOLUTION      = 1.0f / (float)CSGO_COORD_DENOMINATOR;

// this is limited by the network fractional bits used for coords
// because net coords will be only be accurate to 5 bits fractional
// Standard collision test epsilon
// 1/32nd inch collision epsilon
const float CSGO_DIST_EPSILON = 0.03125f;
// --------- end of source-sdk-2013 code ---------

} // namespace sim

#endif // SIM_CSGOCONSTANTS_H_
