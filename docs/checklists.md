## Checklists for making releases and updating libraries or fonts

### Releasing a new major/minor/patch update
1. Ensure that dev features are disabled (e.g. collision benchmarks)
1. Commit all changes into Git that need to go into the release
1. Make sure your working tree is clean, i.e. `git status` shows no untracked files or unstaged changes
    - You can use `git stash` to temporarily remove working directory changes that shouldn't be included in the next release
    - After publishing the next release, you can then restore the changes with `git stash pop`
1. Ensure that DZSimulator uses the latest OpenSSL version
1. Increase project version in top-level CMakeLists.txt
1. Write changelogs for GitHub and ingame display
1. In Visual Studio, right-click top-level CMakeLists.txt > Configure DZSimulatorProject
1. Build executable
1. Make sure in-app build timestamp is correctly updated
1. Commit new version tag (and possibly ingame changelog) and push it:

    **CAUTION: The git tag's name must follow strict rules in order for the new release to be detected by by previous DZSimulator versions!**
    - Tag name MUST be of the format `vX.Y.Z` where X, Y and Z are positive integers.
    - When comparing the new tag name to the tag name of all previous releases, one of the following must be true:
        - The new X is greater than the old X
        - Both X values are equal and the new Y is greater than the old Y
        - Both X values are equal, both Y values are equal and the new Z is greater than the old Z
    ```
    git commit -m "Bumped DZSimulator version to X.Y.Z"
    git tag vX.Y.Z
    git push origin
    git push origin --tags
    ```
1. Inside the repo, run:
    ```
    git ls-files --cached --recurse-submodules -z | tar --use-compress-program='xz -8' -cvf ../DZSimulator-vX.Y.Z-Source-code-with-submodules.tar.xz --xform=s:^:DZSimulator/: --null --files-from=-
    ```
    - Your working tree must be clean before running this command to ensure the correct files and their correct version get archived (Note: Untracked files don't get archived)
    - If you're on Windows, you need to run this command in **Git Bash**, which is probably included in your Git installation
    - Command was tested with GNU tar 1.34
    - The `.tar.xz` archive is created in the parent directory
1. Rename executable ("DZSimulator-vX.Y.Z.exe") and source code archive with new version number
1. Make a release on GitHub, write changes and attach source code archive and the executable(s)
1. Test if new GitHub release is detected by previously released DZSimulator versions

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
