NOTE:
- Building DZSimulator is only tested and recommended with Visual Studio 2022! These instructions for building with VSCode or the command line might not work!
- Mainly, building for Windows is explained here. Rudimentary explanations to build for the web (Emscripten/WASM) can be found towards the end of the document.
- Every `.cpp` file that gets added to the project must be added to the `target_sources()` command in the top-level [CMakeLists.txt](CMakeLists.txt) to be considered for compilation. Visual Studio might be able to do that semi-automatically, but double check it!
- Visual Studio 2019 version 16.5 or later is recommended as it makes CMake project manipulation available. (Automatic target and source file adding, removing, and renaming in the CMake project without manually editing the CMake scripts)

## <ins>STEP 1: Downloading the repository's contents</ins>

Depending on your situation, downloading the entire repo requires different steps with varying effort.

### CASE 1
Clone the repository recursively with
```
git clone https://github.com/lacyyy/DZSimulator.git --recursive
```

### CASE 2
If the repository was cloned non-recursively previously, you can get the missing submodules with
```
git submodule update --init --recursive
```

### CASE 3
If for some reason, you need to build this repo and it has empty submodule folders and no Git history (which is what you get when selecting "Download ZIP" or the default "Download Source code" on GitHub's releases page), you need to add the submodules manually!

1. Delete all empty folders inside `thirdparty/`, the next steps will fail otherwise. Deleting the top-level `.gitmodules` file too is recommended. 
1. In the repo folder (where `docs/`, `res/`, `src/` and `thirdparty/` reside), run:
    ```
    git init
    git add -A
    git commit -m "Added non-submodule DZSimulator files"
    ```

1. Next, run these commands without changing the working directory to get the required submodules at specific versions (You can ignore the 'detached HEAD' advice): 
    ```
    git submodule add https://github.com/chriskohlhoff/asio.git thirdparty/asio/
    git submodule add https://github.com/mosra/corrade.git thirdparty/corrade/
    git submodule add https://github.com/ocornut/imgui.git thirdparty/imgui/
    git submodule add https://github.com/mosra/magnum.git thirdparty/magnum/
    git submodule add https://github.com/mosra/magnum-integration.git thirdparty/magnum-integration/
    git submodule add https://github.com/mosra/magnum-plugins.git thirdparty/magnum-plugins/
    git submodule add https://github.com/mosra/toolchains.git thirdparty/toolchains/
    git submodule add https://github.com/libsdl-org/SDL.git thirdparty/SDL/
    git -C thirdparty/asio/ checkout asio-1-23-0
    git -C thirdparty/corrade/ checkout c548c45aff4e1f08c707a61775ab3131ef735739
    git -C thirdparty/imgui/ checkout v1.88
    git -C thirdparty/magnum/ checkout 4b765e9b12ed617e435ca49695f8ef7e66af654b
    git -C thirdparty/magnum-integration/ checkout 1a66b05bd7db0a5484366054ddc678bebf79921e
    git -C thirdparty/magnum-plugins/ checkout 7d1aba3fda3324ac3f0d0694e673350c1fbd55f8
    git -C thirdparty/toolchains/ checkout 65568a98fa48de0369f35e9788779fdfbe14cacc
    git -C thirdparty/SDL/ checkout release-2.0.22
    ```
1. Check if the newly installed submodules have the right version by running `git submodule status`. The library versions are correct if you see the following hash values for each one:
    ```
    53dea9830964eee8b5c2a7ee0a65d6e268dc78a1 thirdparty/SDL (release-2.0.22)
    4915cfd8a1653c157a1480162ae5601318553eb8 thirdparty/asio (asio-1-23-0)
    c548c45aff4e1f08c707a61775ab3131ef735739 thirdparty/corrade (v2020.06-1383-gc548c45a)
    9aae45eb4a05a5a1f96be1ef37eb503a12ceb889 thirdparty/imgui (v1.62-2483-g9aae45eb)
    4b765e9b12ed617e435ca49695f8ef7e66af654b thirdparty/magnum (v2020.06-2375-g4b765e9b1)
    1a66b05bd7db0a5484366054ddc678bebf79921e thirdparty/magnum-integration (v2020.06-181-g1a66b05)
    7d1aba3fda3324ac3f0d0694e673350c1fbd55f8 thirdparty/magnum-plugins (v2020.06-1205-g7d1aba3f)
    65568a98fa48de0369f35e9788779fdfbe14cacc thirdparty/toolchains (heads/master)
    ```

1. Then commit these submodules into Git:
    ```
    git add -A
    git commit -m "Added submodule DZSimulator files"
    ```


## <ins>STEP 2: Building the application (for Windows)</ins>

NOTE: Build instructions of option 2 and 3 sometimes didn't work, perhaps installing Visual Studio with choosing the "Desktop development with C++" workload is always required...

### OPTION 1 (RECOMMENDED): **Building with Visual Studio**
- Visual Studio 2022 or newer is recommended, Visual Studio 2019 might work too with CMake folder projects
- Choose "Open a local folder" or File > Open > Folder and select the repo folder (where `docs/`, `res/`, `src/` and `thirdparty/` reside)
- Make sure CMake presets are used in Visual Studio by setting: Tools > Options > CMake > General > "Prefer using CMake presets" or "Always use CMakePresets.json" or "Use CMake Presets if available"
    - Then close and reopen the repo folder in Visual Studio to activate the integration
- Select the desired CMake configure preset (e.g. "Windows x64 Debug" or "Windows x64 Release") in the toolbar
- Wait for CMake to finish configuring
- Right-click `src/main.cpp` and select "Set as Startup Item"
- Press Ctrl+B to build

### OPTION 2: **Building with Visual Studio Code**

1. Install VSCode
1. Install the C/C++ extension for VS Code, also install the CMake and CMake Tools extension
1. Download the [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022). When you run the downloaded executable, it updates and runs the Visual Studio Installer. To install only the tools you need for C++ development, select the "Desktop development with C++" workload.
1. Open "x64 Native Tools Command Prompt" from your recently installed Visual Studio tools to build 64-bit code (You can just enter the prompt into Windows search to find it)
1. In that command prompt, change directory to the DZSimulator repo folder (where `docs/`, `res/`, `src/` and `thirdparty/` reside)
1. Run `code .` to open VSCode in that directory
1. In VSCode, select a configure preset (e.g. "Windows x64 Debug" or "Windows x64 Release")
1. In VSCode, build the default build preset and wait for compilation to finish
1. In VSCode, select launch target "DZSimulator.exe" and launch it

### OPTION 3: **Building from the command line** ([CMake](https://cmake.org/) >= 3.20 is required)

1. Download the [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022). When you run the downloaded executable, it updates and runs the Visual Studio Installer. To install only the tools you need for C++ development, select the "Desktop development with C++" workload.
1. Open "x64 Native Tools Command Prompt" from your recently installed Visual Studio tools to build 64-bit code (You can just enter the prompt into Windows search to find it)
1. In that command prompt, change directory to the DZSimulator repo folder (where `docs/`, `res/`, `src/` and `thirdparty/` reside)
1. Check available presets with
    ```
    cmake --list-presets=all
    ```
1. Generate project files (Choose command with your desired preset):
    ```
    cmake -G "Visual Studio 17 2022" -A x64 --preset win-x64-debug
    cmake -G "Visual Studio 17 2022" -A x64 --preset win-x64-release-static
    cmake -G "Visual Studio 17 2022" -A x64 --preset win-x64-release-static-debuggable
    ```
1. Build (Choose command with your desired preset):
    ```
    cmake --build out/build/win-x64-debug --parallel --config Debug
    cmake --build out/build/win-x64-release-static --parallel --config Release
    cmake --build out/build/win-x64-release-static-debuggable --parallel --config RelWithDebInfo	
    ```
1. Run the binary (Choose command with your desired preset):
    ```
    out\build\win-x64-debug\Debug\bin\DZSimulator.exe
    out\build\win-x64-release-static\Release\bin\DZSimulator.exe
    out\build\win-x64-release-static-debuggable\Release\bin\DZSimulator.exe
    ```

## <ins>Appendix: Building for the web (Emscripten/WASM):</ins>

1. First, you need to install [Emscripten](https://emscripten.org), version `3.1.20` specifically. Other versions might not behave as expected. Please refer to the official install instructions, but
these commands might do the job if you're on Windows:
    ```
    cd /YOUR/EMSCRIPTEN/INSTALL/DIR/
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    emsdk install 3.1.20
    emsdk activate 3.1.20
    emsdk_env.bat
    ```
1. Then create a new `CMakeUserPresets.json` file in DZSimulator's repo and put in the following:
    ```
    {
        "version": 2,
        "configurePresets": [
            {
                "name": "emscripten-wasm-release",
                "inherits": "BASE-emscripten-wasm-release",
                "displayName": "Web (Emscripten) Release",
                "description": "Target WebAssembly and WebGL 2.0 (based on OpenGL ES 3.0) in Release mode. You must build 'Windows x64 Release' before building this!",
                "cacheVariables": {
                    "EMSCRIPTEN_PREFIX": "/YOUR/EMSCRIPTEN/INSTALL/DIR/emsdk/upstream/emscripten",
                    "CMAKE_INSTALL_PREFIX": "${sourceDir}/out/install/${presetName}"
                }
            }
        ]
    }
    ```
    and replace `/YOUR/EMSCRIPTEN/INSTALL/DIR/` with the path of where you've just installed Emscripten
    - Optionally, you can change the value of `CMAKE_INSTALL_PREFIX` to wherever you want the final files needed by the website to be placed by CMake's `INSTALL` target
1. The Emscripten option should now appear in the configure preset dropdown in Visual Studio's toolbar, select it
    - This should automatically configure the project (which takes a while for Emscripten). If it doesn't configure automatically, go into the Solution Explorer, right-click the top-level `CMakeLists.txt` and select "Configure Cache" or "Delete Cache and Reconfigure"
1. If configuring succeeded, start building by hitting Ctrl+Shift+B or by selecting Build > Build All
1. If building succeded and you want to place all newly built files required by the website in the directory specified by `CMAKE_INSTALL_PREFIX` in your `CMakeUserPresets.json` file, select Build > Install DZSimulatorProject

### Additional notes for web builds:
- You must have built "Windows x64 Release" at least once before attempting to build for Emscripten
- A website running DZSimulator must be configured as "[cross-origin isolated](https://web.dev/coop-coep/)" because DZSimulator uses the `SharedArrayBuffer` feature
- To quickly test the website on your local machine:
    1. Start Chrome with a flag that ignores the "cross-origin isolated" requirement
        ```
        "YOUR/CHROME/INSTALL/DIR/chrome.exe" --enable-features=SharedArrayBuffer
        ```
    1. Start a non-"cross-origin isolated" HTTP server
        ```
        cd /YOUR/DZSIMULATOR/WEBSITE/FILES/DIR/
        python -m http.server
        ```
    1. Go to `http://localhost:8000` in Chrome to visit the DZSimulator website
- DZSimulator's web build uses a fixed amount of memory
    - You should specify the maximum amount of used memory in the top-level `CMakeLists.txt` through the `DZSIM_WEB_PORT_MAX_MEM_USAGE` variable
        - Note: More and more displacement collision caches are created during gameplay, test the worst case of this!
- DZSimulator's web build silently stack-overflows if assertions are disabled
    - In the top-level `CMakeLists.txt` you can
        - enable/disable assertions through an Emscripten linker option
        - increase/decrease the stack size through Emscripten linker options
