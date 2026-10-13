#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
revision=6c11357e6d79a89e88cda2fe0e67c99a8923e116
source_dir="$root/.tools/bspc-src"
if [[ ! -d "$source_dir/.git" ]]; then
 git clone https://github.com/bnoordhuis/bspc.git "$source_dir"
 git -C "$source_dir" checkout --detach "$revision"
fi
[[ "$(git -C "$source_dir" rev-parse HEAD)" == "$revision" ]] || { echo 'Unexpected BSPC source revision' >&2; exit 1; }
# Skip a plane already used by an ancestor. Thin adjacent triangle prisms can
# expose that numerical case; the upstream code aborts instead of rejecting it.
for patch in bspc-parent-planes.patch bspc-terrain-partition.patch bspc-reachability-capacity.patch bspc-md4-32bit.patch; do
 if git -C "$source_dir" apply --check "$root/scripts/patches/$patch" 2>/dev/null; then
  git -C "$source_dir" apply "$root/scripts/patches/$patch"
 else
  git -C "$source_dir" apply --reverse --check "$root/scripts/patches/$patch"
 fi
done
make -C "$source_dir" -j"${QCE_BUILD_JOBS:-4}" CFLAGS='-Dstricmp=strcasecmp -DCom_Memcpy=memcpy -DCom_Memset=memset -DMAC_STATIC= -DQDECL= -DLINUX -DBSPC -D_FORTIFY_SOURCE=2 -I. -Ideps -Wall -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -fcommon -O2'
mkdir -p "$root/.tools/bin"
cp "$source_dir/bspc" "$root/.tools/bin/.bspc.$$"
mv -f "$root/.tools/bin/.bspc.$$" "$root/.tools/bin/bspc"
