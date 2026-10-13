#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -I"$root/engine/code/qcommon" -I"$root/engine/code/game" "$root/tests/camera.c" "$root/engine/code/game/bg_pmove.c" "$root/engine/code/game/bg_slidemove.c" "$root/engine/code/game/bg_misc.c" "$root/engine/code/qcommon/q_math.c" "$root/engine/code/qcommon/q_shared.c" -lm -o "$root/build/tests/camera"
"$root/build/tests/camera"
