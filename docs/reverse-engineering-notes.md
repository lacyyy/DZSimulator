### CS2 REVERSE ENGINEERING PLANS
- Figure out new collision mechanics incl. how bevel planes are created
    - Do players still appear to float on slanted unaligned brush edges like in CSGO?
        - If not, does this indicate that GJK is used? (Does GJK do EXACT intersection tests?)
    - Visualize bevel planes with [this script](https://github.com/GameChaos/cs2_things/blob/main/scripts/vscripts/raytracing.lua). Note that VScripts need to be enabled by patching binaries.
- Does CS2 do the exact same 4-quadrant standable ground check like CSGO does?
- Physics data file format details(?): [Valve dev talk](https://www.youtube.com/watch?v=Nsf2_Au6KxU) about relative pointers and streaming static physics data directly into memory

## CSGO MECHANICS REVERSE ENGINEERING, IDEAS AND FINDINGS 

### FINDINGS (some unsure, a bit old, should retest)
- WASD inputs and viewing angle are only sampled once at tick start
- walk input (shift) has special logic
- what about jump/primary/secondary fire?
- all other input bits are set if their button was pressed anytime in the last tick interval (seemingly)
- bumpmine triggers when trace hits player AABB, but not from the player that threw the bumpmine
- Ladder brushes block grenades/bumpmines!

### USEFUL COMMANDS
- ConVar "cl_pdump 1" prints a lot of movement related values and states
- Could ConVar "r_visualizetraces 1" help understanding knife mechanics? (vid on this topic: https://youtu.be/VRG1cFXOen4)
    - VERY detailed information on DZ knife mechanics: https://youtu.be/HzR-f_SY-Gk?t=94
- Could command "ent_bbox player" help understanding effects of cl_interp and cl_interp_ratio?
- Use "cl_weapon_debug_show_accuracy 2" to reverse taser mechanics

### IDEAS
- **STRAFE MECHANIC**
    - Use `"+left;+moveleft"` keybind, also at different FPS, to see if FPS has effect on strafe efficiency
- **SOLID OBJECTS (from public/bspflags.h)**
    - everything that is normally solid:

    `#define MASK_SOLID (CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_GRATE)`
    - everything that blocks player movement:

    `#define MASK_PLAYERSOLID (CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_GRATE)`
    - water physics in these contents:

    `#define MASK_WATER (CONTENTS_WATER | CONTENTS_MOVEABLE | CONTENTS_SLIME)`
- **GLIDABILITY HEURISTIC:**
    - if player.vel.z < 140 at initial ramp collision tick, probability of being stopped probably decreases with more speed -> Investigate probabilistic correlation
    - if player.vel.z < 140 when sliding along a ramp surface for multiple ticks, being stopped is highly likely when close enough to ground

- Is the near Z clipping plane 8 units in front of the camera?
- The collision model can't be too small. The limit is 0.5 units wide in any direction. That's the smallest a collision piece can get. https://imgur.com/a/l6BkOxA
- vbsp will bite you anyway, it rounds any points within 0.05 units of an integer -> check vbsp doc
- parsing bsp map entitities: entspy output? new entitities appended to the end? checksum? parsable?
- walking up 18.5 unit steps is instant(?), walking down them as well?!
- enable_fast_math cvar? (Turns Denormals-Are-Zeroes and Flush-to-Zero on or off)
- multiple user move commands(with their duration) per tick? -> test with slomo local gotv recording?
- How are Bump Mines slowed down when thrown into water?


- Water jumps?? Kind of hard/random... https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/shared/gamemovement.cpp#L1272

    
