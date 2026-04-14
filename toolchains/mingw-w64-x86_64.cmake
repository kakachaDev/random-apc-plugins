# ============================================================
# MinGW-w64 Cross-Compilation Toolchain for JUCE 8 Plugins
# Host:   Linux (x86_64)
# Target: Windows (x86_64)
#
# Prerequisites:
#   sudo apt-get install mingw-w64
#
# Usage:
#   cmake -S . -B build-mingw \
#     -DCMAKE_TOOLCHAIN_FILE=toolchains/mingw-w64-x86_64.cmake \
#     [options]
#
# Or via helper script:
#   bash scripts/build-mingw-cross.sh <PluginName>
# ============================================================

# --- Target System ---
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# --- Toolchain Binaries ---
# Override with APC_MINGW_PREFIX env var or cmake cache var if your
# distro uses a different prefix (e.g. x86_64-w64-mingw32 vs
# x86_64-mingw32).
if(DEFINED ENV{APC_MINGW_PREFIX})
    set(_MINGW_PREFIX "$ENV{APC_MINGW_PREFIX}")
elseif(DEFINED APC_MINGW_PREFIX)
    set(_MINGW_PREFIX "${APC_MINGW_PREFIX}")
else()
    set(_MINGW_PREFIX "x86_64-w64-mingw32")
endif()

find_program(CMAKE_C_COMPILER   NAMES "${_MINGW_PREFIX}-gcc"   REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES "${_MINGW_PREFIX}-g++"   REQUIRED)
find_program(CMAKE_RC_COMPILER  NAMES "${_MINGW_PREFIX}-windres")
find_program(CMAKE_AR           NAMES "${_MINGW_PREFIX}-ar"    REQUIRED)
find_program(CMAKE_RANLIB       NAMES "${_MINGW_PREFIX}-ranlib")
find_program(CMAKE_STRIP        NAMES "${_MINGW_PREFIX}-strip")
find_program(CMAKE_DLLTOOL      NAMES "${_MINGW_PREFIX}-dlltool")
find_program(CMAKE_OBJCOPY      NAMES "${_MINGW_PREFIX}-objcopy")

# --- Sysroot / Find-Root ---
# Tell CMake where Windows headers and libraries live so it doesn't
# accidentally pick up native Linux ones.
if(EXISTS "/usr/${_MINGW_PREFIX}")
    set(CMAKE_FIND_ROOT_PATH "/usr/${_MINGW_PREFIX}")
elseif(EXISTS "/usr/lib/gcc/${_MINGW_PREFIX}")
    set(CMAKE_FIND_ROOT_PATH "/usr/lib/gcc/${_MINGW_PREFIX}")
endif()

# Search libraries/includes in the sysroot; programs (tools) on the host.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# --- Shared vs Static CRT ---
# -static-libgcc / -static-libstdc++ avoids shipping the MinGW DLLs
# alongside the plugin. Use CACHE vars so individual targets can override.
option(APC_MINGW_STATIC_RUNTIME
    "Link libgcc/libstdc++ statically (no runtime DLL dependency)" ON)

if(APC_MINGW_STATIC_RUNTIME)
    set(_STATIC_FLAGS "-static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS_INIT    "${_STATIC_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_STATIC_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_STATIC_FLAGS}")
endif()

# --- WebView2 SDK ---
# Pass -DJUCE_WEBVIEW2_PACKAGE_LOCATION=<path> at configure time (pointing to
# the directory that CONTAINS the Microsoft.Web.WebView2.X.Y.Z folder).
# JUCE's FindWebView2.cmake picks it up automatically from that variable.
# The build-mingw-cross.sh script sets this for you.

# --- Useful MinGW Compile Flags ---
# Silence noisy MinGW-specific warnings that don't affect correctness.
set(CMAKE_CXX_FLAGS_INIT
    "-D_WIN32_WINNT=0x0A00 \
     -DWINVER=0x0A00 \
     -DNOMINMAX \
     -DWIN32_LEAN_AND_MEAN \
     -Wno-error=cast-function-type")

# Windows 10 target lets JUCE unlock modern API surface.

# --- Ensure CMake treats this as a Windows cross-build ---
# Without this JUCE's platform detection in CMakeLists.txt (WIN32 check)
# would be FALSE on a Linux host.
set(WIN32 TRUE)
set(MINGW TRUE)
