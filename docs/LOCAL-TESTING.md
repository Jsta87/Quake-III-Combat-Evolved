# Test on your own computer

The current build and launch scripts target Linux with a graphical desktop
(or a working graphical Linux environment). Python 3 with venv support, a C compiler, CMake 3.25
or newer, Ninja, and SDL2 development headers are required. Windows/macOS launcher
support has not been validated. No original Halo executable is used at runtime.

On Ubuntu/Debian, install `build-essential`, `python3`, `python3-venv` and
`libsdl2-dev`. The bootstrap scripts install pinned CMake/Ninja tools locally and
use system SDL2 when available; the SDL fallback installs under `.tools/sdl/`,
with `QCE_SDL_PREFIX` available for a custom prefix.

Clone the development branch `work`. The default branch `main` currently
contains the initial scaffold rather than the implemented game.

```sh
git clone --branch work https://github.com/Jsta87/Quake-III-Combat-Evolved.git
cd Quake-III-Combat-Evolved
mkdir -p assets/baseq3 assets/halo
```

## Supply game data

Copy your Quake III Arena `pak0.pk3` through `pak8.pk3` into `assets/baseq3/`.
These provide the arena maps, bots, menus and remaining Quake resources. `pak0.pk3`
is required by the launcher; use the complete installed set for consistent tests.
Missionpack/Team Arena data is not needed for this baseq3 prototype.

Copy your original Xbox Halo CE build-2276 `bloodgulch.map` into `assets/halo/`.
This is the build already used by the project, with header build string
`01.10.12.2276`. Halo PC/Custom Edition maps and other Xbox builds are not supported
by the current converter. Keep the filename lowercase on Linux.

`ui.map` from the matching Xbox release is optional for UI asset extraction;
it is not required to play the current weapon-presentation prototype. No other
Halo maps, executable, disc image, or extracted tag directory is required now.
Commercial source data and generated assets are ignored by Git.

Expected inputs:

```text
assets/
  baseq3/
    pak0.pk3
    ...
    pak8.pk3
  halo/
    bloodgulch.map
    ui.map          # optional
```

## Build, convert, launch

```sh
./scripts/bootstrap.sh
./scripts/bootstrap-client.sh
export PATH="$PWD/.tools/bin:$PATH"
./scripts/build.sh client

python3 scripts/convert-halo-assets.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch \
  --pk3 assets/baseq3/zzz-qce-halo.pk3
python3 scripts/animate-halo-weapons.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch \
  --pk3 assets/baseq3/zzz-qce-halo.pk3

./scripts/run-client.sh
```

The first converter exports textures, audio and source geometry. The second
exports skeletal weapon/arm models, animation bindings/sounds, digit skins and
materials, then replaces the same local PK3. Both stages are required for the
latest presentation. Conversion takes a few minutes. Quit the game before
regenerating its PK3. There is no need to convert Quake's PK3s.

For an existing converted setup, the latest HUD visor/shield corrections can be
regenerated without rebaking weapon animations:

```sh
python3 scripts/animate-halo-weapons.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch \
  --pk3 assets/baseq3/zzz-qce-halo.pk3 --materials-only
```

The client starts q3dm1 with Halo movement/combat enabled. Pick up a second weapon
and use number keys or Q to switch. R reloads, F melees, right mouse throws a
grenade, G changes grenade type, E replaces a nearby gun, Ctrl crouches, and the
wheel zooms in/out. `qce_drop` drops the current weapon from the console. L toggles
the local flashlight preview where a light marker is available.


The assault rifle should show 60 on its weapon display when full, decrement with
predicted shots and refill after reloading. The shotgun reload should open,
insert shells, then close; pressing fire while it has ammunition closes before
firing. Switching to another carried weapon cancels a reload without adding
unfinished ammunition. The plasma pistol should enter charging, hold charged,
fire on release, then show overheat/recovery as appropriate.

Use `qce_status` in the console to inspect gameplay state. `cg_debuganim 1` logs
clip changes and animation sound events. Perfect Xbox visual/audio parity is
still outstanding; see [Weapon presentation](WEAPON-PRESENTATION.md).

## Updating and multiplayer

After pulling gameplay changes, rebuild client/server modules together. This
pass uses QCE protocol **99**, including the larger entity pool and replicated variant traits; older
project builds and stock Quake/ioquake3 clients are incompatible. The client build
also produces a dedicated server, or build it separately with
`./scripts/build.sh server`. Every participating machine needs its own Quake data
and the same converted Halo package for matching presentation.

`QCE_MAP` chooses another installed Quake arena. `QCE_DATA_DIR` can instead point
to an external directory containing `baseq3/`; place the converted Halo PK3 beside
Quake's pak files in that directory too.

Removing `zzz-qce-halo.pk3` restores Quake weapon presentation and sounds while
keeping the project's Halo gameplay enabled.

See [playtest fixes and controls](PLAYTEST-FIXES.md) for Spartan colors, optional
lunge, movement/view scale tuning and development cheats.

See [the playtest follow-up](PLAYTEST-FOLLOWUP.md) for the previous protocol changes, 128 slots,
reload interruption, new material conversion and scoreboard controls.

See [game variants and the next playtest pass](GAME-VARIANTS.md) for all 29 variant
cvars, save/load, Spartan bot batches and the current large-lobby limits.

## Combat presentation pass

Reconvert the Halo assets and rebuild both engines/modules for protocol **99**.
Try `set qce_visorRGB "30 160 255"` for a blue visor. Bots always use gold.
`set gv_maxHeldWeapons 0` followed by `map_restart 0` enables all eight Halo guns;
`give weapons` fills them when cheats are enabled. Restore `2` and restart for
normal Halo inventory. See [combat presentation](COMBAT-PRESENTATION.md) for
behavior, conversion commands, validation and remaining retail-parity limits.

## Blood Gulch and Warthog test

After converting the weapon/player assets, also run:

```sh
python3 scripts/convert-halo-map.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/map \
  --pk3 assets/baseq3/zz-qce-bloodgulch.pk3
QCE_MAP=qce_bloodgulch ./scripts/run-client.sh
```

Use E to enter/exit a nearby Warthog, WASD to drive and the mouse for its chase
camera. See [vehicle testing and current limits](VEHICLES.md). This first map
conversion has no bot navigation, and the vehicle turret is not functional yet.
