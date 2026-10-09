#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -ffunction-sections -fdata-sections \
  -I"$root/engine/code/qcommon" -I"$root/engine/code/game" \
  "$root/tests/heat.c" "$root/engine/code/game/bg_pmove.c" \
  "$root/engine/code/game/bg_slidemove.c" "$root/engine/code/game/g_qce_variant.c" "$root/engine/code/game/bg_misc.c" \
  "$root/engine/code/qcommon/q_math.c" "$root/engine/code/qcommon/q_shared.c" \
  -Wl,--gc-sections -lm -o "$root/build/tests/heat"
"$root/build/tests/heat"
cc -std=c99 -O2 -ffunction-sections -fdata-sections \
 -I"$root/engine/code/qcommon" -I"$root/engine/code/game" \
 "$root/tests/heat-pickup.c" "$root/engine/code/game/g_items.c" "$root/engine/code/game/g_qce_variant.c" "$root/engine/code/game/bg_misc.c" \
 "$root/engine/code/qcommon/q_math.c" "$root/engine/code/qcommon/q_shared.c" \
 -Wl,--gc-sections -lm -o "$root/build/tests/heat-pickup"
"$root/build/tests/heat-pickup"
