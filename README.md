# DANGER ZONE SIMULATOR

![Latest Version](https://img.shields.io/github/v/tag/lacyyy/DZSimulator?label=version)
![Latest Release Date](https://img.shields.io/github/release-date/lacyyy/DZSimulator)
![Total Downloads](https://img.shields.io/github/downloads/lacyyy/DZSimulator/total?label=total%20downloads)

A standalone app that partially loads maps from Counter-Strike:Global Offensive and makes practicing rampsliding and using "Bump Mines" in CSGO's "Danger Zone" gamemode much easier!

If you don't know what I mean with rampsliding and Bump Mines, watch this: https://youtu.be/xmAYeyYd4AE

----

![Demonstration screenshot 1](docs/media/demo_img_1.png)
Example of DZSimulator showing the map dz_sirocco and visualizing surfaces that are suitable for rampsliding in green color

TODO: Add more images and/or video for demonstration

----

## <ins>Requirements</ins>

Steam and CSGO must be installed on your machine so that DZSimulator is able to load CSGO maps.

DZSimulator can be used by just itself to explore maps, but it can also act as a CSGO overlay!

If DZSimulator is used as a CSGO overlay (works only in local server), make sure that:
- CSGO is in "Fullscreen Windowed mode"
- CSGO was started with the launch option `-netconport 34755`
- CSGO loaded a map locally (e.g. with console commands `game_mode 0;game_type 6;map dz_blacksite`) and DZSimulator has the same map loaded
- In DZSimulator's "Visualizations" menu, "Glidability for player in local CS:GO session" is enabled and you pressed "CONNECT" once CSGO was started
- In DZSimulator's "Video Settings" menu, overlay mode is enabled

To get started with practicing Bump Mines, I recommend watching [this](https://youtu.be/IPWxlnEsLkQ) and [this](https://youtu.be/YblZkx7mXFM) video!


## <ins>Is this a cheat? Can my Steam account get VAC-banned for using this?</ins>

No, this is not a cheat and can't get you VAC-banned!

DZSimulator doesn't modify CSGO's game files or injects code into CSGO's DLL files, like a cheat would. Instead, it just reads CSGO's map files to display them in its own window and then simulates CSGO gameplay and movement within itself to figure out rampsliding spots, without touching the CSGO process.

There's one more feature that needs to be explained: DZSimulator's ability to read the player's movement from within a local CSGO server: Is this cheat behaviour? No! This works legitimately through CSGO's launch option `-netconport`. It allows outside programs to connect to CSGO's console, send commands and receive console output. So when CSGO is inside a local server, `sv_cheats 1` can be enabled and DZSimulator is then able to get the player position through the `getpos` command! This doesn't work in online matches because the server is required to have `sv_cheats` set to `1` in order for `getpos` to work.

Hence, you can't get VAC-banned for using DZSimulator.

## <ins>Download</ins>

**Please note that this is a very early version and can contain bugs! If you encounter any, please report them to me through this page's "Issues" tab or through my [Twitter DMs](https://twitter.com/lacyyycs)!**

You can download the newest version of DZSimulator for Windows [**here**](https://github.com/lacyyy/DZSimulator/releases/latest).

Adding a download for GNU/Linux machines is planned in the future!
Using DZSimulator in the web browser might be possible in the future too!

## You can stop reading here if you just want to try out DZSimulator! 

----
Information for developers follows here.

### Repository Structure
- `docs/` - Developer notes, including planned features.
- `res/` - Files that get compiled into the executable (top-level [LICENSES-THIRD-PARTY.txt](LICENSES-THIRD-PARTY.txt) too). Contains third party software!
- `src/` - DZSimulator source code. Also contains third party software!
- `thirdparty/` - Other third party software, some are used, some not.
- `tools/` - Useful scripts for DZSimulator development

### License Notes

See [LICENSE.txt](LICENSE.txt). ***Read it***, there are some important license clarifications.

All third party software in this repo is accompanied by their corresponding license information. An exception from this is source code stemming from Valve Corp.'s [source-sdk-2013](https://github.com/ValveSoftware/source-sdk-2013): It's clearly marked with comments like *`// ---- start of source-sdk-2013 code ----`* and its "Source 1 SDK License" can be read in [LICENSES-THIRD-PARTY.txt](LICENSES-THIRD-PARTY.txt).

DZSimulator uses some of the third party software present in this repo. Those that are used in final release builds and require legal notices when distributed in **binary** form are listed in [LICENSES-THIRD-PARTY.txt](LICENSES-THIRD-PARTY.txt).

Most software in this repo is licensed under permissive licenses. One notable exception is source code stemming from [source-sdk-2013](https://github.com/ValveSoftware/source-sdk-2013) that is licensed under the "Source 1 SDK License", imposing a few restrictions, **including only being allowed to distribute your derived software for free!** You can read its full details in [LICENSES-THIRD-PARTY.txt](LICENSES-THIRD-PARTY.txt).

DZSimulator source code that is not part of any third party software is available under the [MIT License](LICENSE.txt). See the notes in [LICENSE.txt](LICENSE.txt) to know how to distinguish DZSimulator source code from third party software.

**List of all currently used third party software:**
| Name | Description | License |
| ---- | ----------- | ------- |
| [SDL](https://www.libsdl.org) | Multimedia library | **zlib License** (see `thirdparty/SDL/LICENSE.txt`) |
| [Corrade](https://github.com/mosra/corrade) | Utility base of Magnum | **MIT License** (see `thirdparty/corrade/COPYING`) |
| [Magnum](https://github.com/mosra/magnum) | Graphics middleware | **MIT License** (see `thirdparty/magnum/COPYING`) |
| [Magnum Integration](https://github.com/mosra/magnum-integration) | Dear ImGui integration | **MIT License** (see `thirdparty/magnum-integration/COPYING`) |
| [Magnum Plugins](https://github.com/mosra/magnum-plugins) | Plugins for Magnum | **MIT License** (see `thirdparty/magnum-plugins/COPYING`) |
| [flextGL](https://github.com/mosra/flextgl) | Used by Magnum's OpenGL wrapping layer | [**MIT License**](https://github.com/mosra/flextgl/blob/master/COPYING) |
| [Dear ImGui](https://github.com/ocornut/imgui) | GUI library | **MIT License** (see `thirdparty/imgui/LICENSE.txt`) |
| [podgorskiy/fsal](https://github.com/podgorskiy/fsal) | File reading (VPK archive support) | [**MIT License**](thirdparty/fsal_modified/fsal/LICENSE) |
| [Asio](https://think-async.com/Asio/) | Networking library | **Boost Software License** (see `thirdparty/asio/asio/LICENSE_1_0.txt`) |
| [OpenSSL](https://github.com/openssl/openssl) | TLS library used by cpp-httplib | [**Apache License 2.0**](http://www.apache.org/licenses/LICENSE-2.0) |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | HTTP server and client library | [**MIT License**](thirdparty/cpp-httplib/LICENSE) |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing library | [**MIT License**](thirdparty/json/LICENSE.MIT) |
| [portable-file-dialogs](https://github.com/samhocevar/portable-file-dialogs) | GUI dialogs library | [**DWTFYWT License**](thirdparty/portable-file-dialogs/COPYING) |
| [Noto Sans fonts](https://fonts.google.com/noto/specimen/Noto+Sans) | In-app fonts | [**SIL Open Font License**](res/fonts/OFL.txt) |
| [source-sdk-2013](https://github.com/ValveSoftware/source-sdk-2013) | Game engine SDK | [**Source 1 SDK License**](LICENSES-THIRD-PARTY.txt) |
| [Tracy](https://github.com/wolfpld/tracy) | Profiler, not used in release builds | **3-clause BSD License** (see `thirdparty/tracy/LICENSE`) |

Third party software that is used but isn't listed here was released into the public domain.

NOTE: If you're going to use additional libraries from Magnum or Corrade, make sure to comply to their third-party components' license terms if they depend on any:
- [Magnum's third party components](https://doc.magnum.graphics/magnum/credits-third-party.html)
- [Corrade's third party components](https://doc.magnum.graphics/corrade/corrade-credits-third-party.html)

### Building DZSimulator

See [BUILDING.md](BUILDING.md)

### Contributing

Feel free to contribute! If you plan to do so, please message me beforehand (e.g. through [Twitter DMs](https://twitter.com/lacyyycs) or in the "Issues" tab on this repo's webpage)



