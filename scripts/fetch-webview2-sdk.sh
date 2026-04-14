#!/usr/bin/env bash
# ============================================================
# Fetch & unpack the Microsoft WebView2 NuGet package
# into <repo-root>/webview2_sdk/ for MinGW cross-compilation.
#
# Directory layout produced (what JUCE's FindWebView2.cmake expects):
#
#   webview2_sdk/                          ← JUCE_WEBVIEW2_PACKAGE_LOCATION
#     Microsoft.Web.WebView2.X.Y.Z.W/      ← globbed by FindWebView2.cmake
#       build/
#         native/
#           include/
#             WebView2.h
#           x64/
#             WebView2LoaderStatic.lib
#
# Usage:
#   bash scripts/fetch-webview2-sdk.sh [--version X.Y.Z.W]
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_PATH="$(cd "$SCRIPT_DIR/.." && pwd)"

# JUCE_WEBVIEW2_PACKAGE_LOCATION — parent dir, NOT the versioned package dir
SDK_PARENT="$ROOT_PATH/webview2_sdk"

# Must be >= JUCE's required minimum (1.0.3485.44 as of JUCE 8.x).
# Override with --version if a newer release is available.
WEBVIEW2_VERSION="1.0.3485.44"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) WEBVIEW2_VERSION="$2"; shift 2 ;;
        *) echo "Unknown arg: $1" >&2; exit 1 ;;
    esac
done

# NuGet package dir name must match what FindWebView2.cmake globs for
PACKAGE_DIR_NAME="Microsoft.Web.WebView2.${WEBVIEW2_VERSION}"
PACKAGE_DIR="${SDK_PARENT}/${PACKAGE_DIR_NAME}"
NUPKG_URL="https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${WEBVIEW2_VERSION}"
NUPKG_FILE="/tmp/webview2_${WEBVIEW2_VERSION}.nupkg"

echo "Fetching WebView2 SDK v${WEBVIEW2_VERSION}..."

if ! command -v curl &>/dev/null && ! command -v wget &>/dev/null; then
    echo "ERROR: curl or wget is required." >&2
    exit 1
fi

if ! command -v unzip &>/dev/null; then
    echo "ERROR: unzip is required.  sudo apt-get install unzip" >&2
    exit 1
fi

# Download
if command -v curl &>/dev/null; then
    curl -L --progress-bar -o "$NUPKG_FILE" "$NUPKG_URL"
else
    wget -q --show-progress -O "$NUPKG_FILE" "$NUPKG_URL"
fi

# Extract into versioned subdirectory so FindWebView2.cmake can glob it
echo "Unpacking to $PACKAGE_DIR ..."
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"
unzip -q "$NUPKG_FILE" -d "$PACKAGE_DIR"
rm -f "$NUPKG_FILE"

# Verify expected files
HEADER="$PACKAGE_DIR/build/native/include/WebView2.h"
LIB="$PACKAGE_DIR/build/native/x64/WebView2LoaderStatic.lib"

echo ""
if [[ -f "$HEADER" ]]; then
    echo "  Header  : $HEADER  ✓"
else
    echo "  WARNING : Header not found: $HEADER" >&2
fi

if [[ -f "$LIB" ]]; then
    echo "  Library : $LIB  ✓"
else
    echo "  WARNING : Library not found: $LIB" >&2
fi

echo ""
echo "WebView2 SDK ready."
echo ""
echo "  JUCE_WEBVIEW2_PACKAGE_LOCATION = $SDK_PARENT"
echo ""
echo "The build script picks this up automatically."
echo "Or pass explicitly:"
echo "  bash scripts/build-mingw-cross.sh <Plugin> --webview2-sdk $SDK_PARENT"
