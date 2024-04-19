#include "sim/CsgoConfig.h"

using namespace sim;

CsgoConfig::CsgoConfig(Corrade::NoInitT)
{
}

CsgoConfig::CsgoConfig(InitWithDzDefaults_Tag)
    : game_mode( GameMode::DANGER_ZONE )
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
    : game_mode( GameMode::COMPETITIVE )
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
