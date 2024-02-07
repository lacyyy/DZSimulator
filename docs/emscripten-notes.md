### TODO list for the web port of DZSimulator

- Look through [Magnum's Emscripten-related issues](https://github.com/mosra/magnum/issues?page=2&q=is%3Aissue+emscripten) and note relevant info
- Web app seems to run out of memory after long idling time, why?
    ```
    Aborted(Cannot enlarge memory arrays to size 33558528 bytes (OOM). Either
    (1) compile with -sINITIAL_MEMORY=X with X higher than the current value
    33554432, (2) compile with -sALLOW_MEMORY_GROWTH which allows increasing
    the size at runtime, or (3) if you want malloc to return NULL (0) instead
    of this abort, compile with -sABORTING_MALLOC=0)
    ```
    - NOTE: Displacement collision code gradually creates more caches during gameplay
- Figure out a good way of grabbing the mouse ([this?](https://developer.mozilla.org/en-US/docs/Web/API/Pointer_Lock_API))
- Add more build info
    - MAGNUM_TARGET_WEBGL
    - [Emscripten version](https://github.com/emscripten-core/emscripten/pull/17883)
    - OpenGL version?
    - pthreads multithreading support: __EMSCRIPTEN_PTHREADS__
    - Why is CORRADE_TARGET_32BIT defined?
- License text can't be copied to clipboard
- Replace portable-file-dialogs calls with something else in web builds
    - E.g. glidability shader init error msg
- Figure out changing screen size and entering fullscreen
    - See: https://webglfundamentals.org/webgl/lessons/webgl-resizing-the-canvas.html
- Does GSI or netconport work with CS from the web?
    - Use [Emscripten socket support](https://emscripten.org/docs/porting/networking.html)?
    - Implement GSI with Asio to get rid of cpp-httplib in web builds?
    - If neither GSI or netconport work, get rid of cpp-httplib and Asio in web builds
    - Add note in LICENSES-THIRD-PARTY.txt that some libraries are not used in web builds?
- Get rid of include directories from unused libraries in CMakeLists.txt
- Test VSync and FPS settings. Useless in web builds?
- Test DPI scaling in the web
- Check out Emscripten ports for zlib/bzip2 and Bullet!
- Add an Emscripten Debug build option? What are the benefits? Faster build times?
    - https://doc.magnum.graphics/magnum/platforms-html5.html#platforms-html5-code-size
- Refactor BUILDING.md
- Different browsers must be tested
- Check out [Emscripten speed and memory profiling](https://emscripten.org/docs/porting/Debugging.html#profiling)
- Test if hardware acceleration is disabled in browser and tell user about it
- Magnum has the flags EmscriptenApplication::GLConfiguration::Flag::PowerPreferenceLowPower and Flag::PowerPreferenceHighPerformance
- Is [this](https://developer.chrome.com/articles/file-system-access/) a way to read from a user's CSGO installation? Can this grant access to a directory or just single files?
- When registering a domain for a DZSim website, make sure to register with protection against WHOIS to not get doxxed
