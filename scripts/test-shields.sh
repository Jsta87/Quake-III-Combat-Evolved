#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -ffunction-sections -fdata-sections \
 -I"$root/engine/code/qcommon" -I"$root/engine/code/game" \
 "$root/engine/code/game/g_qce_variant.c" "$root/engine/code/game/bg_misc.c" "$root/tests/shields.c" "$root/engine/code/game/g_combat.c" \
 -Wl,--gc-sections -lm -o "$root/build/tests/shields"
"$root/build/tests/shields"
