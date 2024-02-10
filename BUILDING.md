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
    git submodule add https://github.com/wolfpld/tracy.git thirdparty/tracy/
    git -C thirdparty/asio/ checkout asio-1-23-0
    git -C thirdparty/corrade/ checkout 62d566efca9a661234d9c6a2c5885ac14727783f
    git -C thirdparty/imgui/ checkout v1.90.1
    git -C thirdparty/magnum/ checkout 6394c85c06a5eb67713763c7e471e0fef3160c41
    git -C thirdparty/magnum-integration/ checkout 05cbe5f85593b7d4252048df98f0bc3bb48b540d
    git -C thirdparty/magnum-plugins/ checkout cef3912083b9e1adb6379429d0940be1c51fc111
    git -C thirdparty/toolchains/ checkout 2149f703ac890afade1ca9fd042ba82478f41d1c
    git -C thirdparty/SDL/ checkout release-2.0.22
    git -C thirdparty/tracy/ checkout v0.10
    ```
1. Check if the newly installed submodules have the right version by running `git submodule status`. The library versions are correct if you see the following hash values for each one:

    ```
    53dea9830964eee8b5c2a7ee0a65d6e268dc78a1 thirdparty/SDL (release-2.0.22)
    4915cfd8a1653c157a1480162ae5601318553eb8 thirdparty/asio (asio-1-23-0)
    62d566efca9a661234d9c6a2c5885ac14727783f thirdparty/corrade (v2020.06-1585-g62d566ef)
    d6cb3c923d28dcebb2d8d9605ccc7229ccef19eb thirdparty/imgui (v1.62-3271-gd6cb3c92)
    6394c85c06a5eb67713763c7e471e0fef3160c41 thirdparty/magnum (v2020.06-2765-g6394c85c0)
    05cbe5f85593b7d4252048df98f0bc3bb48b540d thirdparty/magnum-integration (v2020.06-207-g05cbe5f)
    cef3912083b9e1adb6379429d0940be1c51fc111 thirdparty/magnum-plugins (v2020.06-1381-gcef39120)
    2149f703ac890afade1ca9fd042ba82478f41d1c thirdparty/toolchains (heads/master)
    37aff70dfa50cf6307b3fee6074d627dc2929143 thirdparty/tracy (v0.10)
    ```

1. Then commit these submodules into Git:
    ```
    git add -A
    git commit -m "Added submodule DZSimulator files"
    ```

## <ins>STEP 2: Building the application (for Windows)</ins>

NOTE: Build instructions of option 2 and 3 sometimes didn't work, perhaps installing Visual Studio with choosing the "Desktop development with C++" workload is always required...

### PREREQUISITE: **Installing OpenSSL**

No matter which of the options you choose later, you need to install OpenSSL first. This needs to be done only the first time and after that only when DZSimulator switches to newer OpenSSL versions. 

1. Install [Conan](https://conan.io/downloads)
1. Determine how OpenSSL is built by running `conan profile detect --force`
    - (I'm unsure about this: It should probably be ensured that this detected profile is set to the MSVC compiler that's also used for DZSim compilation, targeting x86_64. To change a profile's settings, see [this](https://docs.conan.io/2/reference/config_files/settings.html#reference-config-files-settings-yml).)
1. For every build type you want to build DZSimulator in, you need to install OpenSSL separately for that build type by running the respective command:

    ```
    conan install conanfile_ossl.py -b=missing --output-folder=out/openssl/win-x64-debug/ -s build_type=Debug
    conan install conanfile_ossl.py -b=missing --output-folder=out/openssl/win-x64-release/ -s build_type=Release
    conan install conanfile_ossl.py -b=missing --output-folder=out/openssl/win-x64-release-minsize/ -s build_type=MinSizeRel
    conan install conanfile_ossl.py -b=missing --output-folder=out/openssl/win-x64-release-w-profiling/ -s build_type=RelWithDebInfo
    ```

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
1. In VSCode, select the build preset with the same name, build it and wait for compilation to finish
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
    cmake --preset=win-x64-debug
    cmake --preset=win-x64-release
    cmake --preset=win-x64-release-minsize
    cmake --preset=win-x64-release-w-profiling
    ```
1. Build (Choose command with the same preset as in the last step):
    ```
    cmake --build --preset=win-x64-debug
    cmake --build --preset=win-x64-release
    cmake --build --preset=win-x64-release-minsize
    cmake --build --preset=win-x64-release-w-profiling
    ```

## <ins>Appendix: Building for the web (Emscripten/WASM):</ins>

1. First, you need to install [Emscripten](https://emscripten.org), version `3.1.20` specifically. Other versions might not behave as expected. Please refer to the official install instructions, but these commands might do the job if you're on Windows:
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
