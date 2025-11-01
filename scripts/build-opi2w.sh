#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJ_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$PROJ_DIR/build"

# Load env (sets SYSROOT etc.) if exists
if [ -f "/home/kiwlee/cross/scripts/env-opi-zero-2w.sh" ]; then
  # shellcheck disable=SC1091
  source "/home/kiwlee/cross/scripts/env-opi-zero-2w.sh"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ "${1:-}" = "clean" ]; then
  rm -rf "$BUILD_DIR"/*
fi

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$PROJ_DIR/cmake/toolchain-opi2w.cmake"

make -j"$(nproc)"

echo "Build done: $BUILD_DIR/bin/lvglsim"

