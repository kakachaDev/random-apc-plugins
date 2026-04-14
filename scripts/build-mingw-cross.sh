#!/usr/bin/env bash
# ============================================================
# APC MinGW Cross-Compilation Builder
# Host:   Linux (x86_64)
# Target: Windows VST3 (.vst3)
#
# Usage:
#   bash scripts/build-mingw-cross.sh <PluginName> [options]
#
# Options:
#   --no-install        Skip copying .vst3 to dist/
#   --skip-tests        Skip prerequisite phase check
#   --jobs N            Parallel build jobs (default: auto)
#   --webview2-sdk DIR  Path to extracted WebView2 NuGet SDK
#   --mingw-prefix PFX  Toolchain prefix (default: x86_64-w64-mingw32)
#   --release / --debug Build configuration (default: Release)
#   --add-swap SIZE     Create a temporary swap file before building
#                       SIZE is in GiB, e.g. --add-swap 4  (requires sudo)
# ============================================================

set -euo pipefail

# ---- Parse arguments ----
PLUGIN_NAME=""
NO_INSTALL=false
SKIP_TESTS=false
ADD_SWAP_GIB=0   # 0 = disabled
_AVAIL_MB="$(awk '/MemAvailable/ {print int($2/1024)}' /proc/meminfo 2>/dev/null || echo 4096)"
_MAX_SAFE_JOBS=$(( _AVAIL_MB / 1500 ))
(( _MAX_SAFE_JOBS < 1 )) && _MAX_SAFE_JOBS=1
JOBS="$(( $(nproc 2>/dev/null || echo 4) < _MAX_SAFE_JOBS ? $(nproc 2>/dev/null || echo 4) : _MAX_SAFE_JOBS ))"
WEBVIEW2_SDK_DIR=""
MINGW_PREFIX=""
BUILD_CONFIG="Release"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-install)     NO_INSTALL=true; shift ;;
        --skip-tests)     SKIP_TESTS=true; shift ;;
        --jobs)           JOBS="$2"; shift 2 ;;
        --webview2-sdk)   WEBVIEW2_SDK_DIR="$2"; shift 2 ;;
        --mingw-prefix)   MINGW_PREFIX="$2"; shift 2 ;;
        --release)        BUILD_CONFIG="Release"; shift ;;
        --debug)          BUILD_CONFIG="Debug"; shift ;;
        --add-swap)       ADD_SWAP_GIB="$2"; shift 2 ;;
        -*)               echo "Unknown option: $1" >&2; exit 1 ;;
        *)                PLUGIN_NAME="$1"; shift ;;
    esac
done

if [[ -z "$PLUGIN_NAME" ]]; then
    echo "Usage: $0 <PluginName> [options]" >&2
    echo "  --webview2-sdk DIR  Path to WebView2 NuGet SDK root" >&2
    echo "  --mingw-prefix PFX  Toolchain prefix (x86_64-w64-mingw32)" >&2
    echo "  --jobs N            Parallel jobs" >&2
    echo "  --no-install        Skip dist/ output" >&2
    echo "  --add-swap N        Create N GiB swap file before building (needs sudo)" >&2
    exit 1
fi

# ---- Paths ----
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_PATH="$(cd "$SCRIPT_DIR/.." && pwd)"
TOOLCHAIN_FILE="$ROOT_PATH/toolchains/mingw-w64-x86_64.cmake"
BUILD_DIR="$ROOT_PATH/build-mingw-${PLUGIN_NAME}"
DIST_DIR="$ROOT_PATH/dist/${PLUGIN_NAME}-windows-vst3"
PLUGIN_DIR="$ROOT_PATH/plugins/$PLUGIN_NAME"
STATUS_JSON="$PLUGIN_DIR/status.json"

# ---- Swap management ----
# juceaide compiles juce_gui_basics.cpp which peaks at ~1.8 GB with native GCC.
# This happens inside CMake's configure step (ExternalProject, separate process),
# so CMAKE_BUILD_PARALLEL_LEVEL=1 is not enough on low-memory servers.
# Solution: add swap before building, remove it after.
_SWAP_FILE="/tmp/apc-build-swap-$$"
_SWAP_ACTIVE=false

_ensure_swap() {
    local gib="$1"
    local bytes=$(( gib * 1024 * 1024 * 1024 ))
    echo "Creating ${gib} GiB swap file at $_SWAP_FILE ..."
    if command -v fallocate &>/dev/null; then
        sudo fallocate -l "${gib}G" "$_SWAP_FILE"
    else
        sudo dd if=/dev/zero of="$_SWAP_FILE" bs=1M count=$(( gib * 1024 )) status=none
    fi
    sudo chmod 600 "$_SWAP_FILE"
    sudo mkswap -q "$_SWAP_FILE"
    sudo swapon "$_SWAP_FILE"
    _SWAP_ACTIVE=true
    echo "Swap active: ${gib} GiB"
}

_remove_swap() {
    if $_SWAP_ACTIVE && [[ -f "$_SWAP_FILE" ]]; then
        sudo swapoff "$_SWAP_FILE" 2>/dev/null || true
        sudo rm -f "$_SWAP_FILE"
        _SWAP_ACTIVE=false
        echo "Swap removed."
    fi
}
trap '_remove_swap' EXIT

if (( ADD_SWAP_GIB > 0 )); then
    _ensure_swap "$ADD_SWAP_GIB"
elif (( _AVAIL_MB < 3000 )); then
    echo ""
    echo "⚠️  WARNING: Only ${_AVAIL_MB} MB RAM available."
    echo "   JUCE's juceaide build (juce_gui_basics.cpp) peaks at ~1.8 GB."
    echo "   The build may be OOM-killed."
    echo ""
    echo "   Fix options:"
    echo "     1. Add swap automatically:"
    echo "        bash $0 $PLUGIN_NAME --add-swap 4   # needs sudo"
    echo ""
    echo "     2. Add swap manually:"
    echo "        sudo fallocate -l 4G /swapfile"
    echo "        sudo chmod 600 /swapfile && sudo mkswap /swapfile && sudo swapon /swapfile"
    echo "        # then re-run this script"
    echo ""
    echo "   Continuing anyway..."
    echo ""
fi

# ---- Dependency check ----
echo "--- APC MinGW Cross-Builder: $PLUGIN_NAME ---"
echo "Host: $(uname -m)-linux"
echo "Target: Windows x86_64 (VST3)"
echo ""

_PREFIX="${MINGW_PREFIX:-x86_64-w64-mingw32}"

if ! command -v "${_PREFIX}-g++" &>/dev/null; then
    echo "ERROR: MinGW toolchain not found: ${_PREFIX}-g++" >&2
    echo "" >&2
    echo "Install with:" >&2
    echo "  sudo apt-get install mingw-w64" >&2
    echo "" >&2
    echo "Then re-run this script." >&2
    exit 1
fi

GCC_VER=$("${_PREFIX}-g++" --version | head -1)
echo "Toolchain: $GCC_VER"
echo "RAM available: ${_AVAIL_MB} MB  →  using ${JOBS} parallel job(s) for main build"

if ! command -v cmake &>/dev/null; then
    echo "ERROR: cmake not found in PATH" >&2
    exit 1
fi

# ---- Plugin existence check ----
if [[ ! -d "$PLUGIN_DIR" ]]; then
    echo "ERROR: Plugin directory not found: $PLUGIN_DIR" >&2
    exit 1
fi

if [[ ! -f "$PLUGIN_DIR/CMakeLists.txt" ]]; then
    echo "ERROR: No CMakeLists.txt in $PLUGIN_DIR" >&2
    exit 1
fi

# ---- Prerequisite phase check ----
if [[ -f "$STATUS_JSON" ]] && command -v jq &>/dev/null && ! $SKIP_TESTS; then
    current_phase="$(jq -r '.current_phase // ""' "$STATUS_JSON" 2>/dev/null || echo "")"
    if [[ "$current_phase" != "code" && "$current_phase" != "ship" && "$current_phase" != "complete" ]]; then
        echo "WARNING: Plugin phase is '$current_phase' — implementation may be incomplete." >&2
        echo "         Use --skip-tests to suppress this warning." >&2
    fi
fi

# ---- Visage detection ----
USE_VISAGE=false
if [[ -f "$STATUS_JSON" ]] && command -v jq &>/dev/null; then
    fw="$(jq -r '.ui_framework // "pending"' "$STATUS_JSON" 2>/dev/null || echo "pending")"
    [[ "$fw" == "visage" ]] && USE_VISAGE=true
fi

# ---- WebView2 SDK: locate JUCE_WEBVIEW2_PACKAGE_LOCATION ----
# FindWebView2.cmake globs for Microsoft.Web.WebView2.* inside this dir.
# So this must be the PARENT of the versioned package folder, not the
# package folder itself.
WEBVIEW2_FLAG=""
_resolve_webview2() {
    local dir="$1"
    # Accept either the parent dir (webview2_sdk/) or the versioned package dir
    if ls "$dir"/Microsoft.Web.WebView2.* &>/dev/null 2>&1; then
        # dir IS the parent — correct
        echo "$dir"
    elif [[ "$(basename "$dir")" == Microsoft.Web.WebView2.* ]]; then
        # dir is the versioned package dir — use its parent
        echo "$(dirname "$dir")"
    else
        echo ""
    fi
}

if [[ -n "$WEBVIEW2_SDK_DIR" ]]; then
    if [[ ! -d "$WEBVIEW2_SDK_DIR" ]]; then
        echo "ERROR: WebView2 SDK not found at: $WEBVIEW2_SDK_DIR" >&2
        exit 1
    fi
    _loc="$(_resolve_webview2 "$(realpath "$WEBVIEW2_SDK_DIR")")"
    if [[ -z "$_loc" ]]; then
        echo "ERROR: $WEBVIEW2_SDK_DIR does not contain a Microsoft.Web.WebView2.* directory." >&2
        echo "       Run: bash scripts/fetch-webview2-sdk.sh" >&2
        exit 1
    fi
    WEBVIEW2_FLAG="-DJUCE_WEBVIEW2_PACKAGE_LOCATION=${_loc}"
    echo "WebView2 SDK: ${_loc}"
else
    # Auto-detect from repo root
    if [[ -d "$ROOT_PATH/webview2_sdk" ]]; then
        _loc="$(_resolve_webview2 "$ROOT_PATH/webview2_sdk")"
        if [[ -n "$_loc" ]]; then
            WEBVIEW2_FLAG="-DJUCE_WEBVIEW2_PACKAGE_LOCATION=${_loc}"
            echo "WebView2 SDK: ${_loc} (auto-detected)"
        else
            echo "WebView2 SDK: webview2_sdk/ found but missing Microsoft.Web.WebView2.* subdirectory."
            echo "              Re-run: bash scripts/fetch-webview2-sdk.sh"
        fi
    else
        echo "WebView2 SDK: not found — run: bash scripts/fetch-webview2-sdk.sh"
    fi
fi

# ---- Assemble cmake flags ----
VISAGE_FLAG=""
$USE_VISAGE && VISAGE_FLAG="-DAPC_ENABLE_VISAGE:BOOL=ON"

MINGW_PREFIX_FLAG=""
[[ -n "$MINGW_PREFIX" ]] && MINGW_PREFIX_FLAG="-DAPC_MINGW_PREFIX=${MINGW_PREFIX}"

# ---- 1. Configure ----
echo ""
echo "[1/3] Configuring..."
# CMAKE_BUILD_PARALLEL_LEVEL=1 limits the internal juceaide build to a single
# job during configure. juceaide compiles juce_gui_basics.cpp which peaks at
# ~1.5 GB; running multiple copies in parallel OOM-kills cc1plus on ≤4 GB hosts.
CMAKE_BUILD_PARALLEL_LEVEL=1 \
cmake -S "$ROOT_PATH" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE="$BUILD_CONFIG" \
    --fresh \
    $VISAGE_FLAG \
    $WEBVIEW2_FLAG \
    $MINGW_PREFIX_FLAG \
    2>&1 | tee "$BUILD_DIR/../_cmake-config-mingw.log" || {
    echo "" >&2
    echo "ERROR: CMake configuration failed." >&2
    echo "Log saved to: $ROOT_PATH/_cmake-config-mingw.log" >&2
    exit 1
}

# ---- 2. Build VST3 ----
echo ""
echo "[2/3] Building VST3 (${BUILD_CONFIG}) with ${JOBS} jobs..."
cmake --build "$BUILD_DIR" \
    --config "$BUILD_CONFIG" \
    --target "${PLUGIN_NAME}_VST3" \
    -- -j"$JOBS" \
    2>&1 | tee "$BUILD_DIR/../_build-mingw.log" || {
    echo "" >&2
    echo "ERROR: VST3 build failed." >&2
    echo "Log saved to: $ROOT_PATH/_build-mingw.log" >&2
    exit 1
}

# ---- 3. Collect & install ----
echo ""
echo "[3/3] Collecting artifacts..."

VST3_BUNDLE="$(find "$BUILD_DIR" -name "${PLUGIN_NAME}.vst3" -type d 2>/dev/null | head -1 || true)"

if [[ -z "$VST3_BUNDLE" ]]; then
    echo "WARNING: .vst3 bundle not found after build." >&2
    echo "         Searching for any .vst3 under $BUILD_DIR:" >&2
    find "$BUILD_DIR" -name "*.vst3" 2>/dev/null || true
else
    echo "Found: $VST3_BUNDLE"

    if ! $NO_INSTALL; then
        mkdir -p "$DIST_DIR"
        TARGET="$DIST_DIR/${PLUGIN_NAME}.vst3"
        rm -rf "$TARGET"
        cp -R "$VST3_BUNDLE" "$TARGET"
        echo ""
        echo "INSTALLED to: $TARGET"
        echo ""
        echo "Copy to Windows VST3 folder:"
        echo "  C:\\Program Files\\Common Files\\VST3\\"
        echo "    or"
        echo "  %APPDATA%\\VST3\\"
    fi
fi

# ---- Done ----
echo ""
echo "==========================="
echo " MinGW cross-build: DONE"
echo "==========================="
echo "Plugin:   $PLUGIN_NAME"
echo "Config:   $BUILD_CONFIG"
echo "Artifacts: $DIST_DIR"
