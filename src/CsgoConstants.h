#ifndef CSGOCONSTANTS_H_
#define CSGOCONSTANTS_H_

#include <Magnum/Math/Angle.h>

using namespace Magnum::Math::Literals;


//// TODO Move these constant's and all their other occurrences somewhere else,
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


const float CSGO_TICKRATE = 64.0f; // Matchmaking Danger Zone server tickrate

// CSGO's horizontal FOV depends on resolution aspect ratio
// Testing indicates that CSGO's vertical FOV is between 73.7 and 73.8 degrees
// A precise FOV value here makes the image much sharper when DZSim is used as
// an overlay above CSGO!
const Magnum::Math::Deg CSGO_VERT_FOV = 73.75_degf; // CSGO's fixed vertical field of view

const float CSGO_PLAYER_HEIGHT_STANDING = 72.0f;
const float CSGO_PLAYER_HEIGHT_CROUCHED = 54.0f;
const float CSGO_PLAYER_WIDTH = 32.0f; // width and depth
const float CSGO_PLAYER_EYE_LEVEL_STANDING = 64.093811f;
const float CSGO_PLAYER_EYE_LEVEL_CROUCHING = 46.076218f;
const float CSGO_PLAYER_FEET_LEVEL = 0.031250f; // "cl_showpos 2" and "getpos_exact" show feet position

// CSGO convars' default values in the Danger Zone(!) gamemode
const float CSGO_CVAR_SV_ACCELERATE = 5.5f; // Linear acceleration amount for when player walks
const bool  CSGO_CVAR_SV_ACCELERATE_USE_WEAPON_SPEED = 1; // whether or not player acceleration is affected by carried weapons
const float CSGO_CVAR_CL_FORWARDSPEED = 450.0f;
const float CSGO_CVAR_CL_BACKSPEED = 450.0f;
const float CSGO_CVAR_CL_SIDESPEED = 450.0f;
const float CSGO_CVAR_SV_STOPSPEED = 80.0f; // Minimum stopping speed when on ground.
const float CSGO_CVAR_SV_FRICTION = 5.2f; // World friction.
const float CSGO_CVAR_SV_GRAVITY = 800.0f; // World gravity. units / (second^2)
const float CSGO_CVAR_SV_MAXSPEED = 320.0f; // Seems useless, actual max speed seems to be 260
const float CSGO_CVAR_SV_MAXVELOCITY = 3500.0f; // Maximum speed any ballistically moving object is allowed to attain per axis.
const float CSGO_CVAR_SV_STEPSIZE = 18.0f; // Hidden CSGO CVar

const float CSGO_CVAR_SV_AIRACCELERATE           = 12.0f; // no description
const float CSGO_CVAR_SV_AIRACCELERATE_PARACHUTE =  2.6f; // no description
const float CSGO_CVAR_SV_AIR_PUSHAWAY_DIST = 220.0f; // no description
const float CSGO_CVAR_SV_AIR_MAX_WISHSPEED = 30.0f;
const float CSGO_CVAR_SV_AIR_MAX_HORIZONTAL_PARACHUTE_RATIO =   0.87f;  // ratio of max hori para speed when player glides sideways?
const float CSGO_CVAR_SV_AIR_MAX_HORIZONTAL_PARACHUTE_SPEED = 240.0f;   // max horizontal speed when parachute is open
const float CSGO_CVAR_SV_PLAYER_PARACHUTE_VELOCITY = -200.0f; // downwards parachute falling speed

const float CSGO_CVAR_SV_JUMP_IMPULSE = 301.993377f; // Initial upward velocity for player jumps; sqrt(2*gravity*height).
const float CSGO_CVAR_SV_JUMP_IMPULSE_EXOJUMP_MULTIPLIER = 1.05f; // ExoJump impulse multiplier
const bool  CSGO_CVAR_SV_AUTOBUNNYHOPPING = 0; // Players automatically jump when touching ground while holding jump button
const bool  CSGO_CVAR_SV_ENABLEBUNNYHOPPING = 0; // Disables in-air movement speed cap

const float CSGO_CVAR_SV_STAMINAMAX = 80.0f; // Maximum stamina penalty
const float CSGO_CVAR_SV_STAMINARECOVERYRATE = 60.0f; // Rate at which stamina recovers (units/sec)

const float CSGO_CVAR_SV_STAMINAJUMPCOST = 0.08f; // Stamina penalty for jumping without exo legs
const float CSGO_CVAR_SV_STAMINALANDCOST = 0.05f; // Stamina penalty for landing without exo legs
const float CSGO_CVAR_SV_EXOSTAMINAJUMPCOST = 0.04f;  // Stamina penalty for jumping with exo legs
const float CSGO_CVAR_SV_EXOSTAMINALANDCOST = 0.015f; // Stamina penalty for landing with exo legs

const float CSGO_CVAR_SV_EXOJUMP_JUMPBONUS_FORWARD = 0.4f;  // ExoJump forwards velocity bonus when duck jumping
const float CSGO_CVAR_SV_EXOJUMP_JUMPBONUS_UP      = 0.58f; // ExoJump upwards bonus when holding the jump button (percentage of sv_gravity value)

const float CSGO_CVAR_SV_WATER_SWIM_MODE = 1.0f; // (def: 0) Prevent going under water
const float CSGO_CVAR_SV_WATER_MOVESPEED_MULTIPLIER = 0.5f; // (def: 0.8)

const float CSGO_CVAR_SV_WEAPON_ENCUMBRANCE_SCALE = 0.3f; // (def: 0) Encumbrance ratio to active weapon
const float CSGO_CVAR_SV_WEAPON_ENCUMBRANCE_PER_ITEM = 0.85f; // Encumbrance fixed cost

const float CSGO_CVAR_SV_LADDER_SCALE_SPEED = 0.78f; // (min. 0.0, max. 1.0) Scale top speed on ladders

const float CSGO_CVAR_HEALTHSHOT_HEALTHBOOST_DAMAGE_MULTIPLIER = 0.9f; // (def: 1) A multiplier for damage that healing player receives.
const float CSGO_CVAR_HEALTHSHOT_HEALTHBOOST_SPEED_MULTIPLIER = 1.2f; // (def: 1)
const float CSGO_CVAR_HEALTHSHOT_HEALTHBOOST_TIME = 6.5f; // (def: 0)
const bool  CSGO_CVAR_SV_HEALTH_APPROACH_ENABLED = 1; // (def: 0) Whether or not the HP are granted at once (0) or over time (1).
const float CSGO_CVAR_SV_HEALTH_APPROACH_SPEED = 10.0f; // The rate at which the healing is granted, in HP per second
const float CSGO_CVAR_SV_DZ_PLAYER_MAX_HEALTH = 120.0f;
const float CSGO_CVAR_SV_DZ_PLAYER_SPAWN_HEALTH = 120.0f;

const float CSGO_CVAR_MOLOTOV_THROW_DETONATE_TIME = 20.0f; // (def: 2)
const float CSGO_CVAR_INFERNO_MAX_RANGE = 400.0f; // (def: 150) Maximum distance flames can spread from their initial ignition point
const float CSGO_CVAR_FF_DAMAGE_REDUCTION_GRENADE_SELF = 1.0f; // How much to damage a player does to himself with his own grenade.
const float CSGO_CVAR_SV_HEGRENADE_DAMAGE_MULTIPLIER = 1.1f; // (def: 1)
const float CSGO_CVAR_SV_HEGRENADE_RADIUS_MULTIPLIER = 1.7f; // (def: 1)

const float CSGO_CVAR_SV_FALLDAMAGE_SCALE = 0.65f; // (def: 1)
const float CSGO_CVAR_SV_FALLDAMAGE_EXOJUMP_MULTIPLIER = 0.4f; // ExoJump fall damage multiplier
const float CSGO_CVAR_SV_FALLDAMAGE_TO_BELOW_PLAYER_MULTIPLIER = 1.5f; // (def. 1) Scale damage when distributed across two players
const float CSGO_CVAR_SV_FALLDAMAGE_TO_BELOW_PLAYER_RATIO = 0.6f; // (def. 0) Landing on a another player's head gives them this ratio of the damage.

const float CSGO_CVAR_SV_STANDABLE_NORMAL = 0.7f;
const float CSGO_CVAR_SV_WALKABLE_NORMAL = 0.7f; // controls what angle is surf and what you can walk on ?

const float CSGO_CVAR_SV_BUMPMINE_ARM_DELAY      = 0.3f;   // doesn't appear to have an effect ingame
const float CSGO_CVAR_SV_BUMPMINE_DETONATE_DELAY = 0.25f;  // doesn't appear to have an effect ingame

const float CSGO_CVAR_SV_DZ_ZONE_DAMAGE = 1.0f;
const float CSGO_CVAR_SV_DZ_ZONE_HEX_RADIUS = 2200.0f;

const float CSGO_CVAR_SV_TIMEBETWEENDUCKS = 0.4f; // Minimum time before recognizing consecutive duck key
const float CSGO_CVAR_SV_KNIFE_ATTACK_EXTEND_FROM_PLAYER_AABB = 10.0f; // (def: 0)

// TODO sv_shield_* and mp_shield_speed_* cvars ???
// sv_cs_player_speed_has_hostage 

// TODO cvars to investigate:
//mp_hostages_run_speed_modifier
//mp_shield_speed_deployed
//mp_shield_speed_holstered
//sv_cs_player_speed_has_hostage
//sv_ladder_scale_speed
//sv_ladder_dampen
//sv_script_think_interval
//
//sv_stepsize
//sv_wateraccelerate
//sv_waterdist
//sv_waterfriction
//
//cl_pitchdown
//cl_pitchup
//cl_pitchspeed
//cl_pdump 1 for player values
//
//r_eyewaterepsilon

// ==== HARDCODED GAME CONSTANTS ====

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
const float CSGO_CONST_MIN_LEAVE_GROUND_VEL_Z = 250.0f;

// https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/sp/src/game/shared/gamemovement.cpp#L3798-L3867
// If the player is moving up faster than this, they are guaranteed to not enter
// walk mode. Otherwise, the game attempts to find ground beneath the player
// that they can walk on (not too steep). If walkable ground is found, the player
// enters walk mode, otherwise they stay in air mode. Whether or not a surface
// is walkable is determined by the cvar sv_standable_normal
const float CSGO_CONST_MIN_NO_GROUND_CHECKS_VEL_Z = 140.0f;

// --------- end of source-sdk-2013 code ---------

#endif // CSGOCONSTANTS_H_
