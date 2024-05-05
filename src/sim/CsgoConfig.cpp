#include "sim/CsgoConfig.h"

#include <cassert>

using namespace sim;
using namespace Magnum::Math;
using Loadout = sim::Entities::Player::Loadout;

CsgoConfig::CsgoConfig(Corrade::NoInitT)
{
}

CsgoConfig::CsgoConfig(InitWithDzDefaults_Tag)
    : enable_consistent_bumpmine_activations( false )
    , game_mode( GameMode::DANGER_ZONE )
    , sv_accelerate( 5.5f )
    , sv_accelerate_use_weapon_speed( true )
    , cl_forwardspeed( 450.0f )
    , cl_backspeed( 450.0f )
    , cl_sidespeed( 450.0f )
    , sv_stopspeed( 80.0f )
    , sv_friction( 5.2f )
    , sv_gravity( 800.0f )
    , sv_maxspeed( 320.0f )
    , sv_maxvelocity( 3500.0f )
    , sv_stepsize( 18.0f )
    , sv_airaccelerate( 12.0f )
    , sv_airaccelerate_parachute( 2.6f )
    , sv_air_pushaway_dist( 220.0f )
    , sv_air_max_wishspeed( 30.0f )
    , sv_air_max_horizontal_parachute_ratio( 0.87f )
    , sv_air_max_horizontal_parachute_speed( 240.0f )
    , sv_player_parachute_velocity( -200.0f )
    , sv_jump_impulse( 301.993377f )
    , sv_jump_impulse_exojump_multiplier( 1.05f )
    , sv_autobunnyhopping( false )
    , sv_enablebunnyhopping( false )
    , sv_staminamax( 80.0f )
    , sv_staminarecoveryrate( 60.0f )
    , sv_staminajumpcost( 0.08f )
    , sv_staminalandcost( 0.05f )
    , sv_exostaminajumpcost( 0.04f )
    , sv_exostaminalandcost( 0.015f )
    , sv_exojump_jumpbonus_forward( 0.4f )
    , sv_exojump_jumpbonus_up( 0.58f )
    , sv_water_swim_mode( 1.0f )
    , sv_water_movespeed_multiplier( 0.5f )
    , sv_weapon_encumbrance_scale( 0.3f )
    , sv_weapon_encumbrance_per_item( 0.85f )
    , sv_ladder_scale_speed( 0.78f )
    , healthshot_healthboost_damage_multiplier( 0.9f )
    , healthshot_healthboost_speed_multiplier( 1.2f )
    , healthshot_healthboost_time( 6.5f )
    , sv_health_approach_enabled( true )
    , sv_health_approach_speed( 10.0f )
    , sv_dz_player_max_health( 120.0f )
    , sv_dz_player_spawn_health( 120.0f )
    , molotov_throw_detonate_time( 20.0f )
    , inferno_max_range( 400.0f )
    , ff_damage_reduction_grenade_self( 1.0f )
    , sv_hegrenade_damage_multiplier( 1.1f )
    , sv_hegrenade_radius_multiplier( 1.7f )
    , sv_falldamage_scale( 0.65f )
    , sv_falldamage_exojump_multiplier( 0.4f )
    , sv_falldamage_to_below_player_multiplier( 1.5f )
    , sv_falldamage_to_below_player_ratio( 0.6f )
    , sv_standable_normal( 0.7f )
    , sv_walkable_normal( 0.7f )
    , sv_bumpmine_arm_delay( 0.3f )
    , sv_bumpmine_detonate_delay( 0.25f )
    , sv_dz_zone_damage( 1.0f )
    , sv_dz_zone_hex_radius( 2200.0f )
    , sv_timebetweenducks( 0.4f )
    , sv_knife_attack_extend_from_player_aabb( 10.0f )
{
}

CsgoConfig::CsgoConfig(InitWithCompDefaults_Tag)
    : enable_consistent_bumpmine_activations( false )
    , game_mode( GameMode::COMPETITIVE )
    , sv_accelerate( 5.5f )
    , sv_accelerate_use_weapon_speed( true )
    , cl_forwardspeed( 450.0f )
    , cl_backspeed( 450.0f )
    , cl_sidespeed( 450.0f )
    , sv_stopspeed( 80.0f )
    , sv_friction( 5.2f )
    , sv_gravity( 800.0f )
    , sv_maxspeed( 320.0f )
    , sv_maxvelocity( 3500.0f )
    , sv_stepsize( 18.0f )
    , sv_airaccelerate( 12.0f )
    , sv_airaccelerate_parachute( 2.6f )
    , sv_air_pushaway_dist( 0.0 ) // differs in DZ
    , sv_air_max_wishspeed( 30.0f )
    , sv_air_max_horizontal_parachute_ratio( 0.87f )
    , sv_air_max_horizontal_parachute_speed( 240.0f )
    , sv_player_parachute_velocity( -200.0f )
    , sv_jump_impulse( 301.993377f )
    , sv_jump_impulse_exojump_multiplier( 1.05f )
    , sv_autobunnyhopping( false )
    , sv_enablebunnyhopping( false )
    , sv_staminamax( 80.0f )
    , sv_staminarecoveryrate( 60.0f )
    , sv_staminajumpcost( 0.08f )
    , sv_staminalandcost( 0.05f )
    , sv_exostaminajumpcost( 0.04f )
    , sv_exostaminalandcost( 0.015f )
    , sv_exojump_jumpbonus_forward( 0.4f )
    , sv_exojump_jumpbonus_up( 0.58f )
    , sv_water_swim_mode( 0.0f ) // differs in DZ
    , sv_water_movespeed_multiplier( 0.8f ) // differs in DZ
    , sv_weapon_encumbrance_scale( 0.0f ) // differs in DZ
    , sv_weapon_encumbrance_per_item( 0.85f )
    , sv_ladder_scale_speed( 0.78f )
    , healthshot_healthboost_damage_multiplier( 1.0f ) // differs in DZ
    , healthshot_healthboost_speed_multiplier( 1.0f ) // differs in DZ
    , healthshot_healthboost_time( 0.0f ) // differs in DZ
    , sv_health_approach_enabled( false ) // differs in DZ
    , sv_health_approach_speed( 10.0f )
    , sv_dz_player_max_health( 120.0f )
    , sv_dz_player_spawn_health( 120.0f )
    , molotov_throw_detonate_time( 2.0f ) // differs in DZ
    , inferno_max_range( 150.0f ) // differs in DZ
    , ff_damage_reduction_grenade_self( 1.0f )
    , sv_hegrenade_damage_multiplier( 1.0f ) // differs in DZ
    , sv_hegrenade_radius_multiplier( 1.0f ) // differs in DZ
    , sv_falldamage_scale( 1.0f ) // differs in DZ
    , sv_falldamage_exojump_multiplier( 0.4f )
    , sv_falldamage_to_below_player_multiplier( 1.0f ) // differs in DZ
    , sv_falldamage_to_below_player_ratio( 0.0f ) // differs in DZ
    , sv_standable_normal( 0.7f )
    , sv_walkable_normal( 0.7f )
    , sv_bumpmine_arm_delay( 0.3f )
    , sv_bumpmine_detonate_delay( 0.25f )
    , sv_dz_zone_damage( 1.0f )
    , sv_dz_zone_hex_radius( 2200.0f )
    , sv_timebetweenducks( 0.4f )
    , sv_knife_attack_extend_from_player_aabb( 0.0f ) // differs in DZ
{
}


// =============================================================================


// Properties of a player with a specific loadout.
struct LoadoutCharacteristics {
    float max_player_running_speed;
    float exo_hori_boost; // Horizontal velocity increase of duck-jumping w/ exo
    float exo_hori_max_speed; // Horizontal velocity limit of duck-jumping w/ exo
};

static LoadoutCharacteristics GetCharacteristics(const Loadout& lo,
                                                 const CsgoConfig& cfg)
{
    constexpr Loadout::Weapon Fists    = Loadout::Weapon::Fists;
    constexpr Loadout::Weapon Knife    = Loadout::Weapon::Knife;
    constexpr Loadout::Weapon BumpMine = Loadout::Weapon::BumpMine;
    constexpr Loadout::Weapon Taser    = Loadout::Weapon::Taser;
    constexpr Loadout::Weapon XM1014   = Loadout::Weapon::XM1014;

    // -------------------------------------------------------------------------

    // Depending on mode and setting, non-active weapons limit the max speed.
    bool encumbrance_enabled = false; // Whether non-active weapons slow down
    if (cfg.game_mode == CsgoConfig::GameMode::COMPETITIVE) {
        encumbrance_enabled = false;
    }
    else if (cfg.game_mode == CsgoConfig::GameMode::DANGER_ZONE) {
        if (cfg.sv_accelerate_use_weapon_speed) encumbrance_enabled = true;
        else                                    encumbrance_enabled = false;
    }
    else { // Error: Invalid game mode
        assert(0);
        return {
            .max_player_running_speed = 0.0f,
            .exo_hori_boost = 0.0f,
            .exo_hori_max_speed = 0.0f
        };
    }

    // -------------------------------------------------------------------------

    // I gave up trying to understand CSGO's player speed mechanics, so here's
    // a look-up table for different combinations of weapons on a player and
    // game settings.

    if (!encumbrance_enabled)
    {
        // The following values were obtained from CSGO tests under these
        // circumstances:
        // - sv_weapon_encumbrance_scale 0
        // - sv_exojump_jumpbonus_forward 0.4
        // - Player equipped the exojump (!)
        // - Player has full duck speed and stamina
        // - Player duck-jumps perfectly using duck-jump bind:
        //   - alias "+djump" "+jump; +duck"
        //   - alias "-djump" "-jump; -duck"
        //   - bind "space" "+djump"
        // CAUTION: All of these influence the following LUT values!

        switch (lo.active_weapon) {
            // Note: CSGO's 'scripts/items/items_game.txt' lists fists with a
            //       max speed of 275, but CSGO caps at 260.
            case Fists:    return { .max_player_running_speed = 260.0f,
                                    .exo_hori_boost = 148.218f,
                                    .exo_hori_max_speed = 395.247f };
            case Knife:    return { .max_player_running_speed = 250.0f,
                                    .exo_hori_boost = 142.517f,
                                    .exo_hori_max_speed = 380.045f };
            case BumpMine: return { .max_player_running_speed = 245.0f,
                                    .exo_hori_boost = 139.667f,
                                    .exo_hori_max_speed = 372.444f };
            case Taser:    return { .max_player_running_speed = 220.0f,
                                    .exo_hori_boost = 125.415f,
                                    .exo_hori_max_speed = 334.44f };
            case XM1014:   return { .max_player_running_speed = 215.0f,
                                    .exo_hori_boost = 122.565f,
                                    .exo_hori_max_speed = 326.839f };
            default: break; // Error
        }
    }
    else { // encumbrance_enabled
        // In Danger Zone, the max running speed depends on non-active weapons
        // the player carries and whether exojump is equipped.
        // Here we're assuming the player has exojump equipped.

        // The following values were obtained from CSGO tests under these
        // circumstances:
        // - sv_accelerate_use_weapon_speed 1
        // - sv_weapon_encumbrance_per_item 0.85
        // - sv_weapon_encumbrance_scale 0.3
        // - sv_exojump_jumpbonus_forward 0.4
        // - Player equipped the exojump (!)
        // - Player has full duck speed and stamina
        // - Player duck-jumps perfectly using duck-jump bind:
        //   - alias "+djump" "+jump; +duck"
        //   - alias "-djump" "-jump; -duck"
        //   - bind "space" "+djump"
        // CAUTION: All of these influence the following LUT values!
        //          In CSGO, a player without an exojump is slightly slower in
        //          some cases.
        //          We are not handling non-exojump cases, it'd be too much work.

        // NOTE: The inner if-checks must be ordered by descending encumbrance!
        // NOTE: Non-active fists, Bump Mines or a tablet don't seem to cause
        //       encumbrance!

        switch (lo.active_weapon) {
            case Fists: {
                if (lo.non_active_weapons[XM1014])
                    return {
                        .max_player_running_speed = 239.018f,
                        .exo_hori_boost = 148.218f,
                        .exo_hori_max_speed = 387.236f
                    };
                if (lo.non_active_weapons[Taser])
                    return {
                        .max_player_running_speed = 240.344f,
                        .exo_hori_boost = 148.218f,
                        .exo_hori_max_speed = 388.562f
                    };
                if (lo.non_active_weapons[Knife])
                    return {
                        .max_player_running_speed = 248.3f,
                        .exo_hori_boost = 148.218f,
                        .exo_hori_max_speed = 395.247f
                    };
                // Else, no slowdown
                // Note: CSGO's 'scripts/items/items_game.txt' lists fists with
                //       a max speed of 275, but CSGO caps at 260.
                return {
                    .max_player_running_speed = 260.0f,
                    .exo_hori_boost = 148.218f,
                    .exo_hori_max_speed = 395.247f
                };
            }
            case Knife: {
                if (lo.non_active_weapons[XM1014])
                    return {
                        .max_player_running_speed = 229.825f,
                        .exo_hori_boost = 142.517f,
                        .exo_hori_max_speed = 372.342f
                    };
                if (lo.non_active_weapons[Taser])
                    return {
                        .max_player_running_speed = 231.1f,
                        .exo_hori_boost = 142.517f,
                        .exo_hori_max_speed = 373.617f
                    };
                // Else, no slowdown
                return {
                    .max_player_running_speed = 250.0f,
                    .exo_hori_boost = 142.517f,
                    .exo_hori_max_speed = 380.045f
                };
            }
            case BumpMine: {
                if (lo.non_active_weapons[XM1014])
                    return {
                        .max_player_running_speed = 229.825f,
                        .exo_hori_boost = 139.667f,
                        .exo_hori_max_speed = 369.492f
                    };
                if (lo.non_active_weapons[Taser])
                    return {
                        .max_player_running_speed = 231.1f,
                        .exo_hori_boost = 139.667f,
                        .exo_hori_max_speed = 370.767f
                    };
                if (lo.non_active_weapons[Knife])
                    return {
                        .max_player_running_speed = 238.75f,
                        .exo_hori_boost = 139.667f,
                        .exo_hori_max_speed = 372.444f
                    };
                // Else, no slowdown
                return {
                    .max_player_running_speed = 245.0f,
                    .exo_hori_boost = 139.667f,
                    .exo_hori_max_speed = 372.444f
                };
            }
            case Taser: {
                // No slowdown, not even with XM1014
                return {
                    .max_player_running_speed = 220.0f,
                    .exo_hori_boost = 125.415f,
                    .exo_hori_max_speed = 334.44f
                };
            }
            case XM1014: {
                // No slowdown
                return {
                    .max_player_running_speed = 215.0f,
                    .exo_hori_boost = 122.565f,
                    .exo_hori_max_speed = 326.839f
                };
            }
            default: break; // Error
        }
    }

    // Error: Missing weapon checks or invalid game mode or invalid loadout
    assert(0);
    return {
        .max_player_running_speed = 0.0f,
        .exo_hori_boost = 0.0f,
        .exo_hori_max_speed = 0.0f
    };
}

float CsgoConfig::GetMaxPlayerRunningSpeed(const Loadout& lo) const
{
    return GetCharacteristics(lo, *this).max_player_running_speed;
}

float CsgoConfig::GetExoHoriBoost(const Loadout& lo) const
{
    return GetCharacteristics(lo, *this).exo_hori_boost;
}

float CsgoConfig::GetExoHoriBoostMaxSpeed(const Loadout& lo) const
{
    return GetCharacteristics(lo, *this).exo_hori_max_speed;
}
