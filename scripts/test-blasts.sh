#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -ffunction-sections -fdata-sections \
 -I"$root/engine/code/qcommon" -I"$root/engine/code/game" \
 "$root/tests/blasts.c" "$root/engine/code/game/g_combat.c" \
 "$root/engine/code/game/g_qce_variant.c" "$root/engine/code/game/bg_misc.c" "$root/engine/code/qcommon/q_math.c" "$root/engine/code/qcommon/q_shared.c" \
 -Wl,--gc-sections -lm -o "$root/build/tests/blasts"
"$root/build/tests/blasts"
