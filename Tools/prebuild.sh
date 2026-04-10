#!/bin/bash
# Master prebuild script for Linux.
# Runs all prebuild steps needed before building.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$SCRIPT_DIR/.."

echo "============================================"
echo " Polyphase Prebuild (Linux)"
echo "============================================"
echo ""

# --- libgit2 ---
echo "[1/2] Building libgit2..."
"$SCRIPT_DIR/prebuild_libgit2.sh"
echo ""

# --- Shaders ---
echo "[2/2] Compiling shaders..."
pushd "$REPO_ROOT/Engine/Shaders/GLSL" > /dev/null
bash compile.sh
popd > /dev/null
echo ""

echo "============================================"
echo " Prebuild complete."
echo "============================================"
