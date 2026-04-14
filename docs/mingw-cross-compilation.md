# MinGW Cross-Compilation for Windows

Build Windows VST3 plugins from a **Linux** host using the MinGW-w64 toolchain —
no Windows machine or VM required.

---

## Prerequisites

### 1. Install MinGW-w64

```bash
# Debian / Ubuntu
sudo apt-get install mingw-w64

# Arch
sudo pacman -S mingw-w64-gcc

# Fedora / RHEL
sudo dnf install mingw64-gcc-c++
```

Verify the compiler is available:

```bash
x86_64-w64-mingw32-g++ --version
```

### 2. Other tools

```bash
sudo apt-get install cmake ninja-build jq unzip curl
```

---

## Quick Start

### Plugins WITHOUT WebView2

For plugins using the Visage framework or a headless processor, no extra SDK
is needed:

```bash
bash scripts/build-mingw-cross.sh ColourBrickwallScream
```

Artifacts land in `dist/ColourBrickwallScream-windows-vst3/`.

### Plugins WITH WebView2

JUCE's WebView2 support requires the Microsoft WebView2 NuGet SDK headers
and import library.

**Step 1 — Download the SDK once:**

```bash
bash scripts/fetch-webview2-sdk.sh
# SDK unpacked to: webview2_sdk/
```

**Step 2 — Build:**

```bash
bash scripts/build-mingw-cross.sh ColourBrickwallScream \
    --webview2-sdk webview2_sdk
```

The SDK path is also auto-detected if `webview2_sdk/` exists in the repo root.

---

## All Options

```
bash scripts/build-mingw-cross.sh <PluginName> [options]

  --webview2-sdk DIR   Path to extracted WebView2 NuGet SDK root
  --mingw-prefix PFX   Toolchain prefix (default: x86_64-w64-mingw32)
  --jobs N             Parallel build jobs (default: nproc)
  --no-install         Do not copy .vst3 to dist/
  --skip-tests         Skip prerequisite phase check
  --release            Release build (default)
  --debug              Debug build
```

---

## Toolchain File

The toolchain is at `toolchains/mingw-w64-x86_64.cmake`.

You can use it directly with CMake if you prefer manual invocation:

```bash
cmake -S . -B build-mingw \
    -DCMAKE_TOOLCHAIN_FILE=toolchains/mingw-w64-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DAPC_WEBVIEW2_SDK_DIR=/path/to/webview2_sdk

cmake --build build-mingw --target ColourBrickwallScream_VST3 -- -j$(nproc)
```

### Toolchain CMake cache variables

| Variable | Default | Description |
|---|---|---|
| `APC_MINGW_PREFIX` | `x86_64-w64-mingw32` | Toolchain binary prefix |
| `APC_MINGW_STATIC_RUNTIME` | `ON` | Link libgcc/libstdc++ statically |
| `APC_WEBVIEW2_SDK_DIR` | auto-detected | WebView2 NuGet SDK root |

---

## What Gets Built

| Format | Cross-compiled? | Notes |
|---|---|---|
| VST3 | **Yes** | Primary target |
| AU | No | macOS-only, always skipped |
| LV2 | No | Linux-only format |
| Standalone | Possible | Requires extra Windows runtime DLLs |

---

## Installing the VST3 on Windows

Copy the `.vst3` bundle to one of these folders on the Windows machine:

```
C:\Program Files\Common Files\VST3\     ← system-wide
%APPDATA%\VST3\                         ← current user only
```

If the plugin uses WebView2, the target machine must have the
**WebView2 Runtime** installed (comes pre-installed on Windows 10/11).

---

## Known Limitations

### 1. WebView2 static linking

`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` embeds the WebView2 loader stub
but still requires `WebView2Loader.dll` or the runtime on the end-user machine.
The `.lib` in the NuGet package is a Windows COFF import library — MinGW links
against it normally via its PE import-lib support.

### 2. LTO with MinGW

`juce::juce_recommended_lto_flags` may fail with older MinGW toolchains.
If you hit LTO linker errors, add `-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF`
to the configure step.

### 3. POSIX vs Win32 threads

Ubuntu's default `x86_64-w64-mingw32` package ships the **POSIX** thread model
variant which is compatible with JUCE's threading primitives. The Win32 thread
variant may cause `std::thread` link errors.

Check which variant is active:

```bash
x86_64-w64-mingw32-g++ -v 2>&1 | grep "Thread model"
# should print: Thread model: posix
```

If it prints `win32`, switch to the POSIX variant:

```bash
sudo update-alternatives --config x86_64-w64-mingw32-g++
# choose the posix entry
```

### 4. Resource files (.rc)

JUCE generates `.rc` resource files for version info. MinGW's `windres` handles
these automatically when `CMAKE_RC_COMPILER` is set (the toolchain file sets it).

---

## Troubleshooting

### `gtk/gtk.h: No such file or directory`

This is a Linux-only GTK header leaking into the Windows build.
Ensure the toolchain file is loaded (`-DCMAKE_TOOLCHAIN_FILE=...`) and that
`CMAKE_FIND_ROOT_PATH_MODE_INCLUDE` is `ONLY`.

### `undefined reference to '__imp_CoCreateInstance'`

Add `-lole32` to `CMAKE_EXE_LINKER_FLAGS` or ensure the plugin links
`juce::juce_audio_plugin_client` which pulls in the Windows COM libraries.

### `undefined reference to 'WebView2GetEnvironment'`

The WebView2 import lib is not linked. Verify `APC_WEBVIEW2_SDK_DIR` points to
the extracted NuGet package and that `build/native/x64/WebView2LoaderStatic.lib`
exists inside it.

### CMake can't find the toolchain compilers

```bash
export APC_MINGW_PREFIX=x86_64-w64-mingw32
# or pass on command line:
bash scripts/build-mingw-cross.sh MyPlugin --mingw-prefix x86_64-w64-mingw32
```
