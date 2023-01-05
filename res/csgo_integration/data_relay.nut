// This is a script written in the Squirrel programming language that can be run
// inside CSGO. Currently, Squirrel 2.2.3 is used in CSGO's VM.
// http://squirrel-lang.org/doc/squirrel2.html
// https://developer.valvesoftware.com/wiki/Squirrel

// This ".nut" version of this script is actually not used by any software.
// It's merely put here to make easier changes before manually converting it to
// ".cfg". So, each time you update this script, make sure to translate the
// changes into the ".cfg" version that actually gets used.

// CAUTION: Printed messages and some variable names are referenced by
//          DZSimulator code, keep it with this script compatible!

// Prefix explanation: DZSDR1 <--> Danger Zone Simulator Data Relay Version 1
// If you make compatibility-breaking changes to this script, make
// sure to increase the version number in all function, variable,
// entity and printed data marker names!
// This way, different versions of this script running simultaneously
// don't interfere.

// Purpose of this script: Every game tick, print game data to the console that
// can then be read by DZSimulator.

function dzsdr1_get_player() { // human player, no bot
    { DZSDR1_X = null }
    while(DZSDR1_X = Entities.FindByClassname(DZSDR1_X, "player")) {
        // If player is in T or CT team
        if(DZSDR1_X.GetTeam() == 2 || DZSDR1_X.GetTeam() == 3) {
            return DZSDR1_X
        }
    }
    return null
}

function dzsdr1_check_loop_end() {
    // Delete timer and stop loop if we haven't seen a keep-alive signal recently.
    // A keep-alive signal can be sent from the outside like this:
    //   script DZSDR1_KEEP_ALIVE_UNTIL <- Time() + 5.0
    if(Time() > DZSDR1_KEEP_ALIVE_UNTIL) {
        if(DZSDR1_TIMER && DZSDR1_TIMER.IsValid()) {
            DZSDR1_TIMER.Destroy()
        }
    }
}

function dzsdr1_loop_tick()
{
    local tick_data_msg = ""
    tick_data_msg += "dzs-dr-v1-tick-start " + DZSDR1_TICK_CNT + "\n"
    
    DZSDR1_PLAYER = dzsdr1_get_player()
    if(DZSDR1_PLAYER)
    {
        local feet_pos = DZSDR1_PLAYER.GetOrigin()
        local eye_pos = DZSDR1_PLAYER.EyePosition()
        local vel = DZSDR1_PLAYER.GetVelocity()

        tick_data_msg += "player "
        tick_data_msg += feet_pos.x + " " + feet_pos.y + " " + feet_pos.z + " "
        tick_data_msg += (eye_pos.z + 0.062561) + " " // eye_pos X and Y is the same as X and Y of feet_pos, don't need to send them again
        tick_data_msg += (DZSDR1_PLAYER.GetBoundingMaxs().z < 72) + " " // is crouching
        tick_data_msg += vel.x + " " + vel.y + " " + vel.z + "\n"
    }

    DZSDR1_CNT = 0
    DZSDR1_X = null
    // print() can only print ~2000 chars, limit the message's max length using a bump mine limit
    while(DZSDR1_CNT < 21 && (DZSDR1_X = Entities.FindByClassname(DZSDR1_X, "bumpmine_projectile"))) {
        local origin = DZSDR1_X.GetOrigin()
        local angles = DZSDR1_X.GetAngles()
        tick_data_msg += "bm " + DZSDR1_X.entindex() + " "
        tick_data_msg += origin.x + " " + origin.y + " " + origin.z + " "
        tick_data_msg += angles.x + " " + angles.y + " " + angles.z + "\n"
        DZSDR1_CNT += 1
    }
    tick_data_msg += "dzs-dr-v1-tick-end\n"

    
    print(tick_data_msg) // Server-side tick data

    // 'getpos' prints client-side view angles and eye position.
    // It's cheat-protected, ensure cheats are on with 'sv_cheats 1'.
    // Send 'echo' before to ensure getpos's response is on its own line.
    SendToConsole("echo;sv_cheats 1;getpos;")

    DZSDR1_TICK_CNT = (DZSDR1_TICK_CNT + 1) % 100

    dzsdr1_check_loop_end()
}

function dzsdr1_start()
{
    const DZSDR1_TIMER_NAME = "dzsdr1_timer"
    DZSDR1_TICK_CNT <- 0
    DZSDR1_KEEP_ALIVE_UNTIL <- Time() + 10.0

    // Replacement for 'local' variables (.cfg version can't use 'local') 
    DZSDR1_X <- null
    DZSDR1_PLAYER <- null
    DZSDR1_CNT <- 0

    // Delete previously existing entities with the same name
    DZSDR1_X = null
    while(DZSDR1_X = Entities.FindByName(DZSDR1_X, DZSDR1_TIMER_NAME)) {
        DZSDR1_X.Destroy()
    }
    
    // Create a new timer
    DZSDR1_TIMER <- Entities.CreateByClassname( "logic_timer" )
    DZSDR1_TIMER.ConnectOutput( "OnTimer", "OnTimer" )

    // This is a hack that turns our logic_timer into a preserved entity,
    // causing it to NOT get reset/destroyed during a round reset!
    DZSDR1_TIMER.__KeyValueFromString("classname", "info_target")

    DZSDR1_TIMER.__KeyValueFromString( "targetname", DZSDR1_TIMER_NAME )
    DZSDR1_TIMER.__KeyValueFromFloat( "refiretime", 0.0 ) // Fire timer every tick

    DZSDR1_TIMER.ValidateScriptScope()
    DZSDR1_TIMER.GetScriptScope().OnTimer <- dzsdr1_loop_tick
    
    EntFireByHandle( DZSDR1_TIMER, "Enable", "", 0.0, null, null )
}

dzsdr1_start()
