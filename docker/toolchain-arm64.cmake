# CMake cross-compilation toolchain: amd64 host → arm64 target (CM4, Pi 5).
#
# Relies on Debian multiarch — the arm64 Qt and its dependencies are installed
# into /usr/lib/aarch64-linux-gnu alongside the host's amd64 libraries, so
# there is no separate sysroot. CMAKE_FIND_ROOT_PATH_MODE_* is what keeps
# find_package() from picking up the host's amd64 Qt by mistake.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/lib/aarch64-linux-gnu /usr)

# Executables (moc, rcc, cmake itself) must come from the host; libraries and
# headers must come from the arm64 tree.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Multiarch pkg-config: point at the arm64 .pc files only.
set(ENV{PKG_CONFIG_LIBDIR} /usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig)
set(ENV{PKG_CONFIG_PATH} "")
