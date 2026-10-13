#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -ffunction-sections -fdata-sections -I"$root/engine/code/game" -I"$root/engine/code/qcommon" \
 "$root/tests/spawns.c" "$root/engine/code/game/g_client.c" "$root/engine/code/qcommon/q_math.c" "$root/engine/code/qcommon/q_shared.c" \
 -Wl,--gc-sections -lm -o "$root/build/tests/spawns"
"$root/build/tests/spawns"
