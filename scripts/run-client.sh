#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
data="${QCE_DATA_DIR:-$root/assets}"
[[ -f "$data/baseq3/pak0.pk3" ]] || { echo "Quake III assets missing" >&2; exit 2; }
mkdir -p "$root/runtime-client/baseq3"
cp "$root"/build/client/RelWithDebInfo/baseq3/*.so "$root/runtime-client/baseq3/"
cp "$root/config/qce-controls.cfg" "$root/runtime-client/baseq3/qce-controls.cfg"
# Prefer the machine's SDL runtime, which supports desktop OpenGL. The locally
# built development library can lack desktop backends on minimal cloud images.
if [[ -f /usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0 ]]; then
 export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
export MESA_SHADER_CACHE_DIR="$root/runtime-client/mesa-cache"
exec "$root/build/client/RelWithDebInfo/ioquake3" \
 +set fs_basepath "$data" +set fs_homepath "$root/runtime-client" \
 +set sv_pure 0 +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
 +exec qce-controls.cfg +set g_qceCombat 1 +set g_qceMovement 1 +set sv_master1 "" +map "${QCE_MAP:-q3dm1}" "$@"
