The "fsal" directory contains the "File System Abstraction Layer" library that is used to load CS:GO's game assets from the game directory and CS:GO's VPK archives.

It is from https://github.com/podgorskiy/fsal commit 43a10da (May 5, 2020) and was modified specifically for DZSimulator.

Namely, The ZipArchive functionality has been stripped out together with the dependency libraries and the example and test build options. A number of changes were also made in the VPK archive code: Fixed memory leaks, removed undefined behaviour, added fail checks, added file filtering and more.

Return statements indicating failure were added to some functions that would have thrown exceptions or failed asserts otherwise.

Some lines of code were changed to allow compilation in C++20.

All changes to the original source code are marked with a "DZSIM_MOD" comment.
