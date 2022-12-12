## PLEASE DON'T MODIFY LIBRARIES THAT ARE INCLUDED AS GIT SUBMODULES.

Updating or downgrading the submodule library to a certain commit of the library's official repo with
```
git -C thirdparty/<SUBMODULE_NAME>/ checkout <COMMIT_OR_TAG>
```
is fine. If you do that, don't forget to update the instructions in [BUILDING.md](../BUILDING.md) with the new commit hash.

Why? Someone that only has access to the DZSimulator source code without submodules and without a `.git/` directory (which is what you get when selecting "Download ZIP" or the default "Download Source code" on GitHub's releases page) should be able to add the original unmodified versions of the missing libraries to get a working project. If you need to modify a library, consider adding its source code directly to the repo and not via git submodules.


## Upgrading Magnum and Corrade

Since Magnum is under active development and the last big release was in 2020, we are using Magnum and Corrade libraries from a somewhat random point in their commit history. If you need to upgrade them to a newer version from their GitHub repos, take a close look at the changes since then and test if they break anything! To determine the commit hash a submodule library is currently at, run:
```
git submodule status
```


## Magnum and Corrade usage

Some of Magnum's and Corrade's libraries, plugins, executables or tests depend on third party components. Check their license requirements before using them.

**If components you use require their legal notices to be included when distributed in binary form, add them to the top-level [LICENSES-THIRD-PARTY.txt](../LICENSES-THIRD-PARTY.txt) that can be viewed inside the DZSimulator application!**

Please see:
- [Magnum license](https://doc.magnum.graphics/magnum/index.html#mainpage-license)
- [Corrade license](https://doc.magnum.graphics/corrade/index.html#corrade-mainpage-license)
- [Magnum's third party components](https://doc.magnum.graphics/magnum/credits-third-party.html)
- [Corrade's third party components](https://doc.magnum.graphics/corrade/corrade-credits-third-party.html)
