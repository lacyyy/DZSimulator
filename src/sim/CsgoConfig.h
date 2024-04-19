#ifndef SIM_CSGOCONFIG_H
#define SIM_CSGOCONFIG_H

#include <Corrade/Tags.h>

// Tags to select the correct CsgoConfig constructor
struct InitWithDzDefaults_Tag{};
struct InitWithCompDefaults_Tag{};
constexpr InitWithDzDefaults_Tag   InitWithDzDefaults;
constexpr InitWithCompDefaults_Tag InitWithCompDefaults;


namespace sim {

// CSGO game settings (ConVars, weapon stats, ...)
class CsgoConfig {
public:
    // Initialize nothing
    explicit CsgoConfig(Corrade::NoInitT);
    // Initialize with default 'Danger Zone' game settings
    explicit CsgoConfig(InitWithDzDefaults_Tag);
    // Initialize with default 'Competitive 5v5' game settings
    explicit CsgoConfig(InitWithCompDefaults_Tag);


    // ------------- CSGO ConVars -------------

    // Game mode to be played.
    // (Combines CSGO's two ConVars 'game_mode' and 'game_type' into one)
    enum GameMode {
        DANGER_ZONE, // Danger Zone game mode
        COMPETITIVE  // Regular 5v5 competitive game mode
    } game_mode;

    float sv_accelerate; // Linear acceleration amount for when player walks
    bool  sv_accelerate_use_weapon_speed; // Whether player acceleration is affected by carried weapons
    float cl_forwardspeed;
    float cl_backspeed;
    float cl_sidespeed;
    float sv_stopspeed; // Minimum stopping speed when on ground.
    float sv_friction; // World friction.
    float sv_gravity;  // World gravity. units / (second^2)
    float sv_maxspeed; // Seems useless, actual max speed seems to be 260
    float sv_maxvelocity; // Maximum speed any ballistically moving object is allowed to attain per axis.
    float sv_stepsize; // Hidden CSGO CVar

    float sv_airaccelerate;           // no description
    float sv_airaccelerate_parachute; // no description
    float sv_air_pushaway_dist;       // no description
    float sv_air_max_wishspeed;
    float sv_air_max_horizontal_parachute_ratio; // ratio of max hori para speed when player glides sideways?
    float sv_air_max_horizontal_parachute_speed; // max horizontal speed when parachute is open
    float sv_player_parachute_velocity; // downwards parachute falling speed

    float sv_jump_impulse; // Initial upward velocity for player jumps; sqrt(2*gravity*height).
    float sv_jump_impulse_exojump_multiplier; // ExoJump impulse multiplier
    bool  sv_autobunnyhopping; // Players automatically jump when touching ground while holding jump button
    bool  sv_enablebunnyhopping; // Disables in-air movement speed cap

    float sv_staminamax; // Maximum stamina penalty
    float sv_staminarecoveryrate; // Rate at which stamina recovers (units/sec)

    float sv_staminajumpcost; // Stamina penalty for jumping without exo legs
    float sv_staminalandcost; // Stamina penalty for landing without exo legs
    float sv_exostaminajumpcost; // Stamina penalty for jumping with exo legs
    float sv_exostaminalandcost; // Stamina penalty for landing with exo legs

    float sv_exojump_jumpbonus_forward; // ExoJump forwards velocity bonus when duck jumping
    float sv_exojump_jumpbonus_up; // ExoJump upwards bonus when holding the jump button (percentage of sv_gravity value)

    float sv_water_swim_mode; // Prevent going underwater
    float sv_water_movespeed_multiplier;

    float sv_weapon_encumbrance_scale; // Encumbrance ratio to active weapon
    float sv_weapon_encumbrance_per_item; // Encumbrance fixed cost

    float sv_ladder_scale_speed; // (min. 0.0, max. 1.0) Scale top speed on ladders

    float healthshot_healthboost_damage_multiplier; // A multiplier for damage that healing player receives.
    float healthshot_healthboost_speed_multiplier;
    float healthshot_healthboost_time;
    bool  sv_health_approach_enabled; // Whether the HP are granted at once (0) or over time (1).
    float sv_health_approach_speed;   // The rate at which the healing is granted, in HP per second
    float sv_dz_player_max_health;
    float sv_dz_player_spawn_health;

    float molotov_throw_detonate_time;
    float inferno_max_range; // Maximum distance flames can spread from their initial ignition point
    float ff_damage_reduction_grenade_self; // How much to damage a player does to himself with his own grenade.
    float sv_hegrenade_damage_multiplier;
    float sv_hegrenade_radius_multiplier;

    float sv_falldamage_scale;
    float sv_falldamage_exojump_multiplier; // ExoJump fall damage multiplier
    float sv_falldamage_to_below_player_multiplier; // Scale damage when distributed across two players
    float sv_falldamage_to_below_player_ratio; // Landing on another player's head gives them this ratio of the damage.

    float sv_standable_normal; // Inclination of surfaces the player can stand on
    float sv_walkable_normal;  // Inclination of surfaces the player can walk on

    float sv_bumpmine_arm_delay;
    float sv_bumpmine_detonate_delay;

    float sv_dz_zone_damage;
    float sv_dz_zone_hex_radius;

    float sv_timebetweenducks; // Minimum time before recognizing consecutive duck key
    float sv_knife_attack_extend_from_player_aabb;


    // TODO Other cvars to potentially consider/investigate:
    //sv_shield_*
    //mp_shield_speed_deployed
    //mp_shield_speed_holstered
    //sv_cs_player_speed_has_hostage
    //mp_hostages_run_speed_modifier
    //sv_ladder_scale_speed
    //sv_ladder_dampen
    //sv_script_think_interval
    //sv_wateraccelerate
    //sv_waterdist
    //sv_waterfriction
    //cl_pitchdown
    //cl_pitchup
    //cl_pitchspeed
    //cl_pdump 1 for player values
    //r_eyewaterepsilon
    //sv_ledge_mantle_helper (default: 1, apparently removed in CS2?)
    //  "1=Only improves success of jump+ducks to windows or vents (jump+duck to
    //  duck), 2=Improves success of all jump+ducks to ledges, 3=if you can get your
    //  eyes above it, you'll pull yourself up"
    //sv_ledge_mantle_helper_dzonly (default: 0)
    //sv_ledge_mantle_helper_debug (hidden cvar)


    // ------------- CSGO weapon stats -------------

    // FIXME TODO The max weapon speeds differ in DZ mode, we currently don't
    //            differentiate!
    // from CSGO's 'scripts/items/items_game.txt'
    float WEAPON_FISTS_MAX_PLAYER_SPEED    = 275.0f;
    float WEAPON_KNIFE_MAX_PLAYER_SPEED    = 250.0f;
    float WEAPON_BUMPMINE_MAX_PLAYER_SPEED = 245.0f;

};

} // namespace sim

#endif // SIM_CSGOCONFIG_H
