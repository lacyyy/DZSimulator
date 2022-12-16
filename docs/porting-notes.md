## This app currently only runs on Windows. Porting this app to WASM/WebGL (Emscripten), Linux or Mac OS is planned. This is a list of things to consider and test on those platforms.

`@PORTING` tag is used in source code to mark relevant code.

### ON ALL PLATFORMS:
- FIRST, CHECK SDL abilities!
    - Cross-platform message boxes? (SDL_ShowSimpleMessageBox)
    - See if app window is focused?
    - File dialogs?
- Setting game server thread priority? Currently on Windows set with WinAPI.
- Opening a website in the default browser? Currently on Windows done with WinAPI.
- Reduce fps limit once app isn't focused? Cross-platform method for detecting that?
- (excluding WebGL?) Make sure CSGO's install dir is automatically detected (what about unicode paths?)
    - https://www.reddit.com/r/GlobalOffensive/comments/cjhcpy/game_state_integration_a_very_large_and_indepth/
    - On OSX, Steam library folders file in most cases will be found at ~/Library/Application Support/Steam/steamapps/libraryfolders.vdf
    - On Linux, Steam library folders file in most cases will be found at ~/.local/share/Steam/steamapps/libraryfolders.vdf
- Test "portable-file-dialog" library on different platforms. Message Boxes? File Dialogs?

### ON WEBGL:
- MAGNUM_TARGET_WEBGL is defined in "Magnum/Magnum.h" if compiled for WebGL
- Does "portable-file-dialogs" library have EmScripten support yet?
- Emscripten support for Dear Imgui integration
    - The ports branch contains additional patches for Emscripten support that aren't present in master in order to keep the example code as simple as possible.
    - https://doc.magnum.graphics/magnum/examples-imgui.html
    - https://github.com/mosra/magnum-examples/tree/ports/src/imgui


### ON LINUX:
- Test Linux with WSL? Use WSL integration in Visual Studio 2022?
