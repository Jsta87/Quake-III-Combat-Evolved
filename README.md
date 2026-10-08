# Quake III: Combat Evolved

Halo CE gameplay on Quake III arenas, built on [ioquake3](https://github.com/ioquake/ioq3).
Gameplay values come from the uploaded Xbox build 2276 maps. All eight systems
in the gameplay pass now have implementations and regression coverage. The
profile remains `mixed`: converted presentation, animated collision and retail
comparison are still needed. Vehicles are deferred.

## Build and run

Linux needs Python 3, a C compiler, CMake >= 3.25, Ninja and SDL2 development files.
The bootstrap scripts install pinned build tools and local SDL dependencies:

```sh
./scripts/bootstrap.sh
./scripts/bootstrap-client.sh
export PATH="$PWD/.tools/bin:$PATH"
./scripts/build.sh server
./scripts/build.sh client
```

Builds produce native and QVM modules for baseq3 and missionpack. CMake validates
and regenerates schema-3 gameplay definitions and rebuilds affected modules.
`QCE_BUILD_JOBS` defaults to 4.

Owned Quake III `pak0.pk3` through `pak8.pk3` go in `assets/baseq3/`, or set
`QCE_DATA_DIR` to a directory containing `baseq3`. Commercial maps/PK3s, extracted
manifests, runtime directories and build outputs stay ignored by Git.

```sh
./scripts/run-client.sh
./scripts/run-server.sh +set g_qceMovement 1 +set g_qceCombat 1
```

The client starts q3dm1 with both profiles enabled; the server defaults to stock
profiles unless overridden. `QCE_MAP` selects another installed arena. Launchers
stage native modules and disable pure checking for development. Use
`+connect localhost` to join a running local dedicated server.

Networking uses QCE protocol **95**. Rebuild both engines and all modules together;
stock Quake/ioquake3 and older project connections/demos are incompatible.

## Halo assets

The uploaded Xbox maps now have an offline [asset converter](docs/ASSET-CONVERSION.md)
for textures, audio, and weapon models. The second stage adds
[animated weapons and Spartan arms](docs/WEAPON-PRESENTATION.md), moving attachment
points, and layered Halo materials. Generated art and PK3s stay local under
`assets/`. Player models, effects, HUD integration and remaining retail material/
animation fidelity are still in progress.

For the complete data/build/conversion walkthrough, see
[Local testing](docs/LOCAL-TESTING.md).

## Controls

| Action | Control |
| --- | --- |
| Reload | R |
| Grenade / select type | G / T |
| Replace nearby firearm / drop | E / Q |
| Melee | F |
| Cycle weapon zoom | Right mouse button |
| Local flashlight preview | L |
| Select carried weapon | Number keys / wheel |

[Controls](config/qce-controls.cfg) load automatically. `qce_status` reports
inventory, vitality, heat, charge, error and profile hash.

## Gameplay

- Two firearms plus melee, ammunition conservation, reserves and interrupted
  per-shell shotgun reload. E replaces the active gun with a nearby visible
  pickup; dropped weapons preserve ammunition, battery, heat and trigger state.
- 75 health / 75 shields, six-second recharge delay, four-second recharge.
  Damage scales shields and health separately. The Magnum's third shot can
  break full shields and kill if it hits the head; sniper headshots can kill
  through shields. Backsmacks remain lethal subject to protection checks.
- Finite bullets/pellets, projectile slowdown, gravity, distance limits, water
  parameters, randomized damage and inner/outer blast falloff. Weapon material
  tables govern penetration and grazing Needler ricochets. Quake surface flags
  map arena geometry to a subset of Halo materials.
- Sustained spread and fire-rate ramps. Shared fixed-point trigger state keeps
  client prediction and server decisions consistent.
- Weapon-ready timings, full/empty reload durations, melee keyframe impacts and
  overheat recovery timings from tag/animation metadata. The optional converted
  package supplies first-person Halo clips and animation sounds.
- Fractional battery consumption, plasma heat and cooling, charged plasma-pistol
  release after 600 ms, homing and shield-stripping EMP. The HUD shows battery percent.
- Authoritative 2× Magnum/rocket and 2×/8× sniper zoom, scoped sniper accuracy and
  damage dezoom. Precision rays resolve a temporary head ellipsoid through the
  arena hull; animated Halo model collision is still needed.
- Imported directional movement speeds, absolute ground/air acceleration,
  jump, gravity, crouch camera transition and slope modifiers. Quake BSP
  collision, hazards, landing damage and damage impulses retain adaptations.
- Independent frag/plasma inventory. Both throw at 800 Quake units/s without an
  artificial upward impulse. Frags arm on the first bounce; plasma bounces off
  walls, sticks to players or settles on the floor, then arms its two-second fuse.
  Attachments are safe against respawns/entity reuse. Needler needles attach for
  750 ms and combine in groups of seven.

See [the eight-item completion record](docs/GAMEPLAY-PASS.md) and
[import values and remaining parity limits](docs/HALO-IMPORT.md). The uploaded
maps suffice for the current first-person weapon conversion. Player models, HUD,
full effects and remaining material fidelity are the next milestones. Bots and remote latency/retail
validation remain work.

## Verification

```sh
python3 tests/profile.py
python3 tests/halo-extraction.py
for suite in movement shields weapons melee headshots grenades swap needles heat network-state spread systems projectiles blasts; do
  ./scripts/test-${suite}.sh || break
done
```

Fixtures exercise actual shared prediction and server handlers with controlled
geometry. Native/QVM builds and offscreen q3dm1 smoke checks are separate from
human multiplayer, audio and retail comparison.

```sh
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy ./scripts/run-client.sh +set r_mode 3 +set r_fullscreen 0 +set s_initsound 0 +set net_ip 127.0.0.1 +set net_port 27966
```

See [Halo data workflow](docs/HALO-DATA.md), [integration](docs/INTEGRATION.md)
and [source provenance/licensing](docs/UPSTREAM.md). Upstream notices and GPL
license remain under `engine/`.

Recent playtest fixes and console controls: [PLAYTEST-FIXES.md](docs/PLAYTEST-FIXES.md).
