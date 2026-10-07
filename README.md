# Quake III: Combat Evolved

Halo CE gameplay on Quake III arenas, using [ioquake3](https://github.com/ioquake/ioq3).
The client/server foundation and experimental combat systems are implemented.
Values, Quake models, sounds and animations remain provisional; retail Halo
accuracy and vehicles are future work.

## Build and run

Linux requires Python 3, a C compiler, CMake >= 3.25, Ninja, and SDL2 development
files for the client. On a minimal cloud image, the bootstrap scripts install
pinned build tools and build SDL2 locally without a system package install:

```sh
./scripts/bootstrap.sh
./scripts/bootstrap-client.sh
export PATH="$PWD/.tools/bin:$PATH"
./scripts/build.sh server
./scripts/build.sh client
```

Builds produce native and QVM modules, including baseq3 and missionpack.
`QCE_BUILD_JOBS` defaults to 4. Both the build script and direct CMake configure
validate/generate the shared gameplay profile; editing the JSON triggers CMake
reconfiguration and module rebuilding.

Supply owned Quake III `pak0.pk3` through `pak8.pk3` under `assets/baseq3/`, or
set `QCE_DATA_DIR` to the directory containing `baseq3`. These files, Halo data,
runtime directories and build outputs are ignored and must stay out of Git.

```sh
./scripts/run-client.sh
# Dedicated server: experimental profiles take effect on spawn.
./scripts/run-server.sh +set g_qceMovement 1 +set g_qceCombat 1
```

The client launcher loads q3dm1 and enables both profiles. The server launcher
uses stock profiles unless overridden. Use this project's rebuilt clients;
vanilla clients lack the shared rules/HUD. `QCE_MAP` chooses another installed
map. Launchers stage native modules and disable pure checking for development.
For a local client connected to the dedicated server, use the client launcher
with `+connect localhost` after starting the server.

Cloud rendering smoke test (no audible sound):

```sh
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy ./scripts/run-client.sh +set r_mode 3 +set r_fullscreen 0 +set s_initsound 0 +set net_ip 127.0.0.1 +set net_port 27966
```

## Controls and implemented systems

| Action | Default control |
| --- | --- |
| Reload | R (`+button12`) |
| Throw selected grenade | G (`+button13`) |
| Select frag / plasma grenade | T (`qce_grenade_type`) |
| Replace active firearm with nearby weapon | E (`qce_swap`) |
| Drop active firearm | Q (`qce_drop`) |
| Melee | F (`+button14`) |
| Select carried firearm | Existing number keys / weapon wheel |

The launcher loads [controls](config/qce-controls.cfg); later `+bind` arguments
can override them. `qce_status` reports slots, magazine/total ammo, both grenade
counts, selected type, shields, health, action state and gameplay-profile hash.

- Movement: replicated, shared prediction; vector acceleration avoids Quake
  strafe-speed buildup. Provisional run speed 200 units/s, crouch factor 0.5,
  jump impulse 300, reduced air acceleration 0.5. Gravity, collision, slopes,
  water, hazards and map interactions still use Quake behavior.
- Health/shields: normally 100 health / 100 shields, without the +25 spawn
  health bonus. Recharge starts after five seconds without damage and restores
  two points per 100 ms. Damage classes scale shields and health independently,
  conserving base damage when shields break. Provisional plasma/lightning
  scaling is 2x against shields and 0.5x against health.
- Inventory: two firearms plus gauntlet. Loaded ammunition is part of total
  ammunition; reloading never creates rounds. Capacities, reload/fire timing,
  spread, recoil, firing behavior, projectile speed/fuse/splash, damage policy
  and melee settings come from [the gameplay profile](data/gameplay-profile.json).
  Empty magazines auto-reload when reserve ammo exists. Reload and melee lock
  conflicting actions until they finish.
- Replacement: E selects a visible weapon within 64 units and in front of the
  player. With two full slots it drops the active firearm, preserving total
  ammo and magazine, then equips the pickup. With a free slot it adds the
  pickup without dropping anything. Q drops explicitly. The owner cannot
  immediately re-pick a drop for two seconds. Death drops preserve both slots.
  Full inventory while gauntlet is selected requires choosing a firearm first.
- Grenades: independent frag/plasma counts, two of each on spawn, up to four
  of each. G throws once per press; the event records the selected type.
  Grenade ammo boxes replenish the selected type. Frags bounce off bodies and
  surfaces and explode on their fuse. Plasma sticks on first contact, arms
  its fuse, follows moving/rotating targets, and safely stops following a
  respawned or reused target. Unattached grenades also expire on their fuse.
- Melee: keeps the firearm equipped and spends no ammo. Each weapon defines
  strike damage, reach, cooldown, lunge reach/speed and delayed lunge impact.
  A directly aimed body beyond strike reach but within lunge reach triggers
  shared predicted forward motion; the server traces again at impact. Walls
  block strikes. A strike from behind a player is lethal through shields;
  friendly-fire, god mode and other protection checks still apply.
- Precision: railgun is a pistol placeholder, BFG is a sniper placeholder using
  hitscan rail behavior and Quake presentation. A qualifying pistol headshot
  kills when its normal damage leaves shields at zero, including **the same
  shot that breaks them** and exact depletion. The sniper policy permits a
  headshot kill through remaining shields. Body shots follow normal damage
  rules. The provisional head zone is the top 20% of the current collision
  box, including crouching, rather than Halo model/bone hitboxes. Headshot
  events identify the shooter for HEADSHOT text and the Quake excellent cue.

These are working systems, not measured Halo parity. The railgun's current
50 damage is a placeholder; the regression suite separately covers a 40-damage
third-shot shield-breaking headshot. Actual Halo weapon names, visuals,
animations, heat/overcharge, aim assistance, detailed damage effects and retail
behavior calibration still require the source data and further implementation.
Bots are not adapted to the new inventory/actions. Remote multiplayer, latency
correction, real audio output and live Halo comparison remain unvalidated.

## Verification and next data upload

```sh
python3 tests/profile.py
./scripts/test-movement.sh
./scripts/test-shields.sh
./scripts/test-weapons.sh
./scripts/test-melee.sh
./scripts/test-headshots.sh
./scripts/test-grenades.sh
./scripts/test-swap.sh
```

Tests execute shared Pmove/inventory code, real server damage dispatch, grenade
attachment and weapon replacement with controlled fixtures. They cover timing,
conservation, body/wall collision, backsmacks, precision exceptions, protections,
respawn safety, malformed profile rejection and stock fallback. Native and QVM
builds plus q3dm1 rendering are checked separately. These checks do not replace
human multiplayer or retail parity testing.

Upload guidance and the offline tag/profile workflow are in
[HALO-DATA.md](docs/HALO-DATA.md). See [integration status](docs/INTEGRATION.md)
and [source provenance/licensing](docs/UPSTREAM.md). Upstream README, GPL license
and notices are retained under `engine/`.
