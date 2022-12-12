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
- Could command "ent_bbox player" help understanding effects of cl_interp and cl_interp_ratio?
- Use "cl_weapon_debug_show_accuracy 2" to reverse taser mechanics

### IDEAS
- **DISPLACEMENTS**
    - displacement collision code: https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/public/dispcoll.cpp
    - Could rampslide stops be caused by displacement transitions / sewing / failed sewing? or just awful disp collision?
        - reproducible slide stop, blacksite, 128tick, standing: `ent_fire !self runscriptcode "self.SetOrigin(Vector(-55,-6900,1200));self.SetVelocity(Vector(0,1700,0))"`
    - displacement errors cause them to become not solid. when loading de_dust2, console prints:
        - DISP_VPHYSICS found bad displacement collision face (252.50 1542.13 147.50) (250.00 1543.00 155.00) (250.00 1543.50 155.00) at tri 25
        - DISP_VPHYSICS entire displacement vdisp_0318 will have no collision, dimensions (6.00 14.00 32.00) from (249.00 1537.00 124.00) to (255.00 1551.00 156.00)
- **BRUSHES**
    - collision algorithm? https://youtu.be/YnCG6fbTQ-I
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

    
