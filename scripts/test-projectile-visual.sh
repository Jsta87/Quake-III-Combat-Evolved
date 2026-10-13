#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -ffunction-sections -fdata-sections \
 "$root/tests/projectile-visual.c" "$root/engine/code/cgame/cg_weapons.c" \
 "$root/engine/code/qcommon/q_math.c" "$root/engine/code/qcommon/q_shared.c" \
 -Wl,--gc-sections -lm -o "$root/build/tests/projectile-visual"
"$root/build/tests/projectile-visual"
