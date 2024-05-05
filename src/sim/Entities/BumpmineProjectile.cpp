#include "sim/Entities/BumpmineProjectile.h"

#include <cmath>

#include "Magnum/Magnum.h"
#include "Magnum/Math/Angle.h"
#include "Magnum/Math/Functions.h"

#include "coll/CollidableWorld.h"
#include "coll/Trace.h"
#include "GlobalVars.h"
#include "sim/CsgoConstants.h"
#include "sim/WorldState.h"
#include "utils_3d.h"

using namespace Magnum;
using namespace sim;
using namespace sim::Entities;
using namespace Math::Literals;

static const Vector3 BM_MINS = { -2.0f, -2.0f, -2.0f };
static const Vector3 BM_MAXS = { +2.0f, +2.0f, +2.0f };

static size_t next_unique_bm_id = 0;
size_t BumpmineProjectile::GenerateNewUniqueID()
{
    return next_unique_bm_id++;
}

void BumpmineProjectile::DoTimeStep(WorldState& world_of_this_bm,
    double step_size_sec, size_t subsequent_tick_id)
{
    if (!is_on_surface) {
        Vector3 pos_delta = step_size_sec * velocity;
        coll::Trace tr{ position, position + pos_delta, BM_MINS, BM_MAXS };
        g_coll_world->DoTrace(&tr); // FIXME Don't trace against player clips!

        if (!tr.results.DidHit()) { // If Bump Mine hasn't hit any surface
            position += step_size_sec * velocity;

            const float bm_gravity = 1.0f;
            velocity.z() -= bm_gravity * g_csgo_game_sim_cfg.sv_gravity * step_size_sec;

            // Limit Bump Mine velocity on each axis
            for (int i = 0; i < 3; i++) {
                if (velocity[i] > g_csgo_game_sim_cfg.sv_maxvelocity) {
                    Debug{} << "[GameSim] WARNING: Got a Bump Mine velocity"
                            << "too high on axis" << i << ". ->" << velocity;
                    velocity[i] = g_csgo_game_sim_cfg.sv_maxvelocity;
                }
                else if (velocity[i] < -g_csgo_game_sim_cfg.sv_maxvelocity) {
                    Debug{} << "[GameSim] WARNING: Got a Bump Mine velocity"
                            << "too low on axis" << i << ". ->" << velocity;
                    velocity[i] = -g_csgo_game_sim_cfg.sv_maxvelocity;
                }
            }
        }
        else { // If Bump Mine hit a surface to stick to
            is_on_surface = true;
            position += tr.results.fraction * pos_delta;
            velocity = { 0.0f, 0.0f, 0.0f };

            Vector3 up = tr.results.plane_normal;
            Vector3 left = utils_3d::GetVectorPerpendicularToNormal(up).normalized();
            Vector3 forward = Math::cross(left, up);
            angles = utils_3d::VectorsToAngles(up, forward, left);

            inv_rotation_on_surface = utils_3d::CalcQuaternion(angles)
                .normalized()
                .invertedNormalized();

            // Only start checking for activations after some delay.
            next_think = subsequent_tick_id + GetTimeIntervalInTicks(
                g_csgo_game_sim_cfg.sv_bumpmine_arm_delay, step_size_sec);
        }
    }

    // Handle think function
    if (next_think <= subsequent_tick_id) {
        float think_delay_secs =
            Think(world_of_this_bm, step_size_sec, subsequent_tick_id);
        next_think = subsequent_tick_id
            + GetTimeIntervalInTicks(think_delay_secs, step_size_sec);
    }
}

float BumpmineProjectile::GetActivationCheckIntervalInSecs()
{
    if (g_csgo_game_sim_cfg.enable_consistent_bumpmine_activations)
        return 0.0f; // Check activation every tick
    else
        return CSGO_BUMP_THINK_INTERVAL_SECS;
}

float BumpmineProjectile::Think(WorldState& world, double step_size_sec,
    size_t cur_tick_id)
{
    if (!is_on_surface)
        return 0.0f; // Think again next tick

    // If we were flagged for detonation
    if (detonates_on_next_think) {
        // Boost player(s)
        // Note: If there were other players, they'd get boosted if their AABB
        //       is touching the Bump Mine AAAB.

        // If player isn't on Bump Mine boost cooldown
        if (world.csgo_mv.m_nextBumpBoost <= cur_tick_id) {
            world.csgo_mv.m_nextBumpBoost = cur_tick_id +
                GetTimeIntervalInTicks(CSGO_BUMP_BOOST_COOLDOWN_SECS, step_size_sec);

            Vector3 player_boost_point =
                world.csgo_mv.GetPlayerCenter() + Vector3(0.0f, 0.0f, 8.0f);
            Vector3 boost_dir = player_boost_point - this->position;

            if (boost_dir.z() < 0.0f) { // Different method for z < 0
                player_boost_point =
                    world.csgo_mv.m_vecAbsOrigin
                    + Vector3(0.0f, 0.0f, world.csgo_mv.GetPlayerMaxs().z());
                boost_dir = player_boost_point - this->position;
            }

            utils_3d::NormalizeInPlace(boost_dir);
            world.csgo_mv.m_vecVelocity += boost_dir * CSGO_BUMP_BOOST_SPEED;
        }

        has_detonated = true; // Mark this Bump Mine for deletion
        return 9999.0f; // We shouldn't ever think again
    }

    Vector3 player_mins = world.csgo_mv.GetPlayerMins();
    Vector3 player_maxs = world.csgo_mv.GetPlayerMaxs();

    bool aabb_hit = coll::AabbIntersectsAabb(
        this->position + CSGO_BUMP_AABB_MINS,
        this->position + CSGO_BUMP_AABB_MAXS,
        world.csgo_mv.m_vecAbsOrigin + player_mins,
        world.csgo_mv.m_vecAbsOrigin + player_maxs);

    if (!aabb_hit)
        return GetActivationCheckIntervalInSecs();

    // Transform player position into Bump Mine's coordinate system
    Vector3 player_center_transf = world.csgo_mv.GetPlayerCenter() - this->position;

    player_center_transf =
        inv_rotation_on_surface.transformVectorNormalized(player_center_transf);

    const float VERT_SCALE_FACTOR =
        CSGO_BUMP_ELLIPSOID_WIDTH / CSGO_BUMP_ELLIPSOID_HEIGHT;

    // Turn ellipsoids into spheres by shrinking the coordinate system
    player_center_transf.z() *= VERT_SCALE_FACTOR;

    // Mirror player behind Bump Mine in front of Bump Mine
    player_center_transf.z() = Math::abs(player_center_transf.z());

    const float SCALED_DOWN_ELLIPSOID_CENTER_Z_OFFSET =
        VERT_SCALE_FACTOR * CSGO_BUMP_ELLIPSOID_CENTER_Z_OFFSET;

    const Vector3 SCALED_DOWN_ELLIPSOID_CENTER =
        Vector3(0.0f, 0.0f, SCALED_DOWN_ELLIPSOID_CENTER_Z_OFFSET);

    float dist_sqr = (SCALED_DOWN_ELLIPSOID_CENTER - player_center_transf).dot();

    const float SPHERE_RADIUS = 0.5f * CSGO_BUMP_ELLIPSOID_WIDTH;
    if (dist_sqr > SPHERE_RADIUS * SPHERE_RADIUS)
        return GetActivationCheckIntervalInSecs(); // Player outside shape

    // Player has triggerd the Bump Mine. Detonate with some delay.
    this->detonates_on_next_think = true;
    return g_csgo_game_sim_cfg.sv_bumpmine_detonate_delay;
}
