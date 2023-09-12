## Porting DZSimulator to Linux or Mac OS is planned. This is a list of things to consider and test on those platforms.

`@PORTING` tag is used in source code to mark relevant code.

### ON ALL PLATFORMS:
- FIRST, CHECK SDL abilities!
    - Cross-platform message boxes? (SDL_ShowSimpleMessageBox)
    - See if app window is focused?
    - File dialogs?
- Setting game server thread priority? Currently on Windows set with WinAPI.
- Opening a website in the default browser? Currently on Windows done with WinAPI.
- Reduce fps limit once app isn't focused? Cross-platform method for detecting that?
- (excluding WebGL) Make sure CSGO's install dir is automatically detected (what about unicode paths?)
    - https://www.reddit.com/r/GlobalOffensive/comments/cjhcpy/game_state_integration_a_very_large_and_indepth/
    - On OSX, Steam library folders file in most cases will be found at ~/Library/Application Support/Steam/steamapps/libraryfolders.vdf
    - On Linux, Steam library folders file in most cases will be found at ~/.local/share/Steam/steamapps/libraryfolders.vdf
- Test "portable-file-dialog" library on different platforms. Message Boxes? File Dialogs?

### ON LINUX:
- First, try running the Windows version of DZSimulator on Linux using WINE!
- Test Linux with WSL? Use WSL integration in Visual Studio 2022?
- Use Visual Studio's [default Linux configure preset](https://learn.microsoft.com/en-us/cpp/build/cmake-presets-vs?view=msvc-170#linux-example)?
