#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -ffunction-sections -fdata-sections \
 -I"$root/engine/code/qcommon" -I"$root/engine/code/game" \
 "$root/tests/entities.c" "$root/engine/code/game/g_utils.c" \
 -Wl,--gc-sections -lm -o "$root/build/tests/entities"
"$root/build/tests/entities"
