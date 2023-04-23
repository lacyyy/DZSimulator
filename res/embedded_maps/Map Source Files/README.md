## This directory only contains source files that are used by the developer to make map changes and create a compiled map file that can be embedded/compiled into DZSimulator's executable.

## None of these source files get compiled into DZSimulator's executable.

----

### Requirements of an embeddable map file:
 - Must be as small as possible
 - Every asset (collision model, image, etc.) the map uses _and is necessary for DZSimulator to load the map_ must be packed inside the embeddable map file
 - The map must not use any assets owned by Valve because we don't have the right to redistribute them

### How to create an embeddable map file from these source files:

1. Open the `.vmf` file in CSGO's level editor, preferably [Hammer++](https://developer.valvesoftware.com/wiki/Hammer%2B%2B)
1. While making your changes to the map, make sure to
    - turn every brush _inside the map_ into a `func_detail`
    - not use too many different prop models as their collision models may contribute a lot to final file size
1. Bring up the map compile menu by pressing `F9` in Hammer
1. Switch to the "Expert" view and configure the compilation as follows:
    - From the configurations, select `Fast` (or a configuration copied from `Fast`)
    - From the compile commands, only check the BSP command, uncheck all other commands
    - Select the BSP command to modify its properties
    - Add `-autoviscluster 4096 -blocksize 8192` to the front of the BSP command's parameter list. The entire parameter line may look like:
      ```
      -autoviscluster 4096 -blocksize 8192 -game $gamedir $path\$file
      ```
1. Press the "Go!" button and make sure the compilation succeeds
1. Remove ALL packed files from the `.bsp` file you just created. Instructions to do this with [VIDE](https://developer.valvesoftware.com/wiki/VIDE):
    - Go to: Tools > Pakfile Lump Editor
    - From "BSP Options", "Open" the `.bsp` you just created
    - Select all files in the list (e.g. by pressing Ctrl+A)
    - From "File Options", press the "Un/Delete" button
    - From "BSP Options", press the "Save" button
1. Now pack all files that are required for DZSimulator to load the map (even if you removed them in the previous step), e.g. `.phy` files of every model that is used in the map. Instructions to do this with [VIDE](https://developer.valvesoftware.com/wiki/VIDE), assuming you still have the now unpacked `.bsp` opened in the "Pakfile Lump Editor":
    - From "Pakfile Options", "Add" the required files. Make sure each file is packed under the correct path.
    - From "BSP Options", press the "Save" button
1. Run the `.bsp` file through the "MapFileShrinker-CSGO.py" script (located in the `tools/` directory) to remove map data that DZSimulator doesn't care about, such as lighting data. The following might be entered into the command line to do this:
    ```
    python "PATH\TO\DZSIM-REPO\tools\MapFileShrinker-CSGO.py" "PATH\TO\DZSIM-REPO\res\embedded_maps\Map Source Files\YOUR-MAP\YOUR-MAP.bsp"
    ```
1. Your `.bsp` file should now be significantly smaller and ready to get embedded into DZSimulator. Move the file to `res/embedded_maps/YOUR-MAP.bsp`
1. Add an entry to DZSimulator's resource configuration file to let the map file get compiled into DZSimulator's executable:
    ```
    [file]
    filename=embedded_maps/YOUR-MAP.bsp
    ```
1. Add your map to DZSimulator code that loads embedded maps
