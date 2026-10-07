#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
data="${QCE_DATA_DIR:-$root/assets}"
if [[ ! -f "$data/baseq3/pak0.pk3" ]]; then
  echo "Place your Quake III baseq3/*.pk3 files in $data/baseq3 (or set QCE_DATA_DIR)." >&2
  exit 2
fi
mkdir -p "$root/runtime/baseq3"
cp "$root"/build/server/RelWithDebInfo/baseq3/*.so "$root/runtime/baseq3/"
exec "$root/build/server/RelWithDebInfo/ioq3ded" +set fs_basepath "$data" \
  +set fs_homepath "$root/runtime" +set fs_game baseq3 \
  +set sv_pure 0 +set vm_game 0 \
  +set dedicated 1 +set sv_hostname "Quake III Combat Evolved - development" \
  +set g_gametype 0 +map "${QCE_MAP:-q3dm1}" "$@"
