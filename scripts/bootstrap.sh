#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
python3 -m venv "$root/.tools"
"$root/.tools/bin/python" -m pip install --disable-pip-version-check cmake==3.31.6 ninja==1.11.1.4
export PATH="$root/.tools/bin:$PATH"
"$root/scripts/build.sh" server
