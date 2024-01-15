#ifndef SIM_CSGOMOVEMENT_H_
#define SIM_CSGOMOVEMENT_H_

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "coll/SweptTrace.h"


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/game/shared/in_buttons.h)
// TODO rename / move these defines
#define IN_JUMP      (1 <<  1)
#define IN_DUCK      (1 <<  2)
#define IN_FORWARD   (1 <<  3)
#define IN_BACK      (1 <<  4)
#define IN_MOVELEFT  (1 <<  9)
#define IN_MOVERIGHT (1 << 10)
#define IN_SPEED     (1 << 17) // Player is holding the speed key
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/const.h)
// edict->movetype values
enum MoveType_t
{
    MOVETYPE_NONE = 0,   // never moves
    MOVETYPE_ISOMETRIC,  // For players -- in TF2 commander view, etc.
    MOVETYPE_WALK,       // Player only - moving on the ground
    MOVETYPE_STEP,       // gravity, special edge handling -- monsters use this
    MOVETYPE_FLY,        // No gravity, but still collides with stuff
    MOVETYPE_FLYGRAVITY, // flies through the air + is affected by gravity
    MOVETYPE_VPHYSICS,   // uses VPHYSICS for simulation
    MOVETYPE_PUSH,       // no clip to world, push and crush
    MOVETYPE_NOCLIP,     // No gravity, no collisions, still do velocity/avelocity
    MOVETYPE_LADDER,     // Used by players only when going onto a ladder
    MOVETYPE_OBSERVER,   // Observer movement, depends on player's observer mode
    MOVETYPE_CUSTOM,     // Allows the entity to describe its own physics

    // should always be defined as the last item in the list
    MOVETYPE_LAST = MOVETYPE_CUSTOM,

    MOVETYPE_MAX_BITS = 4
};
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/game/shared/gamemovement.h)
// (taken and modified from source-sdk-2013/<...>/src/game/shared/igamemovement.h)
// (taken and modified from source-sdk-2013/<...>/src/game/client/c_baseentity.h)
// (taken and modified from source-sdk-2013/<...>/src/game/server/playerlocaldata.h)
// (taken and modified from source-sdk-2013/<...>/src/game/server/player.h)

enum
{
    SPEED_CROPPED_RESET  = 0,
    SPEED_CROPPED_DUCK   = 1,
    SPEED_CROPPED_WEAPON = 2,
};

class CsgoMovement {
public:
    MoveType_t m_MoveType = MOVETYPE_WALK; // why does MOVETYPE_NONE not work as default?
    bool  m_hGroundEntity = false; // Originally, was a pointer to the entity we stand on

    int   m_fFlags = 0; // Player flags (see macros starting with FL_*)
    
    // FIXME TODO Seems like all these values must be stored inside WorldState!
    bool  m_bDucked     = false; // Fully ducked
    bool  m_bDucking    = false; // In process of ducking
    bool  m_bInDuckJump = false; // In process of duck-jumping
    // During ducking process, amount of time before full duck
    float m_flDucktime     = 0.0f; // in milliseconds!
    float m_flDuckJumpTime = 0.0f; // in milliseconds!
    // Jump time, time to auto unduck (since we auto crouch jump now).
    float m_flJumpTime     = 0.0f; // in milliseconds!
    float m_flFallVelocity = 0.0f; // Velocity at time when we hit ground
    bool  m_bAllowAutoMovement = true;

    float m_flMaxSpeed    = 0.0f;
    float m_flForwardMove = 0.0f;
    float m_flSideMove    = 0.0f;
    int   m_nButtons    = 0;
    int   m_nOldButtons = 0;
    Magnum::Vector3 m_vecViewAngles   = { 0.0f, 0.0f, 0.0f }; // Command view angles (local space)
    Magnum::Vector3 m_vecAbsOrigin    = { 0.0f, 0.0f, 0.0f };
    Magnum::Vector3 m_vecVelocity     = { 0.0f, 0.0f, 0.0f };
    Magnum::Vector3 m_vecBaseVelocity = { 0.0f, 0.0f, 0.0f }; // TODO this probably needs to be added into the Player class
    // Output vars
    //float m_outStepHeight; // How much you climbed this move. (NOTE: Unknown purpose)
    //Magnum::Vector3 m_outWishVel; // This is where you tried (NOTE: Unknown purpose)
    Magnum::Vector3 m_outJumpVel; // This is your jump velocity
    
    int m_iSpeedCropped = 0;
    Magnum::Vector3 m_vecForward = { 0.0f, 0.0f, 0.0f };
    Magnum::Vector3 m_vecRight   = { 0.0f, 0.0f, 0.0f };
    Magnum::Vector3 m_vecUp      = { 0.0f, 0.0f, 0.0f };

    //int   m_nOldWaterLevel;
    //float m_flWaterEntryTime;
    //int   m_nOnLadder;
    
    float m_surfaceFriction = 1.0f;


    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    CsgoMovement();

    Magnum::Vector3 GetPlayerMins(bool ducked) const;
    Magnum::Vector3 GetPlayerMaxs(bool ducked) const;

    coll::SweptTrace TracePlayerBBox(
        const Magnum::Vector3& start, const Magnum::Vector3& end);
    coll::SweptTrace TryTouchGround(
        const Magnum::Vector3& start, const Magnum::Vector3& end,
        const Magnum::Vector3& mins, const Magnum::Vector3& maxs);

    // Does most of the player movement logic.
    // Returns with origin, angles, and velocity modified in place.
    // were contacted during the move.
    void PlayerMove(float time_delta);

    // Set ground data, etc.
    void FinishMove(void);

    // Handles both ground friction and water friction
    void Friction(float frametime);

    void AirAccelerate(float frametime, const Magnum::Vector3& wishdir, float wishspeed, float accel);

    void AirMove(float frametime);

    bool CanAccelerate();
    void Accelerate(Magnum::Vector3& wishdir, float wishspeed, float accel, float frametime);

    // Only used by players. Moves along the ground when player is a MOVETYPE_WALK.
    void WalkMove(float frametime);

    // Try to keep a walking player on the ground when running down slopes etc
    void StayOnGround(void);

    // Handle MOVETYPE_WALK.
    void FullWalkMove(float frametime);

    // allow overridden versions to respond to jumping
    void OnJump(float fImpulse)  { /* ... Respond to jumping ... */ }
    void OnLand(float fVelocity) { /* ... Respond to landing ... */ }

    Magnum::Vector3 GetPlayerMins(void) const { return GetPlayerMins(m_bDucked); } // uses local player
    Magnum::Vector3 GetPlayerMaxs(void) const { return GetPlayerMaxs(m_bDucked); } // uses local player

    typedef enum
    {
        GROUND = 0,
        STUCK,
        LADDER
    } IntervalType_t;

    //int GetCheckInterval(IntervalType_t type);

    // Useful for things that happen periodically. This lets things happen on the specified interval, but
    // spaces the events onto different frames for different players so they don't all hit their spikes
    // simultaneously.
    //bool CheckInterval(IntervalType_t type);


    // Decomposed gravity
    void StartGravity(float frametime);
    void FinishGravity(float frametime);

    // Returns true if he started a jump (ie: should he play the jump animation)?
    bool CheckJumpButton(float frametime); // Overridden by each game.

    // The basic solid body movement clip that slides along multiple planes
    int TryPlayerMove(float frametime,
        const Magnum::Vector3* pFirstDest = nullptr,
        const coll::SweptTrace* pFirstTrace = nullptr);

    //float LadderDistance(void) const { return 2.0f; } ///< Returns the distance a player can be from a ladder and still attach to it
    //float ClimbSpeed(void) const { return MAX_CLIMB_SPEED; }
    //float LadderLateralMultiplier(void) const { return 1.0f; }

    // See if the player has a bogus velocity value.
    void CheckVelocity(void);

    // Slide off of the impacting object
    // returns the blocked flags:
    // 0x01 == floor
    // 0x02 == step / wall
    int ClipVelocity(const Magnum::Vector3& in, const Magnum::Vector3& normal,
        Magnum::Vector3& out, float overbounce);

    // Determine if player is in water, on ground, etc.
    void CategorizePosition(void);

    void CheckParameters(void);

    void ReduceTimers(float time_delta);

    void CheckFalling(void);

    // Ducking
    //void Duck(void);
    //void HandleDuckingSpeedCrop();
    //void FinishUnDuck(void);
    //void FinishDuck(void);
    //bool CanUnduck();
    //void UpdateDuckJumpEyeOffset(void);
    //bool CanUnDuckJump(coll::SweptTrace& trace);
    //void StartUnDuckJump(void);
    //void FinishUnDuckJump(coll::SweptTrace& trace);
    //void SetDuckedEyeOffset(float duckFraction);
    //
    //float SplineFraction(float value, float scale);

    void CategorizeGroundSurface(int16_t surface/*const coll::SweptTrace& pm*/);

    // Traces the player bbox as it is swept from start to end
    //CBaseHandle TestPlayerPosition(const Magnum::Vector3& pos, int collisionGroup, coll::SweptTrace& pm);

    // Set whether player is standing on ground, and if so, what ground surface
    void SetGroundEntity(bool has_ground, int16_t surface = -1);

    void StepMove(float frametime, const Magnum::Vector3& vecDestination,
        const coll::SweptTrace& trace);

    // When we step on ground that's too steep, search to see if there's any
    // ground nearby that isn't too steep.
    // 1st return value: True if ground to stand on was found, false otherwise.
    // 2nd return value: Ground surface. -1 if no ground to stand on was found.
    std::tuple<bool, int16_t> TryTouchGroundInQuadrants(const Magnum::Vector3& start, const Magnum::Vector3& end);

};
// --------- end of source-sdk-2013 code ---------


#endif // SIM_CSGOMOVEMENT_H_
