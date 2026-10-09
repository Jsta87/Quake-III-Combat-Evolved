#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -ffunction-sections -fdata-sections \
 -I"$root/engine/code/qcommon" \
 "$root/tests/network-state.c" "$root/engine/code/qcommon/msg.c" "$root/engine/code/qcommon/huffman.c" "$root/engine/code/qcommon/net_chan.c" \
 "$root/engine/code/qcommon/q_math.c" "$root/engine/code/qcommon/q_shared.c" \
 -Wl,--gc-sections -lm -o "$root/build/tests/network-state"
"$root/build/tests/network-state"
