# Fresh Linux installation

Commands below run in a terminal. The package-install command is for Ubuntu/Debian;
on other distributions install equivalent Git, C/C++ build tools, Python development
headers/venv, SDL2 development files and pkg-config. Run the game on a graphical desktop.

On CachyOS/Arch, replace the two `apt` commands below with
`sudo pacman -Syu` and
`sudo pacman -S --needed git base-devel python python-pip sdl2 pkgconf`.

```sh
sudo apt update
sudo apt install -y git build-essential python3 python3-dev python3-venv libsdl2-dev pkg-config
git clone --branch work https://github.com/Jsta87/Quake-III-Combat-Evolved.git
cd Quake-III-Combat-Evolved
mkdir -p assets/baseq3 assets/halo
```

Before continuing, copy your owned Quake III `pak0.pk3` through `pak8.pk3` into
`assets/baseq3/`, and the original **Xbox Halo CE build-2276** `bloodgulch.map`
into `assets/halo/`. PC/MCC map caches are a different format. `ui.map` can also be
kept in `assets/halo/`, but this gameplay build does not need it. Game data and
converted assets are not downloaded by cloning this source repository.

If you still have your old PC's complete `assets` folder, copying that folder
preserves your source data. Reconvert Blood Gulch below to get the bot roam goals
and closed navigation volume, then rebuild its AAS.

Build the engine and generate the local game packages:

```sh
./scripts/bootstrap.sh
./scripts/bootstrap-client.sh
export PATH="$PWD/.tools/bin:$PATH"

python3 scripts/convert-halo-assets.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch --pk3 assets/baseq3/zzz-qce-halo.pk3
python3 scripts/animate-halo-weapons.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch --pk3 assets/baseq3/zzz-qce-halo.pk3
python3 scripts/convert-halo-map.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/map --pk3 assets/baseq3/zz-qce-bloodgulch.pk3
./scripts/bootstrap-bspc.sh
python3 scripts/build-bloodgulch-bots.py
QCE_MAP=qce_bloodgulch ./scripts/run-client.sh
```

Weapon/player conversion takes several minutes. The first AAS compilation is
CPU-heavy and can take tens of minutes; verified matching outputs are cached. The
BSPC bootstrap pins its source revision, rejects repeated ancestor planes, and
partitions dense terrain with exact axial splits before its candidate search.
It also expands the compiler's fixed reachability pool and fixes its 32-bit MD4
word type on 64-bit Linux. Navigation is stored in the same local
Blood Gulch PK3 as the matching BSP. Rebuilding the map requires rebuilding its
AAS; an AAS from a different map revision will not work.

If adding bots reports `AAS not initialized`, quit the game and run
`./scripts/bootstrap-bspc.sh`, then
`python3 scripts/build-bloodgulch-bots.py`, then launch again. The launcher now
checks for matching navigation before opening Blood Gulch. If it reports a
mismatch despite rebuilding, check for older map PK3s or loose files under
`assets/baseq3/maps` and `runtime-client/baseq3/maps` overriding the new package.
Running map conversion again replaces the PK3; always run the navigation builder
afterward to put its matching AAS back into the package.

`gv_maxHeldWeapons 0` applies immediately to living players; `give all` then
provides all eight Halo weapons, which you can cycle with Q. Lowering the limit
prevents new additions until your inventory is under that limit; it preserves
weapons you already hold until you drop or replace them.

Open the console with **~** and add bots:

```text
bot_add 0 16
```

The launcher allows 128 clients by default; `QCE_MAX_CLIENTS=32` selects a
smaller limit. This adds 16 bots with independently randomized skills and armor colors. Bot
model choices are independently randomized between Spartan and Cyborg. Without
the optional Cyborg package they render as Spartans, with a console notice.
Bots do not drive vehicles yet.

## Player models

```text
set qce_playerModel spartan
set qce_playerModel cyborg
set qce_playerModel quake
model ranger
```

The first two use Halo animations, armor/visor color settings and vehicle poses.
`quake` uses the existing Quake model system and the normal `model` cvar; examples
include `sarge`, `ranger`, `bones`, `klesk` and `visor`. Quake models retain their
stock animations and skins, so their weapon grips and vehicle seat poses are
not Halo authored. Halo gameplay and weapons remain enabled. The model cvar is
archived and changes without restarting the map.

## Optional E3-1999 Cyborg conversion

[Digsite's README](https://github.com/digsite/h1#license-information) states that
its content is **not open source** and is limited to MCC projects. This converter
supplies no permission to use that artwork outside MCC. The repository contains
only importer code, and does not redistribute the Digsite model or textures.
Use the import only where you have permission for your intended use.

If you have permission for this use, obtain the tested source revision locally:

```sh
git clone --filter=blob:none --sparse --no-checkout https://github.com/digsite/h1.git assets/halo/digsite
git -C assets/halo/digsite sparse-checkout set tags/digsite/characters/cyborg/99_e3
git -C assets/halo/digsite checkout --detach 4b326caf864b48c0b9c4db62338082691f2b645b
```

Given a local Digsite/HEK `tags` tree containing
`digsite/characters/cyborg/99_e3`, run after the retail conversion above:

```sh
CC=gcc .tools/bin/python -m pip install reclaimer==2.11.2
.tools/bin/python scripts/convert-halo-cyborg.py \
  --tags assets/halo/digsite/tags \
  --retail-output assets/halo/converted/bloodgulch \
  --output assets/halo/converted/cyborg \
  --pk3 assets/baseq3/zzzz-qce-cyborg.pk3
```

The importer reads `99_e3.gbxmodel`, `e3_99_diff.bitmap` and `e3_99_multi.bitmap`.
It reuses the 65 converted retail animations through the matching named skeleton,
including aim interpolation and hand attachment tags. It also generates RGB armor,
a visor layer and helmet portraits. Restart the game after adding a package.
Reconvert Cyborg if the retail animation package changes its frame layout.
The default `cyborg` now uses E3-1999 (`99_e3`), replacing the old Macworld
conversion. `--variant 00_mac` retains the previous optional importer. If you
already cloned the Digsite tree, add the E3 files with
`git -C assets/halo/digsite sparse-checkout add tags/digsite/characters/cyborg/99_e3`,
then rerun the converter above to overwrite your local Cyborg PK3 and restart.

## Cameras and safe spawns

Press **V** to toggle first person / third person. Console settings are
`cg_qceCameraMode 0` (first person) and `cg_qceCameraMode 1` (third person).
`qce_camera` toggles; `qce_camera 0` or `qce_camera 1` selects directly.

Adjust the third-person camera with these archived cvars, in Quake world units:

```text
cg_qceCameraBack 90
cg_qceCameraUp 8
cg_qceCameraSide 24
```

Back is distance behind the eye along the view direction (0–500). Up is vertical
world offset (-200–200). Positive side moves right; negative side moves left
(-200–200). Old camera mode 2 migrates to mode 1 with side -24. Camera placement
still traces against walls. Vehicle/death cameras use their existing views.

Gameplay firing, collision and damage always use the first-person eye/muzzle,
independently of the camera. The crosshair is static by default in both views.
Set `cg_qceDynamicCrosshair 1` to project the first-person aim trace into the
third-person viewport; this option hides the reticle within one metre of the eye
(80 / 3.048 Quake units) or when the aim point is offscreen/behind the camera.
Reticle coloring and magnetism still use the first-person view in either mode.

Observers and third-person players see projectiles launch from the displayed
gun's authored primary-trigger marker. The offset decreases smoothly with travel
toward the server's first-person aim point, reaching zero there. It is retained
across late snapshots, and cannot reappear after convergence/ricochet. Sniper smoke
uses the same barrel-to-impact curve. Collision/damage always use the real path;
world geometry also clips the cosmetic launch offset. Numeric muzzle markers are
part of the source, so weapon assets do not need reconversion for this change.

Run `qce_models` in the console to check whether Cyborg actually loaded. Selecting
`qce_playerModel cyborg` falls back to Spartan if its optional package is absent
or invalid. Copy your existing converted `zzzz-qce-cyborg.pk3` into
`assets/baseq3`, or use the permission-qualified optional conversion steps above,
then restart the game. Cloning/updating the source does not install this artwork.

If every spawn is occupied, players and bots wait one second, then try free starts
or a nearby floor position. Candidates require world clearance, 64 units of
horizontal space from living players and no vertical stacking. No spawn telefrag
is performed. If no safe candidate exists, spawning stays queued and retries.

## Future updates

From the repository folder, with the game closed:

```sh
git checkout work
git pull --ff-only origin work
export PATH="$PWD/.tools/bin:$PATH"
./scripts/build.sh client
QCE_MAP=qce_bloodgulch ./scripts/run-client.sh
```

A normal code update does not require reconverting assets unless its release
notes change the asset format. If `git pull` reports local modifications, preserve
those changes before resolving the update; do not reset your checkout blindly.

## Validation for this pass

Native OpenGL 1 and QVM/OpenGL 2 playtests loaded the generated Blood Gulch AAS
and ran 16 bots through combat. Both player families and the stock Quake model
option were checked in the shoulder-camera views. Separate 128-bot Q3DM17 tests
covered randomized model choices, map restart and absence of spawn telefrags.
Spawn fixtures cover the exact one-second wait, free-space retry, floor offsets,
wall rejection and prevention of standing on another player. Terrain topology,
IQM rotation/attachment conversion and navigation checksum tests pass.

The bot navigation follows converted collision geometry; retail Halo scenery,
optimized visibility, bot vehicle driving and bespoke Quake vehicle seat poses
remain outside this pass. Bot navigation compilation may emit numerical geometry
warnings; the produced file must pass the checksum/area checks and an in-game
load, as it did in these tests.
