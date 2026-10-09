#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -I"$root/engine/code/qcommon" "$root/tests/halo-particles.c" -lm -o "$root/build/tests/halo-particles"
"$root/build/tests/halo-particles"
