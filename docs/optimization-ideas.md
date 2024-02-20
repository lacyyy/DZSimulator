## OPTIMIZATION IDEAS, UNORDERED:

`@OPTIMIZATION` tag is used in source code to mark relevant code.

### LOADING TIME REDUCTION:

- Compile with Clang or GCC instead of MSVC, any speed improvements?
- PARSING CSGO MAPS:
    - Take a look at [Kaitai Struct](https://kaitai.io)
    - Load struct fields directly into memory IF it's safe to assume that PCs today all have twos-complement and little-endian architectures
        - Are there guarantees for this on WASM/Emscripten?
        - Related: https://en.cppreference.com/w/cpp/types/is_trivially_copyable
        - Also, check FP-representation? https://en.cppreference.com/w/cpp/types/numeric_limits/is_iec559
        - See [Valve dev talk](https://www.youtube.com/watch?v=Nsf2_Au6KxU) about relative pointers and streaming static physics data directly into memory
    - Load packed PHY files in order of their position in the BSP file without reopening it each time?
    - Don't parse/load lumps we don't need (leafface lump? face lump?)

### FRAME TIME REDUCTION:

- Reducing vertex data on the GPU might increase FPS significantly
    - Change GPU vertex data layout to minimize memory footprint?
	- https://x.com/SebAaltonen/status/1731317041032770018?s=20
- Reduce triangle count: Combine triangles/quads that are connected and have the same normals, if possible
- Move trajectory calculations from the vertex shader to the fragment shader?
- Some form of occlusion culling?
    - Only worth it for big complex props?
    - https://github.com/nvpro-samples/gl_occlusion_culling
- World-Player collision detection
    - Measure impact of __forceinline in displacement collision code
- During gameplay, displacement collision cache creation might infrequently cause stutters. Measure!
- Can deferred lighting boost our FPS?
- Only set uniforms each frame that are absolutely necessary? Does setting uniforms cause overhead?
- Look into using geometry shaders, can they make glidability more accurate? Look into tesselation?
    - Optimizations through per-face calculations instead of per-vertex?
        - Maybe do glidability calculation per-face for small faces, but per-vertex for bigger faces?
    - Or is this irrelevant because vertex shader calcs are not the bottleneck?
- Vertex shader of GlidabilityShader3D: Avoid divisions by precalculating inverse values? Or by restructuring equations?
- Collision meshes: Remove triangles that are completely encompassed by another coll model section?
- Don't draw individual sections of collision models if they're entirely absorbed by solid brushes!
    - Absorption can be checked by testing if all vertices of a section are "behind" all planes of a brush
    - Is this even possible/viable when collision models are drawn using OpenGL's "instancing"?
- Don't draw displacement triangles if they're entirely absorbed by solid brushes!
    - Test: Calc percentage of total count of affected triangles
- Don't clear color buffer? (skybox is always present)
- Resolution scaling option
- Grenade and player clip brushes in the same space should be converted into a normal solid brush
- Halt time when window isn't in focus? Also halt game server (Decrease server thread priority?)
- Reduce DZSim's overlay lag when connected to csgo
    - Really bad lag seems to be caused by the server not being able to keep up. Does `host_thread_mode 1` help? This command is hidden in CSGO (https://github.com/saul/cvar-unhide)
        - Does this allow to periodically send `getpos` at a higher frequency into the console for a smoother overlay? Right now spamming getpos is only effective when sent at 64 Hz.
- Web version:
    - Does the DZSimulator browser tab always have high CPU usage, even when not selected/minimized? Is this avoidable?

### RAM USAGE REDUCTION:

- Reduce RAM usage, especially peak usage
    - Decide: Work with mesh faces as triangles (a list of 3 vertices) or as polygons (a list of 3+ vertices). Reduces nested std::vector usage?
        - Should probably just replace std::vector size=3 with a Triangle struct...
    - Does DZSim just eat up more RAM as it loads more maps? Does memory need to be freed?
    - Memory usage while parsing a map
    - Ensure memory layout of BSPMap:: subclasses is minimal
    - Destruct BspMap or parts of it after meshes were parsed from it?
    - More and more displacement collision caches are created during gameplay. Maybe delete caches of displacements that are far away or infrequently collided with?

### EXECUTABLE SIZE REDUCTION:

- Use [Bloaty](https://github.com/google/bloaty)
- Use [SizeBench](https://github.com/microsoft/SizeBench) for windows EXEs
- Use [weave](https://github.com/evmar/weave) for WASM files
- Downgrade SDL 2 to an older version?
- Replace embedded font files with smaller / less comprehensive ones
- Compress embedded map, font and other files with ZIP compression before embedding
    - Is decoding them again detrimental for startup time?
    - Improve compression (ratio) of embedded map files by setting unused fields within used lumps to 0 ?
- Web version:
    - Also affects speed: Get rid of exceptions (is this possible?) and disable them in Emscripten build
        - Alternative: Use more performant (?) [WASM exception proposal](https://emscripten.org/docs/porting/exceptions.html#webassembly-exception-handling-based-support)
    - [Configure server-side compression of WASM executable](https://doc.magnum.graphics/magnum/platforms-html5.html#platforms-html5-code-size-server)

### COMPILE TIME REDUCTION:

- Try out unity build?
- Use a tool that analyzes a project's `#include`s
- nlohmann's single-header JSON library might contribute a lot to build time
- https://vitaut.net/posts/2024/faster-cpp-compile-times/
