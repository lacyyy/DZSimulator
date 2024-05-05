#include "sim/CsgoMovement.h"

#include <tuple>

#include <Corrade/Utility/Debug.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Functions.h>

#include "coll/Trace.h"
#include "GlobalVars.h"
#include "sim/CsgoConstants.h"
#include "utils_3d.h"

using namespace sim;
using namespace Magnum;
using namespace utils_3d;
using namespace coll;

//
// IMPORTANT NOTICE:
//
// source-sdk-2013 implements '==' and '!=' vector comparisons using _exact_
// comparisons, where 2 vectors must not differ from each other to compare equal.
//
// In contrast, Magnum::Math (used by DZSimulator) implements them using _fuzzy_
// comparisons, where 2 vectors can differ slightly and still compare equal.
//
// -> Consequently: DZSim code that replicates source-sdk-2013 code must perform
//                  these vector comparisons using the SourceSdkVectorEqual()
//                  function and NOT using the Vector's '==' and '!=' operators!
//

// Compares vectors like source-sdk-2013 does. See note above.
// Returns true if the 2 vectors are exactly equal, false otherwise.
static bool SourceSdkVectorEqual(const Vector3& vec1, const Vector3& vec2) {
    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/public/mathlib/vector.h)
    // More specifically, code is from Vector::operator==()
    return vec1.x() == vec2.x() && vec1.y() == vec2.y() && vec1.z() == vec2.z();
    // --------- end of source-sdk-2013 code ---------
}


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/const.h)

//
// Constants shared by the engine and dlls
// This header file included by engine files and DLL files.
// Most came from server.h

// CBaseEntity::m_fFlags
#define FL_ONGROUND      (1<<0) // At rest / on the ground
#define FL_DUCKING       (1<<1) // Player flag -- Player is fully crouched
#define FL_ANIMDUCKING   (1<<2) // Player flag -- Player is in the process of crouching or uncrouching but could be in transition
// examples:                                   Fully ducked:  FL_DUCKING &  FL_ANIMDUCKING
//           Previously fully ducked, unducking in progress:  FL_DUCKING & !FL_ANIMDUCKING
//                                           Fully unducked: !FL_DUCKING & !FL_ANIMDUCKING
//           Previously fully unducked, ducking in progress: !FL_DUCKING &  FL_ANIMDUCKING
#define FL_WATERJUMP     (1<<3) // player jumping out of water
#define FL_ONTRAIN       (1<<4) // Player is _controlling_ a train, so movement commands should be ignored on client during prediction.
#define FL_INRAIN        (1<<5) // Indicates the entity is standing in rain
#define FL_FROZEN        (1<<6) // Player is frozen for 3rd person camera
#define FL_ATCONTROLS    (1<<7) // Player can't move, but keeps key inputs for controlling another entity
#define FL_CLIENT        (1<<8) // Is a player
#define FL_FAKECLIENT    (1<<9) // Fake client, simulated server side; don't send network messages to them
// NON-PLAYER SPECIFIC (i.e., not used by GameMovement or the client .dll ) -- Can still be applied to players, though
#define FL_INWATER       (1<<10) // In water

// NOTE if you move things up, make sure to change this value
#define PLAYER_FLAG_BITS  11

#define FL_FLY           (1<<11) // Changes the SV_Movestep() behavior to not need to be on ground
#define FL_SWIM          (1<<12) // Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
#define FL_CONVEYOR      (1<<13)
#define FL_NPC           (1<<14)
#define FL_GODMODE       (1<<15)
#define FL_NOTARGET      (1<<16)
#define FL_AIMTARGET     (1<<17) // set if the crosshair needs to aim onto the entity
#define FL_PARTIALGROUND (1<<18) // not all corners are valid
#define FL_STATICPROP    (1<<19) // Eetsa static prop!  
#define FL_GRAPHED       (1<<20) // worldgraph has this ent listed as something that blocks a connection
#define FL_GRENADE       (1<<21)
#define FL_STEPMOVEMENT  (1<<22) // Changes the SV_Movestep() behavior to not do any processing
#define FL_DONTTOUCH     (1<<23) // Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set
#define FL_BASEVELOCITY  (1<<24) // Base velocity has been applied this frame (used to convert base velocity into momentum)
#define FL_WORLDBRUSH    (1<<25) // Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
#define FL_OBJECT        (1<<26) // Terrible name. This is an object that NPCs should see. Missiles, for example.
#define FL_KILLME        (1<<27) // This entity is marked for death -- will be freed by game DLL
#define FL_ONFIRE        (1<<28) // You know...
#define FL_DISSOLVING    (1<<29) // We're dissolving!
#define FL_TRANSRAGDOLL  (1<<30) // In the process of turning into a client side ragdoll.
#define FL_UNBLOCKABLE_BY_PLAYER (1<<31) // pusher that can't be blocked by the player

// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/game/shared/shareddefs.h)

// Height above entity position where the viewer's eye is.
const Vector3 VEC_VIEW      = { 0.0f, 0.0f, CSGO_PLAYER_EYE_LEVEL_STANDING  };
const Vector3 VEC_DUCK_VIEW = { 0.0f, 0.0f, CSGO_PLAYER_EYE_LEVEL_CROUCHING };

const float TIME_TO_DUCK   = 0.2f;
const float TIME_TO_UNDUCK = 0.001f; // Unducking on ground is instant in CSGO...

//const float PLAYER_FATAL_FALL_SPEED     = 1024.0f; // approx 60 feet
const float PLAYER_MAX_SAFE_FALL_SPEED  = 580.0f; // approx 20 feet
const float PLAYER_MIN_BOUNCE_SPEED     = 200.0f;
const float PLAYER_FALL_PUNCH_THRESHOLD = 350.0f; // won't punch player's screen/make scrape noise unless player falling at least this fast.
//const float DAMAGE_FOR_FALL_SPEED = 100.0f / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED); // damage per unit per second.
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/game/shared/gamemovement.h)
#define GAMEMOVEMENT_DUCK_TIME 1000.0f  // ms
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/mathlib/mathlib.h)
// Hermite basis function for smooth interpolation.
// Similar to Gain() above, but very cheap to call.
// value should be between 0 & 1 inclusive
inline float SimpleSpline(float value)
{
    float valueSquared = value * value;
    // Nice little ease-in, ease-out spline-like curve
    return (3 * valueSquared - 2 * valueSquared * value);
}
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/game/shared/gamemovement.cpp)
#define STOP_EPSILON    0.1
#define MAX_CLIP_PLANES 5

// [MD] I'll remove this eventually. For now, I want the ability to A/B the optimizations.
const bool g_bMovementOptimizations = true;

// Roughly how often we want to update the info about the ground surface we're on.
// We don't need to do this very often.
#define CATEGORIZE_GROUND_SURFACE_INTERVAL 0.3f
#define CATEGORIZE_GROUND_SURFACE_TICK_INTERVAL ((int)(CATEGORIZE_GROUND_SURFACE_INTERVAL / TICK_INTERVAL))

//// Purpose: Debug - draw the displacement collision plane.
//void DrawDispCollPlane(CBaseTrace* pTrace)
//{
//    float flLength = 30.0f;
//
//    // Create a basis, based on the impact normal.
//    int nMajorAxis = 0;
//    Vector vecBasisU, vecBasisV, vecNormal;
//    vecNormal = pTrace->plane.normal;
//    float flAxisValue = vecNormal[0];
//    if (fabs(vecNormal[1]) > fabs(flAxisValue)) { nMajorAxis = 1; flAxisValue = vecNormal[1]; }
//    if (fabs(vecNormal[2]) > fabs(flAxisValue)) { nMajorAxis = 2; }
//    if ((nMajorAxis == 1) || (nMajorAxis == 2))
//    {
//        vecBasisU.Init(1.0f, 0.0f, 0.0f);
//    }
//    else
//    {
//        vecBasisU.Init(0.0f, 1.0f, 0.0f);
//    }
//
//    vecBasisV = vecNormal.Cross(vecBasisU);
//    NormalizeInPlace(vecBasisV);
//
//    vecBasisU = vecBasisV.Cross(vecNormal);
//    NormalizeInPlace(vecBasisU);
//
//    // Create the impact point.  Push off the surface a bit.
//    // NOTE FOR DZSIMULATOR: CAUTION! Make sure startpos and endpos are understood
//    // and used/refactored correctly! Is startpos/endpos centered in hull or not?
//    Vector vecImpactPoint = pTrace->startpos + pTrace->fraction * (pTrace->endpos - pTrace->startpos);
//    vecImpactPoint += vecNormal;
//
//    // Generate a quad to represent the plane.
//    Vector vecPlanePoints[4];
//    vecPlanePoints[0] = vecImpactPoint + (vecBasisU * -flLength) + (vecBasisV * -flLength);
//    vecPlanePoints[1] = vecImpactPoint + (vecBasisU * -flLength) + (vecBasisV * flLength);
//    vecPlanePoints[2] = vecImpactPoint + (vecBasisU * flLength) + (vecBasisV * flLength);
//    vecPlanePoints[3] = vecImpactPoint + (vecBasisU * flLength) + (vecBasisV * -flLength);
//
//#if 0
//    // Test facing.
//    Vector vecEdges[2];
//    vecEdges[0] = vecPlanePoints[1] - vecPlanePoints[0];
//    vecEdges[1] = vecPlanePoints[2] - vecPlanePoints[0];
//    Vector vecCross = vecEdges[0].Cross(vecEdges[1]);
//    if (Math::dot(vecCross, vecNormal) < 0.0f)
//    {
//        // Reverse winding.
//    }
//#endif
//
//    // Draw the plane.
//    NDebugOverlay::Triangle(vecPlanePoints[0], vecPlanePoints[1], vecPlanePoints[2], 125, 125, 125, 125, false, 5.0f);
//    NDebugOverlay::Triangle(vecPlanePoints[0], vecPlanePoints[2], vecPlanePoints[3], 125, 125, 125, 125, false, 5.0f);
//
//    NDebugOverlay::Line(vecPlanePoints[0], vecPlanePoints[1], 255, 255, 255, false, 5.0f);
//    NDebugOverlay::Line(vecPlanePoints[1], vecPlanePoints[2], 255, 255, 255, false, 5.0f);
//    NDebugOverlay::Line(vecPlanePoints[2], vecPlanePoints[3], 255, 255, 255, false, 5.0f);
//    NDebugOverlay::Line(vecPlanePoints[3], vecPlanePoints[0], 255, 255, 255, false, 5.0f);
//
//    // Draw the normal.
//    NDebugOverlay::Line(vecImpactPoint, vecImpactPoint + (vecNormal * flLength), 255, 0, 0, false, 5.0f);
//}


// Purpose: Constructs GameMovement interface
CsgoMovement::CsgoMovement()
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    //m_nOldWaterLevel = WL_NotInWater;
    //m_flWaterEntryTime = 0;
    //m_nOnLadder = 0;
    //
    //mv = NULL;
    //
    //memset(m_flStuckCheckTime, 0, sizeof(m_flStuckCheckTime));
}

Vector3 CsgoMovement::GetPlayerMins(bool ducked) const
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!
    if (ducked) return { -0.5f * CSGO_PLAYER_WIDTH, -0.5f * CSGO_PLAYER_WIDTH, 0.0f };
    else        return { -0.5f * CSGO_PLAYER_WIDTH, -0.5f * CSGO_PLAYER_WIDTH, 0.0f };
}

Vector3 CsgoMovement::GetPlayerMaxs(bool ducked) const
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!
    if (ducked) return { +0.5f * CSGO_PLAYER_WIDTH, +0.5f * CSGO_PLAYER_WIDTH, CSGO_PLAYER_HEIGHT_CROUCHED };
    else        return { +0.5f * CSGO_PLAYER_WIDTH, +0.5f * CSGO_PLAYER_WIDTH, CSGO_PLAYER_HEIGHT_STANDING };
}

Vector3 CsgoMovement::GetPlayerViewOffset(bool ducked) const
{
    return ducked ? VEC_DUCK_VIEW : VEC_VIEW;
}

Vector3 CsgoMovement::GetPlayerCenter(bool ducked) const
{
    Vector3 center = m_vecAbsOrigin;
    center.z() += 0.5f * GetPlayerMaxs(ducked).z();
    return center;
}

void CsgoMovement::CategorizeGroundSurface(int16_t surface/*const Trace& pm*/)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    // Observations from "cl_pdump 1" command:
    // NOTE: m_surfaceFriction is 1.0 on normal surfaces
    // NOTE: m_surfaceFriction is 0.25 temporarily when jumping???
    // NOTE: m_surfaceFriction is 0.625 on dz_frostbite's ice lake.
    // NOTE: m_surfaceFriction is 0.625 on ANY glass window.
    // NOTE: m_surfaceFriction is 0.625 on ANY car wind shield.

    // FIXME We are not detecting the surface we stand on at the moment
    m_surfaceFriction = 1.0f;
    return;

    // Take a look at https://developer.valvesoftware.com/wiki/List_of_CS:GO_Surface_Types

    ////IPhysicsSurfaceProps* physprops = MoveHelper()->GetSurfaceProps();
    ////player->m_surfaceProps = pm.surface.surfaceProps;
    ////player->m_pSurfaceData = physprops->GetSurfaceData(player->m_surfaceProps);
    ////physprops->GetPhysicsProperties(player->m_surfaceProps, NULL, NULL, &m_surfaceFriction, NULL);
    ////
    ////// HACKHACK: Scale this to fudge the relationship between vphysics friction values and player friction values.
    ////// A value of 0.8f feels pretty normal for vphysics, whereas 1.0f is normal for players.
    ////// This scaling trivially makes them equivalent.  REVISIT if this affects low friction surfaces too much.
    ////player->m_surfaceFriction *= 1.25f;
    ////if (player->m_surfaceFriction > 1.0f)
    ////    player->m_surfaceFriction = 1.0f;
    ////
    ////player->m_chTextureType = player->m_pSurfaceData->game.material;
}

void CsgoMovement::CheckParameters()
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    //Vector3 v_angle;

    if (m_MoveType != MOVETYPE_NOCLIP)
    {
        float spd;

        spd =
            (m_flForwardMove * m_flForwardMove) +
            (m_flSideMove * m_flSideMove)/* +
            (mv->m_flUpMove * mv->m_flUpMove)*/;

        const float m_flClientMaxSpeed = 320.0f; // Is this the sv_maxspeed value? def: 320
        if (m_flClientMaxSpeed != 0.0)
        {
            m_flMaxSpeed = Math::min(m_flClientMaxSpeed, m_flMaxSpeed);
        }

        // Slow down by the speed factor
        float flSpeedFactor = 1.0f;

        // ...

        m_flMaxSpeed *= flSpeedFactor;

        if (g_bMovementOptimizations)
        {
            // Same thing but only do the sqrt if we have to.
            if ((spd != 0.0) && (spd > m_flMaxSpeed * m_flMaxSpeed))
            {
                float fRatio = m_flMaxSpeed / sqrt(spd);
                m_flForwardMove *= fRatio;
                m_flSideMove *= fRatio;
            }
        }
        else
        {
            spd = sqrt(spd);
            if ((spd != 0.0) && (spd > m_flMaxSpeed))
            {
                float fRatio = m_flMaxSpeed / spd;
                m_flForwardMove *= fRatio;
                m_flSideMove *= fRatio;
            }
        }
    }
}

void CsgoMovement::ReduceTimers(float time_delta)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    float frame_msec = 1000.0f * time_delta;

    if (m_flDucktime > 0)
    {
        m_flDucktime -= frame_msec;
        if (m_flDucktime < 0)
            m_flDucktime = 0;
    }
}


// Purpose: Sets ground entity
void CsgoMovement::FinishMove(void)
{
    m_nOldButtons = m_nButtons;
}

void CsgoMovement::StartGravity(float frametime)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    float ent_gravity = 1.0;

    // Add gravity so they'll be in the correct position during movement
    // yes, this 0.5 looks wrong, but it's not.  
    m_vecVelocity.z() -=
        ent_gravity * g_csgo_game_sim_cfg.sv_gravity * 0.5f * frametime;

    m_vecVelocity.z() += m_vecBaseVelocity.z() * frametime;
    m_vecBaseVelocity.z() = 0;

    // Give player equipped with exo an upwards jump bonus when holding space
    if (m_loadout.has_exojump
        && !m_hGroundEntity
        && (m_nButtons & IN_JUMP)
        && m_vecVelocity.z() >= CSGO_CONST_EXOJUMP_BOOST_RANGE_VEL_Z_MIN
        && m_vecVelocity.z() <= CSGO_CONST_EXOJUMP_BOOST_RANGE_VEL_Z_MAX)
    {
        m_vecVelocity.z() += g_csgo_game_sim_cfg.sv_exojump_jumpbonus_up *
                             g_csgo_game_sim_cfg.sv_gravity *
                             frametime;
    }

    CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: Does the basic move attempting to climb up step heights.  It uses
//          the m_vecAbsOrigin and m_vecVelocity.  It returns a new
//          new m_vecAbsOrigin, m_vecVelocity, and mv->m_outStepHeight.
//-----------------------------------------------------------------------------
void CsgoMovement::StepMove(float frametime,
    const Vector3& vecDestination, const Trace& trace)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    Vector3 vecEndPos;
    vecEndPos = vecDestination;

    // Try sliding forward both on ground and up 16 pixels
    //  take the move that goes farthest
    Vector3 vecPos, vecVel;
    vecPos = m_vecAbsOrigin;
    vecVel = m_vecVelocity;

    // Slide move down.
    TryPlayerMove(frametime, &vecEndPos, &trace);

    // Down results.
    Vector3 vecDownPos, vecDownVel;
    vecDownPos = m_vecAbsOrigin;
    vecDownVel = m_vecVelocity;

    // Reset original values.
    m_vecAbsOrigin = vecPos;
    m_vecVelocity = vecVel;

    // Move up a stair height.
    vecEndPos = m_vecAbsOrigin;
    if (m_bAllowAutoMovement)
    {
        vecEndPos.z() += g_csgo_game_sim_cfg.sv_stepsize + CSGO_DIST_EPSILON;
    }

    Trace trace_up = TracePlayerBBox(m_vecAbsOrigin, vecEndPos);
    if (!trace_up.results.startsolid && !trace_up.results.allsolid)
    {
        m_vecAbsOrigin = m_vecAbsOrigin + trace_up.results.fraction * trace_up.info.delta;
    }

    // Slide move up.
    TryPlayerMove(frametime);

    // Move down a stair (attempt to).
    vecEndPos = m_vecAbsOrigin;
    if (m_bAllowAutoMovement)
    {
        vecEndPos.z() -= g_csgo_game_sim_cfg.sv_stepsize + CSGO_DIST_EPSILON;
    }

    Trace trace_down = TracePlayerBBox(m_vecAbsOrigin, vecEndPos);

    // If we are not on the ground any more then use the original movement attempt.
    if (trace_down.results.plane_normal.z() < g_csgo_game_sim_cfg.sv_walkable_normal)
    {
        m_vecAbsOrigin = vecDownPos;
        m_vecVelocity = vecDownVel;
        //float flStepDist = m_vecAbsOrigin.z() - vecPos.z();
        //if (flStepDist > 0.0f)
        //{
        //    m_outStepHeight += flStepDist;
        //}
        return;
    }

    // If the trace ended up in empty space, copy the end over to the origin.
    if (!trace_down.results.startsolid && !trace_down.results.allsolid)
    {
        m_vecAbsOrigin = m_vecAbsOrigin + trace_down.results.fraction * trace_down.info.delta;
    }

    // Copy this origin to up.
    Vector3 vecUpPos;
    vecUpPos = m_vecAbsOrigin;

    // decide which one went farther
    float flDownDist = (vecDownPos.x() - vecPos.x()) * (vecDownPos.x() - vecPos.x())
                     + (vecDownPos.y() - vecPos.y()) * (vecDownPos.y() - vecPos.y());
    float flUpDist = (vecUpPos.x() - vecPos.x()) * (vecUpPos.x() - vecPos.x())
                   + (vecUpPos.y() - vecPos.y()) * (vecUpPos.y() - vecPos.y());
    if (flDownDist > flUpDist)
    {
        m_vecAbsOrigin = vecDownPos;
        m_vecVelocity = vecDownVel;
    }
    else
    {
        // copy z value from slide move
        m_vecVelocity.z() = vecDownVel.z();
    }

    //float flStepDist = m_vecAbsOrigin.z() - vecPos.z();
    //if (flStepDist > 0)
    //{
    //    m_outStepHeight += flStepDist;
    //}
}

void CsgoMovement::Friction(float frametime)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    float speed, newspeed, control;
    float friction;
    float drop;

    //// If we are in water jump cycle, don't apply friction
    //if (m_flWaterJumpTime)
    //    return;

    // Calculate speed
    speed = m_vecVelocity.length();

    // If too slow, return
    if (speed < 0.1f)
        return;

    drop = 0;

    // apply ground friction
    if (m_hGroundEntity)  // On an entity that is the ground
    {
        friction = g_csgo_game_sim_cfg.sv_friction * m_surfaceFriction;

        // Bleed off some speed, but if we have less than the bleed
        //  threshold, bleed the threshold amount.

        control = (speed < g_csgo_game_sim_cfg.sv_stopspeed) ?
            g_csgo_game_sim_cfg.sv_stopspeed : speed;

        // Add the amount to the drop amount.
        drop += control * friction * frametime;
    }

    // scale the velocity
    newspeed = speed - drop;
    if (newspeed < 0)
        newspeed = 0;

    if (newspeed != speed)
    {
        // Determine proportion of old speed we are using.
        newspeed /= speed;
        // Adjust velocity according to proportion.
        m_vecVelocity *= newspeed;
    }

    //m_outWishVel -= (1.f - newspeed) * m_vecVelocity;
}

void CsgoMovement::FinishGravity(float frametime)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    float ent_gravity = 1.0f;

    //if (m_flWaterJumpTime)
    //    return;

    // Get the correct velocity for the end of the dt 
    m_vecVelocity.z() -=
        ent_gravity * g_csgo_game_sim_cfg.sv_gravity * 0.5f * frametime;

    CheckVelocity();
}

void CsgoMovement::AirAccelerate(float frametime,
    const Vector3& wishdir, float wishspeed, float accel)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    int i;
    float addspeed, accelspeed, currentspeed;
    float wishspd;

    wishspd = wishspeed;

    //if (player->pl.deadflag)
    //    return;

    //if (player->m_flWaterJumpTime)
    //    return;

    // Cap speed
    if (wishspd > g_csgo_game_sim_cfg.sv_air_max_wishspeed)
        wishspd = g_csgo_game_sim_cfg.sv_air_max_wishspeed;

    // Determine veer amount
    currentspeed = Math::dot(m_vecVelocity, wishdir);

    // See how much to add
    addspeed = wishspd - currentspeed;

    // If not adding any, done.
    if (addspeed <= 0.0f)
        return;

    // Determine acceleration speed after acceleration
    accelspeed = accel * wishspeed * frametime * m_surfaceFriction;

    // Cap it
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    // Adjust pmove vel.
    for (i = 0; i < 3; i++)
    {
        m_vecVelocity[i] += accelspeed * wishdir[i];
        //m_outWishVel[i] += accelspeed * wishdir[i];
    }
}

void CsgoMovement::AirMove(float frametime)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    int     i;
    Vector3 wishvel;
    float   fmove, smove;
    Vector3 wishdir;
    float   wishspeed;
    Vector3 forward, right, up;

    // Determine movement angles
    AnglesToVectors(m_vecViewAngles, &forward, &right, &up);

    // Copy movement amounts
    fmove = m_flForwardMove;
    smove = m_flSideMove;

    // Zero out z components of movement vectors
    forward.z() = 0;
    right.z() = 0;
    NormalizeInPlace(forward); // Normalize remainder of vectors
    NormalizeInPlace(right);   // 

    for (i = 0; i < 2; i++)       // Determine x and y parts of velocity
        wishvel[i] = forward[i] * fmove + right[i] * smove;

    wishvel.z() = 0;             // Zero out z part of velocity

    // Determine maginitude of speed of move
    wishdir = wishvel;
    wishspeed = NormalizeInPlace(wishdir);

    //
    // clamp to server defined max speed
    //
    if (wishspeed != 0 && wishspeed > m_flMaxSpeed)
    {
        wishvel *= m_flMaxSpeed / wishspeed;
        wishspeed = m_flMaxSpeed;
    }

    AirAccelerate(frametime, wishdir, wishspeed, g_csgo_game_sim_cfg.sv_airaccelerate);

    // Add in any base velocity to the current velocity.
    m_vecVelocity += m_vecBaseVelocity;

    TryPlayerMove(frametime);

    // Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
    m_vecVelocity -= m_vecBaseVelocity;
}


bool CsgoMovement::CanAccelerate()
{
    //// Dead players don't accelerate.
    //if (player->pl.deadflag)
    //    return false;

    //// If waterjumping, don't accelerate
    //if (m_flWaterJumpTime)
    //    return false;

    return true;
}

void CsgoMovement::Accelerate(Vector3& wishdir, float wishspeed, float accel, float frametime)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    float addspeed, accelspeed, currentspeed;

    // This gets overridden because some games (CSPort) want to allow dead (observer) players
    // to be able to move around.
    if (!CanAccelerate())
        return;

    // See if we are changing direction a bit
    currentspeed = Math::dot(m_vecVelocity, wishdir);

    // Reduce wishspeed by the amount of veer.
    addspeed = wishspeed - currentspeed;

    // If not going to add any speed, done.
    if (addspeed <= 0)
        return;

    // Determine amount of acceleration.
    accelspeed = accel * frametime * wishspeed * m_surfaceFriction;

    // Cap at addspeed
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    // Adjust velocity.
    for (int i = 0; i < 3; i++)
        m_vecVelocity[i] += accelspeed * wishdir[i];
}

// Purpose: Try to keep a walking player on the ground when running down slopes etc
void CsgoMovement::StayOnGround()
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    Vector3 start = m_vecAbsOrigin;
    Vector3 end = m_vecAbsOrigin;
    start.z() += 2;
    end.z() -= g_csgo_game_sim_cfg.sv_stepsize;

    // See how far up we can go without getting stuck

    Trace up_trace = TracePlayerBBox(m_vecAbsOrigin, start);
    start = m_vecAbsOrigin + up_trace.results.fraction * up_trace.info.delta;

    // using trace.startsolid is unreliable here, it doesn't get set when
    // tracing bounding box vs. terrain

    // Now trace down from a known safe position
    Trace down_trace = TracePlayerBBox(start, end);
    if (down_trace.results.fraction > 0.0f && // must go somewhere
        down_trace.results.fraction < 1.0f && // must hit something
        !down_trace.results.startsolid &&     // can't be embedded in a solid
        down_trace.results.plane_normal.z() >= g_csgo_game_sim_cfg.sv_standable_normal) // can't hit a steep slope that we can't stand on anyway
    {
        Vector3 endpos = start + down_trace.results.fraction * down_trace.info.delta;
        float flDelta = Math::abs(m_vecAbsOrigin.z() - endpos.z());

        //This is incredibly hacky. The real problem is that trace returning that strange value we can't network over.
        if (flDelta > 0.5f * CSGO_COORD_RESOLUTION)
        {
            m_vecAbsOrigin = endpos;
        }
    }
}

void CsgoMovement::WalkMove(float frametime)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    Vector3 wishvel;
    float spd;
    float fmove, smove;
    Vector3 wishdir;
    float wishspeed;

    Vector3 dest;
    Vector3 forward, right, up;

    AnglesToVectors(m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

    // Copy movement amounts
    fmove = m_flForwardMove;
    smove = m_flSideMove;

    // Zero out z components of movement vectors
    if (g_bMovementOptimizations)
    {
        if (forward.z() != 0)
        {
            forward.z() = 0;
            NormalizeInPlace(forward);
        }

        if (right.z() != 0)
        {
            right.z() = 0;
            NormalizeInPlace(right);
        }
    }
    else
    {
        forward.z() = 0;
        right.z() = 0;

        NormalizeInPlace(forward); // Normalize remainder of vectors.
        NormalizeInPlace(right);   // 
    }

    for (int i = 0; i < 2; i++)       // Determine x and y parts of velocity
        wishvel[i] = forward[i] * fmove + right[i] * smove;

    wishvel.z() = 0;             // Zero out z part of velocity

    wishdir = wishvel;   // Determine maginitude of speed of move
    wishspeed = NormalizeInPlace(wishdir);

    //
    // Clamp to server defined max speed
    //
    if ((wishspeed != 0.0f) && (wishspeed > m_flMaxSpeed))
    {
        wishvel *= m_flMaxSpeed / wishspeed;
        wishspeed = m_flMaxSpeed;
    }

    // Set pmove velocity
    m_vecVelocity.z() = 0;
    Accelerate(wishdir, wishspeed, g_csgo_game_sim_cfg.sv_accelerate, frametime);
    m_vecVelocity.z() = 0;

    // Add in any base velocity to the current velocity.
    m_vecVelocity += m_vecBaseVelocity;

    spd = m_vecVelocity.length();

    if (spd < 1.0f)
    {
        m_vecVelocity = { 0.0f, 0.0f, 0.0f };
        // Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
        m_vecVelocity -= m_vecBaseVelocity;
        return;
    }

    // first try just moving to the destination
    dest.x() = m_vecAbsOrigin.x() + m_vecVelocity.x() * frametime;
    dest.y() = m_vecAbsOrigin.y() + m_vecVelocity.y() * frametime;
    dest.z() = m_vecAbsOrigin.z();

    // first try moving directly to the next spot
    Trace tr = TracePlayerBBox(m_vecAbsOrigin, dest);

    // If we made it all the way, then copy trace end as new player position.
    //m_outWishVel += wishdir * wishspeed;

    if (tr.results.fraction == 1.0f)
    {
        m_vecAbsOrigin = dest;
        // Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
        m_vecVelocity -= m_vecBaseVelocity;

        StayOnGround();
        return;
    }

    // Don't walk up stairs if not on ground.
    if (!m_hGroundEntity /*&& player->GetWaterLevel() == 0*/)
    {
        // Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
        m_vecVelocity -= m_vecBaseVelocity;
        return;
    }

    //// If we are jumping out of water, don't do anything more.
    //if (player->m_flWaterJumpTime)
    //{
    //    // Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
    //    m_vecVelocity -= m_vecBaseVelocity;
    //    return;
    //}

    StepMove(frametime, dest, tr);

    // Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
    m_vecVelocity -= m_vecBaseVelocity;

    StayOnGround();
}

void CsgoMovement::FullWalkMove(float frametime)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    Vector3 velocity_at_move_start = m_vecVelocity;

    if (true /*!CheckWater()*/)
    {
        StartGravity(frametime);
    }

    // If we are leaping out of the water, just update the counters.
    ////if (player->m_flWaterJumpTime)
    ////{
    ////    WaterJump();
    ////    TryPlayerMove(frametime);
    ////    // See if we are still in water?
    ////    //CheckWater();
    ////    return;
    ////}

    // If we are swimming in the water, see if we are nudging against a place we can jump up out
    //  of, and, if so, start out jump.  Otherwise, if we are not moving up, then reset jump timer to 0
    if (false /*player->GetWaterLevel() >= WL_Waist*/)
    {
        //if (player->GetWaterLevel() == WL_Waist)
        //{
        //    CheckWaterJump();
        //}
        //
        //// If we are falling again, then we must not trying to jump out of water any more.
        //if (m_vecVelocity.z() < 0 &&
        //    player->m_flWaterJumpTime)
        //{
        //    player->m_flWaterJumpTime = 0;
        //}
        //
        //// Was jump button pressed?
        //if (mv->m_nButtons & IN_JUMP)
        //{
        //    CheckJumpButton(frametime);
        //}
        //else
        //{
        //    mv->m_nOldButtons &= ~IN_JUMP;
        //}
        //
        //// Perform regular water movement
        //WaterMove();
        //
        //// Redetermine position vars
        //CategorizePosition();
        //
        //// If we are on ground, no downward velocity.
        //if (player->GetGroundEntity() != NULL)
        //{
        //    m_vecVelocity.z() = 0;
        //}
    }
    else
        // Not fully underwater
    {
        // Was jump button pressed?
        if (m_nButtons & IN_JUMP)
        {
            CheckJumpButton(frametime);
        }
        else
        {
            m_nOldButtons &= ~IN_JUMP;
        }

        // Friction is handled before we add in any base velocity. That way, if we are on a conveyor,
        //  we don't slow when standing still, relative to the conveyor.
        if (m_hGroundEntity)
        {
            m_vecVelocity.z() = 0.0;
            Friction(frametime);
        }

        // Make sure velocity is valid.
        CheckVelocity();

        if (m_hGroundEntity)
        {
            WalkMove(frametime);
        }
        else
        {
            AirMove(frametime);  // Take into account movement when in air.
        }

        // Set final flags.
        bool was_onground_before = m_hGroundEntity;
        CategorizePosition();
        bool is_onground_now = m_hGroundEntity;
        bool landed_onground_just_now = !was_onground_before && is_onground_now;

        // Make sure velocity is valid.
        CheckVelocity();

        // Add any remaining gravitational component.
        if (true /*!CheckWater()*/)
        {
            FinishGravity(frametime);
        }

        // If we are on ground, no downward velocity.
        if (m_hGroundEntity)
        {
            m_vecVelocity.z() = 0;
        }
        
        if (landed_onground_just_now)
        {
            const float HIGH_LANDING_SPEED = 1000.0f;

            float hori_speed_limit;
            if (velocity_at_move_start.length() > HIGH_LANDING_SPEED)
                // CSGO stops the player hard when landing on ground with high speed
                hori_speed_limit = 80.0f;
            else
                // At lower landing speeds, competitive CSGO seems to cap your
                // horizontal speed at 286 when touching ground (in order to
                // limit bunny hopping).
                // It might be an oversimplification, but it seems good enough.
                hori_speed_limit = 286.0f;

            // Limit horizontal speed
            float hori_speed = m_vecVelocity.xy().length();
            if (hori_speed > hori_speed_limit) {
                m_vecVelocity.x() *= hori_speed_limit / hori_speed;
                m_vecVelocity.y() *= hori_speed_limit / hori_speed;
            }
        }

        CheckFalling();
    }

    /*if ((m_nOldWaterLevel == WL_NotInWater && player->GetWaterLevel() != WL_NotInWater) ||
        (m_nOldWaterLevel != WL_NotInWater && player->GetWaterLevel() == WL_NotInWater))
    {
        PlaySwimSound();
        player->Splash();
    }*/
}

bool CsgoMovement::CheckJumpButton(float frametime)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    //if (player->pl.deadflag)
    //{
    //    mv->m_nOldButtons |= IN_JUMP; // don't jump again until released
    //    return false;
    //}

    // See if we are waterjumping.  If so, decrement count and return.
    //if (m_flWaterJumpTime)
    //{
    //    m_flWaterJumpTime -= gpGlobals->frametime;
    //    if (player->m_flWaterJumpTime < 0)
    //        player->m_flWaterJumpTime = 0;
    //
    //    return false;
    //}

    // If we are in the water most of the way...
    //if (player->GetWaterLevel() >= 2)
    //{
    //    // swimming, not jumping
    //    SetGroundEntity(false);
    //
    //    if (player->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
    //        m_vecVelocity.z() = 100;
    //    else if (player->GetWaterType() == CONTENTS_SLIME)
    //        m_vecVelocity.z() = 80;
    //
    //    // play swiming sound
    //    if (player->m_flSwimSoundTime <= 0)
    //    {
    //        // Don't play sound again for 1 second
    //        player->m_flSwimSoundTime = 1000;
    //        PlaySwimSound();
    //    }
    //
    //    return false;
    //}

    // No more effect
    if (!m_hGroundEntity)
    {
        m_nOldButtons |= IN_JUMP;
        return false; // in air, so no effect
    }

    if (m_nOldButtons & IN_JUMP)
        return false; // don't pogo stick

    // In the air now.
    SetGroundEntity(false);

    //player->PlayStepSound((Vector&)m_vecAbsOrigin, player->m_pSurfaceData, 1.0, true);
    //MoveHelper()->PlayerSetAnimation(PLAYER_JUMP);

    // Initial upward velocity for player jumps; sqrt(2*gravity*height).
    float flMul = g_csgo_game_sim_cfg.sv_jump_impulse;

    if (m_loadout.has_exojump)
        flMul *= g_csgo_game_sim_cfg.sv_jump_impulse_exojump_multiplier;

    // Accelerate upward
    // If we are ducking...
    float startz = m_vecVelocity.z();
    if (m_bDucking || (m_fFlags & FL_DUCKING))
    {
        // d = 0.5 * g * t^2      - distance traveled with linear accel
        // t = sqrt(2.0 * 45 / g) - how long to fall 45 units
        // v = g * t              - velocity at the end (just invert it to jump up that high)
        // v = g * sqrt(2.0 * 45 / g )
        // v^2 = g * g * 2.0 * 45 / g
        // v = sqrt( g * 2.0 * 45 )
        m_vecVelocity.z() = flMul;  // 2 * gravity * height
    }
    else
    {
        m_vecVelocity.z() += flMul;  // 2 * gravity * height
    }

    // If duck-jumping with exo, add forwards/horizontal exo boost
    if (m_loadout.has_exojump && !m_bDucked && m_bDucking)
        ApplyForwardsExoBoost();

    FinishGravity(frametime);

    m_outJumpVel.z() += m_vecVelocity.z() - startz;
    //m_outStepHeight += 0.15f;

    OnJump(m_outJumpVel.z());

    // Flag that we jumped.
    m_nOldButtons |= IN_JUMP; // don't jump again until released
    return true;
}

int CsgoMovement::TryPlayerMove(float frametime,
    const Vector3* pFirstDest, const Trace* pFirstTrace)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    int     bumpcount, numbumps;
    Vector3 dir;
    float   d;
    int     numplanes;
    Vector3 planes[MAX_CLIP_PLANES];
    Vector3 primal_velocity, original_velocity;
    Vector3 new_velocity;
    int     i, j;
    Vector3 end;
    float   time_left, allFraction;
    int     blocked;

    numbumps = 4;           // Bump up to four times

    blocked = 0;           // Assume not blocked
    numplanes = 0;           //  and not sliding along any planes

    original_velocity = m_vecVelocity; // Store original velocity
    primal_velocity = m_vecVelocity;

    allFraction = 0;
    time_left = frametime;   // Total time for this movement operation.

    new_velocity = { 0.0f, 0.0f, 0.0f };

    for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
    {
        if (m_vecVelocity.length() == 0.0f)
            break;

        // Assume we can move all the way from the current origin to the
        //  end point.
        end = m_vecAbsOrigin + time_left * m_vecVelocity;

        // See if we can make it from origin to end point.
        Trace tr = (g_bMovementOptimizations &&
            // If their velocity Z is 0, then we can avoid an extra trace here during WalkMove.
            pFirstDest && pFirstTrace && SourceSdkVectorEqual(end, *pFirstDest)) ?
                *pFirstTrace // Copy identical previous trace
                :
                TracePlayerBBox(m_vecAbsOrigin, end);


        allFraction += tr.results.fraction;

        // If we started in a solid object, or we were in solid space
        //  the whole way, zero out our velocity and return that we
        //  are blocked by floor and wall.
        if (tr.results.allsolid)
        {
            // entity is trapped in another solid
            m_vecVelocity = { 0.0f, 0.0f, 0.0f };
            return 4;
        }

        // If we moved some portion of the total distance, then
        //  copy the end position into the pmove.origin and 
        //  zero the plane counter.
        if (tr.results.fraction > 0)
        {
            Vector3 reached_endpos = tr.info.startpos + tr.info.startoffset
                + tr.results.fraction * tr.info.delta;

            if (numbumps > 0 && tr.results.fraction == 1)
            {
                // There's a precision issue with terrain tracing that can cause a swept box to successfully trace
                // when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
                // case until the bug is fixed. (Narrator: It was never fixed)
                // If we detect getting stuck, don't allow the movement
                Trace stuck = TracePlayerBBox(reached_endpos, reached_endpos);
                if (stuck.results.startsolid || stuck.results.fraction != 1.0f)
                {
                    //Msg( "Player will become stuck!!!\n" );
                    m_vecVelocity = { 0.0f, 0.0f, 0.0f };
                    break;
                }
            }

            // actually covered some distance
            m_vecAbsOrigin = reached_endpos;
            original_velocity = m_vecVelocity;
            numplanes = 0;
        }

        // If we covered the entire distance, we are done
        //  and can return.
        if (tr.results.fraction == 1.0f)
        {
            break; // moved the entire distance
        }

        //// Save entity that blocked us (since fraction was < 1.0)
        ////  for contact
        //// Add it if it's not already in the list!!!
        //MoveHelper()->AddToTouched(pm, m_vecVelocity);

        // If the plane we hit has a high z component in the normal, then
        //  it's probably a floor
        if (tr.results.plane_normal.z() > g_csgo_game_sim_cfg.sv_standable_normal)
        {
            blocked |= 1; // floor
        }
        // If the plane has a zero z component in the normal, then it's a 
        //  step or wall
        if (!tr.results.plane_normal.z())
        {
            blocked |= 2; // step / wall
        }

        // Reduce amount of m_flFrameTime left by total time left * fraction
        //  that we covered.
        time_left -= time_left * tr.results.fraction;

        // Did we run out of planes to clip against?
        if (numplanes >= MAX_CLIP_PLANES)
        {
            // this shouldn't really happen
            //  Stop our movement if so.
            m_vecVelocity = { 0.0f, 0.0f, 0.0f };
            //Con_DPrintf("Too many planes 4\n");

            break;
        }

        // Set up next clipping plane
        planes[numplanes] = tr.results.plane_normal;
        numplanes++;

        // modify original_velocity so it parallels all of the clip planes
        //

        // reflect player velocity 
        // Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
        //  and pressing forward and nobody was really using this bounce/reflection feature anyway...
        if (numplanes == 1 && m_MoveType == MOVETYPE_WALK && !m_hGroundEntity)
        {
            for (i = 0; i < numplanes; i++)
            {
                if (planes[i].z() > g_csgo_game_sim_cfg.sv_standable_normal)
                {
                    // floor or slope
                    ClipVelocity(original_velocity, planes[i], new_velocity, 1.0f);
                    original_velocity = new_velocity;
                }
                else
                {
                    ClipVelocity(original_velocity, planes[i], new_velocity, 1.0f);
                }
            }

            m_vecVelocity = new_velocity;
            original_velocity = new_velocity;
        }
        else
        {
            for (i = 0; i < numplanes; i++)
            {
                ClipVelocity(original_velocity, planes[i], m_vecVelocity, 1.0f);

                for (j = 0; j < numplanes; j++)
                    if (j != i)
                    {
                        // Are we now moving against this plane?
                        if (Math::dot(m_vecVelocity, planes[j]) < 0)
                            break; // not ok
                    }
                if (j == numplanes)  // Didn't have to clip, so we're ok
                    break;
            }

            // Did we go all the way through plane set
            if (i != numplanes)
            {   // go along this plane
                // pmove.velocity is set in clipping call, no need to set again.
                ;
            }
            else
            {   // go along the crease
                if (numplanes != 2)
                {
                    m_vecVelocity = { 0.0f, 0.0f, 0.0f };
                    break;
                }
                dir = Math::cross(planes[0], planes[1]);
                NormalizeInPlace(dir);
                d = Math::dot(dir, m_vecVelocity);
                m_vecVelocity = d * dir;
            }

            //
            // if original velocity is against the original velocity, stop dead
            // to avoid tiny occilations in sloping corners
            //
            d = Math::dot(m_vecVelocity, primal_velocity);
            if (d <= 0)
            {
                //Con_DPrintf("Back\n");
                m_vecVelocity = { 0.0f, 0.0f, 0.0f };
                break;
            }
        }
    }

    if (allFraction == 0)
    {
        m_vecVelocity = { 0.0f, 0.0f, 0.0f };
    }

    //// Check if they slammed into a wall
    //float fSlamVol = 0.0f;
    //
    //float fLateralStoppingAmount = primal_velocity.xy().length() - m_vecVelocity.xy().length();
    //if (fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED * 2.0f)
    //{
    //    fSlamVol = 1.0f;
    //}
    //else if (fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED)
    //{
    //    fSlamVol = 0.85f;
    //}
    //
    //PlayerRoughLandingEffects(fSlamVol);

    return blocked;
}

void CsgoMovement::CheckVelocity(void)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    // bound velocity
    Vector3 org = m_vecAbsOrigin;
    for (int i = 0; i < 3; i++)
    {
        // See if it's bogus.
        if (Math::isNan(m_vecVelocity[i]))
        {
            Debug{} << "[GameMovement] ERROR: Got a NaN velocity on axis" << i
                << ". ->" << m_vecVelocity;
            m_vecVelocity[i] = 0.0f;
        }

        if (Math::isNan(m_vecAbsOrigin[i]))
        {
            Debug{} << "[GameMovement] ERROR: Got a NaN origin on axis" << i
                << ". ->" << m_vecAbsOrigin;
            m_vecAbsOrigin[i] = 0.0f;
        }

        // Bound it.
        if (m_vecVelocity[i] > g_csgo_game_sim_cfg.sv_maxvelocity)
        {
            Debug{} << "[GameMovement] WARNING: Got a velocity too high on axis"
                << i << ". ->" << m_vecVelocity;
            m_vecVelocity[i] = g_csgo_game_sim_cfg.sv_maxvelocity;
        }
        else if (m_vecVelocity[i] < -g_csgo_game_sim_cfg.sv_maxvelocity)
        {
            Debug{} << "[GameMovement] WARNING: Got a velocity too low on axis"
                << i << ". ->" << m_vecVelocity;
            m_vecVelocity[i] = -g_csgo_game_sim_cfg.sv_maxvelocity;
        }
    }
}

int CsgoMovement::ClipVelocity(const Vector3& in, const Vector3& normal, Vector3& out, float overbounce)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    float backoff;
    float change;
    float angle;
    int   i, blocked;

    angle = normal.z();

    blocked = 0x00;      // Assume unblocked.
    if (angle > 0)       // If the plane that is blocking us has a positive z component, then assume it's a floor.
        blocked |= 0x01; // 
    if (!angle)          // If the plane has no Z, it is vertical (wall/step)
        blocked |= 0x02; // 


    // Determine how far along plane to slide based on incoming direction.
    backoff = Math::dot(in, normal) * overbounce;

    for (i = 0; i < 3; i++)
    {
        change = normal[i] * backoff;
        out[i] = in[i] - change;
    }

    // iterate once to make sure we aren't still moving through the plane
    float adjust = Math::dot(out, normal);
    if (adjust < 0.0f)
    {
        out -= (normal * adjust);
        // Msg( "Adjustment = %lf\n", adjust );
    }

    // Return blocking flags.
    return blocked;
}

void CsgoMovement::SetGroundEntity(bool has_ground, int16_t surface)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    bool newGround = has_ground;
    bool oldGround = m_hGroundEntity;

    // Note: Ground velocity is always 0 in DZSimulator
    if (!oldGround && newGround) {
        // Subtract ground velocity at instant we hit ground jumping
        //m_vecBaseVelocity -= newGround->GetAbsVelocity(); 
        m_vecBaseVelocity.z() = 0.0f;// newGround->GetAbsVelocity().z();
    }
    else if (oldGround && !newGround) {
        // Add in ground velocity at instant we started jumping
        //m_vecBaseVelocity += oldGround->GetAbsVelocity();
        m_vecBaseVelocity.z() = 0.0f;// oldGround->GetAbsVelocity().z();
    }

    // Set ground entity
    m_hGroundEntity = newGround;

    // If we are on something...
    if (newGround)
    {
        CategorizeGroundSurface(surface/*pm*/);

        // Then we are not in water jump sequence
        //player->m_flWaterJumpTime = 0;

        // Standing on an entity other than the world, so signal that we are touching something.
        //if (!pm->DidHitWorld())
        //{
        //    MoveHelper()->AddToTouched(*pm, m_vecVelocity);
        //}

        m_vecVelocity.z() = 0.0f;
    }
}

//// source-sdk-2013 description:
////-----------------------------------------------------------------------------
//// Traces the player's collision bounds in quadrants, looking for a plane that
//// can be stood upon (normal's z >= 0.7f).  Regardless of success or failure,
//// replace the fraction and endpos with the original ones, so we don't try to
//// move the player down to the new floor and get stuck on a leaning wall that
//// the original trace hit first.
////-----------------------------------------------------------------------------
// 1st return value: True if ground to stand on was found, false otherwise.
// 2nd return value: Ground surface. -1 if no ground to stand on was found.
std::tuple<bool, int16_t> CsgoMovement::TryTouchGroundInQuadrants(const Vector3& start, const Vector3& end)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    Vector3 mins, maxs;
    Vector3 minsSrc = GetPlayerMins();
    Vector3 maxsSrc = GetPlayerMaxs();

    //float fraction = pm.fraction;
    //Vector3 endpos = pm.endpos;

    // Check the -x, -y quadrant
    mins = minsSrc;
    maxs = { Math::min(0.0f, maxsSrc.x()), Math::min(0.0f, maxsSrc.y()), maxsSrc.z() };
    Trace tr1 = TryTouchGround(start, end, mins, maxs);
    if (tr1.results.DidHit() && tr1.results.plane_normal.z() >= g_csgo_game_sim_cfg.sv_standable_normal)
    {
        //pm.fraction = fraction;
        //pm.endpos = endpos;
        return { true, tr1.results.surface };
    }

    // Check the +x, +y quadrant
    mins = { Math::max(0.0f, minsSrc.x()), Math::max(0.0f, minsSrc.y()), minsSrc.z() };
    maxs = maxsSrc;
    Trace tr2 = TryTouchGround(start, end, mins, maxs);
    if (tr2.results.DidHit() && tr2.results.plane_normal.z() >= g_csgo_game_sim_cfg.sv_standable_normal)
    {
        //pm.fraction = fraction;
        //pm.endpos = endpos;
        return { true, tr2.results.surface };
    }

    // Check the -x, +y quadrant
    mins = { minsSrc.x(), Math::max(0.0f, minsSrc.y()), minsSrc.z() };
    maxs = { Math::min(0.0f, maxsSrc.x()), maxsSrc.y(), maxsSrc.z() };
    Trace tr3 = TryTouchGround(start, end, mins, maxs);
    if (tr3.results.DidHit() && tr3.results.plane_normal.z() >= g_csgo_game_sim_cfg.sv_standable_normal)
    {
        //pm.fraction = fraction;
        //pm.endpos = endpos;
        return { true, tr3.results.surface };
    }

    // Check the +x, -y quadrant
    mins = { Math::max(0.0f, minsSrc.x()), minsSrc.y(), minsSrc.z() };
    maxs = { maxsSrc.x(), Math::min(0.0f, maxsSrc.y()), maxsSrc.z() };
    Trace tr4 = TryTouchGround(start, end, mins, maxs);
    if (tr4.results.DidHit() && tr4.results.plane_normal.z() >= g_csgo_game_sim_cfg.sv_standable_normal)
    {
        //pm.fraction = fraction;
        //pm.endpos = endpos;
        return { true, tr4.results.surface };
    }

    //pm.fraction = fraction;
    //pm.endpos = endpos;
    return { false, -1 };
}

void CsgoMovement::CategorizePosition(void)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    // Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
    m_surfaceFriction = 1.0f;

    // if the player hull point one unit down is solid, the player
    // is on ground

    // see if standing on something solid

    // Doing this before we move may introduce a potential latency in water detection, but
    // doing it after can get us stuck on the bottom in water if the amount we move up
    // is less than the 1 pixel 'threshold' we're about to snap to. Also, we'll call
    // this several times per frame, so we really need to avoid sticking to the bottom of
    // water on each call, and the converse case will correct itself if called twice.
    //// We are not doing water at the moment
    ////CheckWater();

    float flOffset = 2.0f;

    Vector3 point = m_vecAbsOrigin;
    point.z() -= flOffset;
    Vector3 bumpOrigin = m_vecAbsOrigin;

    // Shooting up really fast.  Definitely not on ground.
    // On ladder moving up, so not on ground either
    // NOTE: 145 is a jump.
#define NON_JUMP_VELOCITY CSGO_MIN_NO_GROUND_CHECKS_VEL_Z //140.0f

    float zvel = m_vecVelocity.z();
    bool bMovingUp = zvel > 0.0f;
    bool bMovingUpRapidly = zvel > NON_JUMP_VELOCITY;
    float flGroundEntityVelZ = 0.0f;
    ////if (bMovingUpRapidly)
    ////{
    ////    // Tracker 73219, 75878:  ywb 8/2/07
    ////    // After save/restore (and maybe at other times), we can get a case where we were saved on a lift and 
    ////    //  after restore we'll have a high local velocity due to the lift making our abs velocity appear high.  
    ////    // We need to account for standing on a moving ground object in that case in order to determine if we really 
    ////    //  are moving away from the object we are standing on at too rapid a speed.  Note that CheckJump already sets
    ////    //  ground entity to NULL, so this wouldn't have any effect unless we are moving up rapidly not from the jump button.
    ////    CBaseEntity* ground = player->GetGroundEntity();
    ////    if (ground)
    ////    {
    ////        flGroundEntityVelZ = ground->GetAbsVelocity().z;
    ////        bMovingUpRapidly = (zvel - flGroundEntityVelZ) > NON_JUMP_VELOCITY;
    ////    }
    ////}

    // Was on ground, but now suddenly am not
    if (bMovingUpRapidly || (bMovingUp && m_MoveType == MOVETYPE_LADDER))
    {
        SetGroundEntity(false);
    }
    else
    {
        // Try and move down.
        Trace initial_tr = TryTouchGround(bumpOrigin, point, GetPlayerMins(), GetPlayerMaxs());

        if (initial_tr.results.DidHit() && initial_tr.results.plane_normal.z() >= g_csgo_game_sim_cfg.sv_standable_normal)
        {
            SetGroundEntity(true, initial_tr.results.surface);
        }
        else // Was on ground, but now suddenly am not. If we hit a steep plane, we are not on ground
        {
            // Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on
            auto [is_ground_standable, standable_ground_surface] = TryTouchGroundInQuadrants(bumpOrigin, point);

            if (is_ground_standable)
            {
                SetGroundEntity(true, standable_ground_surface);
            }
            else
            {
                SetGroundEntity(false);
                // probably want to add a check for a +z velocity too!
                if ((m_vecVelocity.z() > 0.0f) && m_MoveType != MOVETYPE_NOCLIP)
                {
                    // Allegedly this affects optimal airstrafe mouse movement during subportions of a jump!
                    m_surfaceFriction = 0.25f;
                }
            }
        }
    }
}

// Purpose: Determine if the player has hit the ground while falling, apply
//          damage, and play the appropriate impact sound.
void CsgoMovement::CheckFalling(void)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    // this function really deals with landing, not falling, so early out otherwise
    if (!m_hGroundEntity || m_flFallVelocity <= 0.0f)
        return;

    if (/*!IsDead() &&*/ m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHOLD)
    {
        float fvol = 0.5;

        if (false /*player->GetWaterLevel() > 0*/)
        {
            // They landed in water.
        }
        else
        {
            //
            // They hit the ground.
            //
            //if (m_hGroundEntity->GetAbsVelocity().z() < 0.0f)
            //{
            //    // Player landed on a descending object. Subtract the velocity of the ground entity.
            //    m_flFallVelocity += m_hGroundEntity->GetAbsVelocity().z;
            //    m_flFallVelocity = Math::max(0.1f, m_flFallVelocity);
            //}

            if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
            {
                //
                // If they hit the ground going this fast they may take damage (and die).
                //
                //MoveHelper()->PlayerFallingDamage();
                fvol = 1.0f;
            }
            else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED / 2)
            {
                fvol = 0.85f;
            }
            else if (m_flFallVelocity < PLAYER_MIN_BOUNCE_SPEED)
            {
                fvol = 0.0f;
            }
        }

        //PlayerRoughLandingEffects(fvol);
    }

    // let any subclasses know that the player has landed and how hard
    OnLand(m_flFallVelocity);

    //
    // Clear the fall velocity so the impact doesn't happen again.
    //
    m_flFallVelocity = 0.0f;
}

bool CsgoMovement::CanUnduck()
{
    Vector3 newOrigin = m_vecAbsOrigin;

    if (!m_hGroundEntity)
    {
        // If in air an letting go of crouch, make sure we can offset origin to
        // make up for uncrouching
        Vector3 hullSizeNormal = GetPlayerMaxs(false) - GetPlayerMins(false);
        Vector3 hullSizeCrouch = GetPlayerMaxs(true)  - GetPlayerMins(true);
        Vector3 viewDelta = hullSizeNormal - hullSizeCrouch;
        viewDelta = -viewDelta;
        // CSGO unducks in the air by increasing the hull size 9 units at the
        // top and bottom.
        // -> Lower origin by 9 units
        newOrigin += 0.5f * viewDelta;
    }

    bool saveducked = m_bDucked;
    m_bDucked = false;
    Trace trace = TracePlayerBBox(m_vecAbsOrigin, newOrigin);
    m_bDucked = saveducked;
    if (trace.results.startsolid || trace.results.fraction != 1.0f)
        return false;
    return true;
}

// Purpose: Stop ducking
void CsgoMovement::FinishUnDuck(void)
{
    Vector3 newOrigin = m_vecAbsOrigin;

    if (!m_hGroundEntity)
    {
        // If in air an letting go of crouch, make sure we can offset origin to
        // make up for uncrouching
        Vector3 hullSizeNormal = GetPlayerMaxs(false) - GetPlayerMins(false);
        Vector3 hullSizeCrouch = GetPlayerMaxs(true)  - GetPlayerMins(true);
        Vector3 viewDelta = hullSizeNormal - hullSizeCrouch;
        viewDelta = -viewDelta;
        // CSGO unducks in the air by increasing the hull size 9 units at the
        // top and bottom.
        // -> Lower origin by 9 units
        newOrigin += 0.5f * viewDelta;
    }

    m_fFlags &= ~FL_DUCKING;
    m_bDucked = false;
    m_bDucking = false;
    m_vecViewOffset = GetPlayerViewOffset(false);
    m_flDucktime = 0;

    m_vecAbsOrigin = newOrigin;

    // player->ResetLatched(); // Some kind of animation reset

    // Recategorize position since ducking can change origin
    CategorizePosition();
}

// Purpose: Finish ducking
void CsgoMovement::FinishDuck(void)
{
    if (m_fFlags & FL_DUCKING)
        return;

    m_fFlags |= FL_DUCKING;
    m_bDucked = true;
    m_bDucking = false;
    m_vecViewOffset = GetPlayerViewOffset(true);

    if (!m_hGroundEntity)
    {
        Vector3 hullSizeNormal = GetPlayerMaxs(false) - GetPlayerMins(false);
        Vector3 hullSizeCrouch = GetPlayerMaxs(true)  - GetPlayerMins(true);
        Vector3 viewDelta = hullSizeNormal - hullSizeCrouch;
        // CSGO ducks in the air by decreasing the hull size 9 units at the top
        // and bottom.
        // -> Elevate origin by 9 units
        viewDelta *= 0.5f;

        Vector3 out = m_vecAbsOrigin + viewDelta;
        m_vecAbsOrigin = out;

        // player->ResetLatched(); // Some kind of animation reset
    }

    // See if we are stuck?
    //FixPlayerCrouchStuck(true);

    // Recategorize position since ducking can change origin
    CategorizePosition();
}

void CsgoMovement::SetDuckedEyeOffset(float duckFraction)
{
    Vector3 vDuckHullMin  = GetPlayerMins(true);
    Vector3 vStandHullMin = GetPlayerMins(false);

    float fMore = vDuckHullMin.z() - vStandHullMin.z();

    Vector3 vecDuckViewOffset  = GetPlayerViewOffset(true);
    Vector3 vecStandViewOffset = GetPlayerViewOffset(false);
    Vector3 temp = m_vecViewOffset;
    temp.z() = (     duckFraction  * (vecDuckViewOffset.z() - fMore)) +
               ((1 - duckFraction) * vecStandViewOffset.z());
    m_vecViewOffset = temp;
}

// Purpose: Crop the speed of the player when ducking and on the ground.
//   Input: bInDuck - is the player already ducking
//          bInAir - is the player in air
//    NOTE: Only crop player speed once.
void CsgoMovement::HandleDuckingSpeedCrop(void)
{
    // NOTE: The original source-sdk-2013 only crops player speed here once the
    //       player is fully ducked (m_fFlags & FL_DUCKING).
    //       This leads to a running player, that enters a crouch, moving quite
    //       fast across the floor during the ducking process.
    //       CSGO on the other hand slows down the running player, that enters a
    //       crouch, much quicker once he enters a crouch.
    //       --> We additionally crop player speed here while player is in the
    //           ducking process (m_bDucking).
    if (!(m_iSpeedCropped & SPEED_CROPPED_DUCK) && ((m_fFlags & FL_DUCKING) || m_bDucking) && m_hGroundEntity)
    {
        // NOTE: CSGO's speed factor when ducked is precisely 0.34 .
        //       CSGO running speed with knife (outside DZ): 250
        //       CSGO ducked speed with knife (outside DZ): 85
        //       -> Ducked speed factor = 85 / 250 = 0.34
        float frac = 0.34f;
        m_flForwardMove *= frac;
        m_flSideMove    *= frac;
        //m_flUpMove      *= frac;
        m_iSpeedCropped |= SPEED_CROPPED_DUCK;
    }
}

// Purpose: See if duck button is pressed and do the appropriate things
void CsgoMovement::Duck(void)
{
    int buttonsChanged  = m_nOldButtons ^ m_nButtons;     // These buttons have changed this frame
    int buttonsPressed  = buttonsChanged & m_nButtons;    // The changed ones still down are "pressed"
    int buttonsReleased = buttonsChanged & m_nOldButtons; // The changed ones which were previously down are "released"

    // Check to see if we are in the air.
    bool bInAir  = !m_hGroundEntity;
    bool bInDuck = m_fFlags & FL_DUCKING;

    if (m_nButtons & IN_DUCK)
        m_nOldButtons |= IN_DUCK;
    else
        m_nOldButtons &= ~IN_DUCK;

    // Slow down ducked players.
    HandleDuckingSpeedCrop();

    // If the player is holding down the duck button, the player is in duck transition or ducking
    if ((m_nButtons & IN_DUCK) || m_bDucking || bInDuck)
    {
        // DUCK
        if (m_nButtons & IN_DUCK)
        {
            // Have the duck button pressed, but the player currently isn't in the duck position.
            if ((buttonsPressed & IN_DUCK) && !bInDuck)
            {
                m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
                m_bDucking = true;
            }

            // The player is in duck transition
            if (m_bDucking)
            {
                float flDuckMilliseconds =
                    Math::max(0.0f, GAMEMOVEMENT_DUCK_TIME - m_flDucktime);
                float flDuckSeconds = flDuckMilliseconds * 0.001f;

                // Finish in duck transition when transition time is over, in "duck", in air.
                if (flDuckSeconds > TIME_TO_DUCK || bInDuck || bInAir)
                {
                    FinishDuck();
                }
                else
                {
                    // Calc parametric time
                    float flDuckFraction = SimpleSpline(flDuckSeconds / TIME_TO_DUCK);
                    SetDuckedEyeOffset(flDuckFraction);
                }
            }
        }
        // UNDUCK (or attempt to...)
        else
        {
            // Try to unduck unless automovement is not allowed
            // NOTE: When not onground, you can always unduck
            if (m_bAllowAutoMovement || bInAir || m_bDucking)
            {
                // We released the duck button, we aren't in "duck" and we are not in the air - start unduck transition.
                if (buttonsReleased & IN_DUCK)
                {
                    if (bInDuck)
                    {
                        m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
                    }
                    else if (m_bDucking && !m_bDucked)
                    {
                        // Invert time if release before fully ducked!!!
                        float unduckMilliseconds = 1000.0f * TIME_TO_UNDUCK;
                        float duckMilliseconds   = 1000.0f * TIME_TO_DUCK;
                        float elapsedMilliseconds = GAMEMOVEMENT_DUCK_TIME - m_flDucktime;

                        float fracDucked = elapsedMilliseconds / duckMilliseconds;
                        float remainingUnduckMilliseconds = fracDucked * unduckMilliseconds;

                        m_flDucktime = GAMEMOVEMENT_DUCK_TIME - unduckMilliseconds
                            + remainingUnduckMilliseconds;
                    }
                }


                // Check to see if we are capable of unducking.
                if (CanUnduck())
                {
                    // or unducking
                    if (m_bDucking || m_bDucked)
                    {
                        float flDuckMilliseconds =
                            Math::max(0.0f, GAMEMOVEMENT_DUCK_TIME - m_flDucktime);
                        float flDuckSeconds = flDuckMilliseconds * 0.001f;

                        // Finish ducking immediately if duck time is over or not on ground
                        if (flDuckSeconds > TIME_TO_UNDUCK || bInAir)
                        {
                            FinishUnDuck();
                        }
                        else
                        {
                            // Calc parametric time
                            float flDuckFraction = SimpleSpline(1.0f - (flDuckSeconds / TIME_TO_UNDUCK));
                            SetDuckedEyeOffset(flDuckFraction);
                            m_bDucking = true;
                        }
                    }
                }
                else
                {
                    // Still under something where we can't unduck, so make sure we reset this timer so
                    //  that we'll unduck once we exit the tunnel, etc.
                    if (m_flDucktime != GAMEMOVEMENT_DUCK_TIME)
                    {
                        SetDuckedEyeOffset(1.0f);
                        m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
                        m_bDucked = true;
                        m_bDucking = false;
                        m_fFlags |= FL_DUCKING;
                    }
                }
            }
        }
    }
    // HACK: (jimd 5/25/2006) we have a reoccuring bug (#50063 in Tracker) where the player's
    // view height gets left at the ducked height while the player is standing, but we haven't
    // been  able to repro it to find the cause.  It may be fixed now due to a change I'm
    // also making in UpdateDuckJumpEyeOffset but just in case, this code will sense the
    // problem and restore the eye to the proper position.  It doesn't smooth the transition,
    // but it is preferable to leaving the player's view too low.
    //
    // If the player is still alive and not an observer, check to make sure that
    // his view height is at the standing height.
    else if (true /* !IsDead() && !player->IsObserver() && !player->IsInAVehicle() */)
    {
        if (Math::abs(m_vecViewOffset.z() - GetPlayerViewOffset(false).z()) > 0.1)
        {
            // we should rarely ever get here, so assert so a coder knows when it happens
            //assert(0); // Commented out for DZSimulator
            Debug{} << "[GameMovement] source-sdk-2013 HACK: Restoring player view height";

            // set the eye height to the non-ducked height
            SetDuckedEyeOffset(0.0f);
        }
    }
}

void CsgoMovement::ApplyForwardsExoBoost()
{
    Vector3 forward;
    Vector3 right;
    AnglesToVectors(m_vecViewAngles, &forward, &right);
    forward.z() = 0.0f;
    right.z() = 0.0f;
    NormalizeInPlace(forward);
    NormalizeInPlace(right);

    // Get direction of pressed directional movement keys (WASD)
    // Note: CSGO sometimes gives extra speed when duck-jumping with exo
    //       _diagonally_ (e.g. pressing W+A).
    //       It's about 3% extra speed, we don't recreate that quirk here.
    Vector3 wishdir = { 0.0f, 0.0f, 0.0f };
    if (m_flForwardMove > 0.0f) wishdir += forward;
    if (m_flForwardMove < 0.0f) wishdir -= forward;
    if (m_flSideMove > 0.0f) wishdir += right;
    if (m_flSideMove < 0.0f) wishdir -= right;
    NormalizeInPlace(wishdir);

    float cur_hori_speed = m_vecVelocity.xy().length();

    float hori_boost     = g_csgo_game_sim_cfg.GetExoHoriBoost(m_loadout);
    float max_hori_speed = g_csgo_game_sim_cfg.GetExoHoriBoostMaxSpeed(m_loadout);

    if (cur_hori_speed + hori_boost > max_hori_speed)
        hori_boost = max_hori_speed - cur_hori_speed;

    if (hori_boost < 0.0f)
        return;

    // Note: In CSGO, the further the duck progress, the less forwards boost
    //       is received. This is not considered here, the difference between
    //       DZSim and CSGO in that regard seems relatively small.

    m_vecVelocity += hori_boost * wishdir;
}

void CsgoMovement::PlayerMove(float time_delta)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    CheckParameters();

    // clear output applied velocity
    //m_outWishVel = { 0.0f, 0.0f, 0.0f };
    m_outJumpVel = { 0.0f, 0.0f, 0.0f };

    ReduceTimers(time_delta);

    // Viewing direction vectors are used by ladder code for example
    //AnglesToVectors(m_vecViewAngles, &m_vecForward, &m_vecRight, &m_vecUp);  // Determine movement angles

    //// Always try and unstick us unless we are using a couple of the movement modes
    //if (m_MoveType != MOVETYPE_NOCLIP &&
    //    m_MoveType != MOVETYPE_NONE)
    //{
    //    if (CheckInterval(STUCK))
    //        if (CheckStuck())
    //            return; // Can't move, we're stuck
    //}

    // Now that we are "unstuck", see where we are (player->GetWaterLevel() and type, player->GetGroundEntity()).
    if (m_MoveType != MOVETYPE_WALK /*|| mv->m_bGameCodeMovedPlayer*/)
    {
        CategorizePosition();
    }
    else
    {
        if (m_vecVelocity.z() > CSGO_MIN_LEAVE_GROUND_VEL_Z)
        {
            SetGroundEntity(false);
        }
    }

    // Store off the starting water level
    //m_nOldWaterLevel = player->GetWaterLevel();

    // If we are not on ground, store off how fast we are moving down
    if (m_hGroundEntity == false)
    {
        m_flFallVelocity = -m_vecVelocity.z();
    }

    //m_nOnLadder = 0;

    //player->UpdateStepSound(player->m_pSurfaceData, m_vecAbsOrigin, m_vecVelocity);

    // If this function slows down the player due to ducking,
    // the SPEED_CROPPED_DUCK flag gets set in m_iSpeedCropped.
    Duck();

    // Slow down player if walk button is pressed AND the ducking speed crop
    // WASN'T applied by the Duck() function.
    // CAUTION: This check must happen after Duck() was called!
    bool walkBtnPressed = m_nButtons & IN_SPEED;
    if (walkBtnPressed && !(m_iSpeedCropped & SPEED_CROPPED_DUCK))
    {
        // NOTE: CSGO's speed factor when walking is precisely 0.52 .
        //       CSGO running speed with knife (outside DZ): 250
        //       CSGO walking speed with knife (outside DZ): 130
        //       -> Walking speed factor = 130 / 250 = 0.52
        float frac = 0.52f;
        m_flForwardMove *= frac;
        m_flSideMove    *= frac;
        //m_flUpMove      *= frac;
    }

    // If was not on a ladder now, but was on one before, 
    //  get off of the ladder

    ////// TODO: this causes lots of weirdness.
    //////bool bCheckLadder = CheckInterval( LADDER );
    //////if ( bCheckLadder || m_MoveType == MOVETYPE_LADDER )
    ////{
    ////    if (!LadderMove() &&
    ////        (m_MoveType == MOVETYPE_LADDER))
    ////    {
    ////        // Clear ladder stuff unless player is dead or riding a train
    ////        // It will be reset immediately again next frame if necessary
    ////        m_MoveType = MOVETYPE_WALK;
    ////        //player->SetMoveCollide(MOVECOLLIDE_DEFAULT);
    ////    }
    ////}

    const float NOCLIPSPEED = 5.0f;
    const float NOCLIPACCELERATE = 5.0f;

    // Handle movement modes.
    switch (m_MoveType)
    {
    case MOVETYPE_NONE:
        break;

    case MOVETYPE_NOCLIP:
        //FullNoClipMove(NOCLIPSPEED, NOCLIPACCELERATE);
        break;

    case MOVETYPE_FLY:
    case MOVETYPE_FLYGRAVITY:
        //FullTossMove();
        break;

    case MOVETYPE_LADDER:
        // Note: When we get to implementing ladder climbing, first look at
        //       https://developer.valvesoftware.com/wiki/List_of_CS:GO_Surface_Types
        //       Apparently, there are climbable surfaces ("ladder" and "woodladder")
        //FullLadderMove();
        break;

    case MOVETYPE_WALK:
        FullWalkMove(time_delta);
        break;

    case MOVETYPE_OBSERVER:
        //FullObserverMove(); // clips against world&players
        break;

    default:
        Error{} << "[GameMovement] ERROR: Invalid MoveType:" << m_MoveType;
        assert(0);
        break;
    }
}

// Purpose: Traces player movement + position
Trace CsgoMovement::TracePlayerBBox(const Vector3& start, const Vector3& end)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    // This function's original usage: Do trace with
    //   fMask == MASK_PLAYERSOLID and
    //   collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT

    Trace tr{ start, end, GetPlayerMins(), GetPlayerMaxs() };
    g_coll_world->DoTrace(&tr);
    return tr;
}

Trace CsgoMovement::TryTouchGround(const Vector3& start, const Vector3& end, const Vector3& mins, const Vector3& maxs)
{
    // GENERAL REMINDER: When copying source-sdk-2013 code like `vec1 == vec2`,
    //                   replace it with `SourceSdkVectorEqual(vec1, vec2)`!

    // This function's original usage: Do trace with
    //   fMask == MASK_PLAYERSOLID and
    //   collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT

    Trace tr{ start, end, mins, maxs };
    g_coll_world->DoTrace(&tr);
    return tr;
}

// --------- end of source-sdk-2013 code ---------
