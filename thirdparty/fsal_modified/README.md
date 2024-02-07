The "fsal" directory contains the "File System Abstraction Layer" library that is used to load CS:GO's game assets from the game directory, CS:GO's VPK archives or directly from memory.

It is from https://github.com/podgorskiy/fsal commit 43a10da (May 5, 2020) and was modified specifically for DZSimulator.

Modifications made for DZSimulator:
- Increased minimum CMake version from `3.1` to `3.5`
- ZipArchive functionality has been stripped out together with the dependency libraries and the example and test build options
- Added MemRefFileReadOnly, a read-only modification of MemRefFile, usable with const pointers
- Added LMemRefFileReadOnly, a lockable version of MemRefFileReadOnly that is required when used with SubFile.
- A number of changes in the VPK archive code
  - Fixed memory leaks
  - Removed undefined behaviour
  - Added fail checks
  - Added file filtering
  - And more
- Added return statements indicating failure to some functions that would have thrown exceptions or failed asserts otherwise.
- Changed some lines of code to allow compilation in C++20

All changes to the original source code are marked with a "DZSIM_MOD" comment.
