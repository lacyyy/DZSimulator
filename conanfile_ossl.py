from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

class OnlyGiveMeOpenSSLAndNothingMore_Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        # Specify which OpenSSL version we want installed.
        # Recipe revision is also specified in the hopes that it improves
        # build reproducibility.
        self.requires("openssl/3.0.13#7f175a90f2e6b24d5b9ce6b952794ffb")
        # Note: At the time of writing (2024-02-10), OpenSSL version 3.0 was
        #       chosen instead of 3.1 and 3.2 because it resulted in a release
        #       executable size 721 KiB and 1027 KiB smaller, repectively.

    def generate(self):
        tc = CMakeToolchain(self)

        # By the way, you can specify a desired build system generator like this.
        # Not needed here though.
        #tc = CMakeToolchain(self, generator="Ninja")

        #########################################################################
        
        # When we want to install OpenSSL for DZSimulator using the
        # 'conan install' command, conan generates a CMake toolchain file
        # ('conan_toolchain.cmake') that contains information about where the
        # installed OpenSSL version is located.

        # Unfortunately, it also contains other irrelevant information about how
        # conan would like to build this project, such as cpp version, compiler,
        # platform, flags, etc.
        # All we want is to know where OpenSSL is. These other settings can
        # interfere with DZSimulator's separate build setup, therefore we need to
        # remove these other settings from conan's generated toolchain file.

        # From all the blocks (settings categories) in conan's toolchain file,
        # select a few and remove the rest.
        # - Block 'try_compile': Not sure of this is needed.
        # - Block 'find_paths':  Definitely needed, points to OpenSSL installation.
        # - Block 'pkg_config':  Not sure of this is needed.
        tc.blocks.select("try_compile", "find_paths", "pkg_config")

        # To be honest, I wonder if this OpenSSL installation + manipulation of
        # conan's toolchain file + inclusion of it in DZSimulator's own CMake
        # build setup is reliable.
        # It seems like an approach not intended by Conan.

        tc.generate()
