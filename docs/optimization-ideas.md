## OPTIMIZATION IDEAS, UNORDERED:

### LOADING TIME REDUCTION:
- PARSING CSGO MAPS:
    - Load packed PHY files in order of their position in the BSP file without reopening it each time?
    - Don't parse/load lumps we don't need (leafface lump? face lump?)

### FRAME TIME REDUCTION:
- Can deferred lighting boost our FPS?
- Only set uniforms each frame that are absolutely necessary? Does setting uniforms cause overhead?
- Look into using geometry shaders, can they make glidability more accurate? Look into tesselation?
    - Optimizations through per-face calculations instead of per-vertex?
        - Maybe do glidability calculation per-face for small faces, but per-vertex for bigger faces?
    - Or is this irrelevant because vertex shader calcs are not the bottleneck?
- Collision meshes: Remove triangles that are completely encompassed by another coll model section?
- Don't draw individual sections of collision models if they're entirely absorbed by solid brushes!
	- Absorption can be checked by testing if all vertices of a section are "behind" all planes of a brush
    - Is this even possible/viable when collision models are drawn using OpenGL's "instancing"?
- Don't clear color buffer? (skybox is always present)
- Resolution scaling option
- Grenade and player clip brushes in the same space should be converted into a normal solid brush
- Halt time when window isn't in focus? Also halt game server (Decrease server thread priority?)
- Reduce DZSim's overlay lag when connected to csgo
    - Really bad lag seems to be caused by the server not being able to keep up. Does `host_thread_mode 1` help? This command is hidden in CSGO (https://github.com/saul/cvar-unhide)
        - Does this allow to periodically send `getpos` at a higher frequency into the console for a smoother overlay? Right now spamming getpos is only effective when sent at 64 Hz.

### RAM USAGE REDUCTION:
- Reduce RAM usage, especially peak usage
    - Decide: Work with mesh faces as triangles (a list of 3 vertices) or as polygons (a list of 3+ vertices). Reduces nested std::vector usage?
        - Should probably just replace std::vector size=3 with a Triangle struct...
    - Does DZSim just eat up more RAM as it loads more maps? Does memory need to be freed?
    - Memory usage while parsing a map
    - Ensure memory layout of BSPMap:: subclasses is minimal
    - Destruct BspMap or parts of it after meshes were parsed from it?

### EXECUTABLE SIZE REDUCTION:
- Replace embedded font files with smaller / less comprehensive ones
- Compress embedded map, font and other files with ZIP compression before embedding
    - Is decoding them again detrimental for startup time?
