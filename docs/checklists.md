## Checklists for making releases and updating libraries or fonts

### Setting up public DZSimulator repo:
- Add Magnum tag to GitHub repo

### Releasing a new major/minor/patch/tweak update
1. Increase project version in top-level CMakeLists.txt
1. Write changelogs for GitHub and ingame display
1. In the parent directory of the repo, run:
       `tar --exclude="DZSimulator/.git" --exclude="DZSimulator/.vs" --exclude="DZSimulator/out" -zcvf "DZSimulator-vX.X.X-Source-code-with-submodules.tar.gz" DZSimulator`
1. In Visual Studio, right-click top-level CMakeLists.txt > Configure DZSimulatorProject
1. Build executable
1. Rename executable("DZSimulator-vX.X.X.exe") and source code archive with new version number
1. Make sure in-app build timestamp is correctly updated
1. Commit new version tag and push it:
    ```
    git commit -m "Bumped DZSimulator version to X.X.X"
    git tag vX.X.X
    git push origin --tags
    ```
1. Make a release on GitHub, write changes and attach source code archive and the executable(s)

### Updating any of Magnum/Corrade repos
1. If project suddenly stops building after a Magnum upgrade, see https://doc.magnum.graphics/magnum/troubleshooting.html
1. Read change notes since last version
1. Download all 4 up to date repos and replace the old ones (keep folder names!)
1. Update licenses in LICENSES-THIRD-PARTY.txt, comply to them if they changed
1. Update commit hash of new magnum, magnum-plugins,
    magnum-integration and corrade in BUILDING.md
1. Replace "Find*.cmake" files in `/thirdparty/_cmake_modules/` with newer versions
    found in `/thirdparty/magnum/modules/`, `/thirdparty/corrade/modules/`,
    `/thirdparty/magnum-plugins/modules/` and `/thirdparty/magnum-integration/modules/`
1. Right-Click top-level CMakeLists.txt > Configure DZSimulatorProject
1. Build again and test

### Updating / Adding other third party libraries
1. Obey its (new) license requirements
1. Update license (or add it to) LICENSES-THIRD-PARTY.txt if necessary for binary distribution
1. Set/Update library path in top-level CMakeLists.txt
1. Add to/Update third party software list in README.md and in build info inside app
1. Add to/Update BUILDING.md instructions
1. Right-Click top-level CMakeLists.txt > Configure DZSimulatorProject

### Updating / Adding fonts
1. Check license and add license to LICENSES-THIRD-PARTY.txt if necessary for redistribution
1. Add to third party software list in README.md
1. Right-Click top-level CMakeLists.txt > Configure DZSimulatorProject

### Add a submodule
`git submodule add <URL> thirdparty/<MODULE-NAME>/`

### Occasionally update the submodule to a new version:
`git -C thirdparty/<MODULE-NAME>/ checkout <new version>`

`git add thirdparty/<MODULE-NAME>/`

`git commit -m "update submodule to new version"`

### See the list of submodules in a superproject
`git submodule status`
