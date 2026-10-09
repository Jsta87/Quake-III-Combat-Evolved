# Halo game variants and development controls

This pass adds 29 working server settings. The option families were checked against
Halo CE's recovered `universal_variant` / `game_variant_options` in
[game_engine.h](https://github.com/cybersecurity/halo-ce-universal/blob/main/source/game/game_engine.h).
Movement, jump, gravity and damage traits also follow later Halo custom-game
conventions. Unsupported modes, vehicles and motion-sensor options are not exposed.

Set these on the hosting game/server console. A remote player cannot load a variant
or add bots through a client command; a remote administrator can use RCON.

```text
set gv_primaryWeapon pistol
set gv_secondaryWeapon assaultrifle
set gv_movespeed 1.1
set gv_mapWeaponSet rifles
gv_save my_variant
gv_load my_variant
```

Names accept letters, digits, underscores and hyphens, with an optional `.cfg`
extension. Files go to `baseq3/variants/<name>.cfg` beneath the game's home folder
(`runtime-client/` for the client launcher, `runtime/` for the server launcher).
Save includes every option below. Load validates the entire versioned file before
applying anything; it does not execute arbitrary config commands. Cvars are archived
so settings survive quitting. `gv_load` applies a variant without changing the map.
Use `map_restart 0` or load a map for settings marked **Map**.

| Cvar | Default | Values / effect | Applies |
|---|---:|---|---|
| `gv_movespeed` | 1 | 0–4; scales player speed and acceleration; 0 stops horizontal movement | Live |
| `gv_jumpHeight` | 1 | 0–4; approximate ballistic jump-height multiplier at unchanged gravity | Live |
| `gv_gravity` | 1 | 0–4; player gravity multiplier; 0 removes player gravity | Live |
| `gv_primaryWeapon` | assaultrifle | Weapon name, `random`, or `none` | Respawn |
| `gv_secondaryWeapon` | none | Weapon name, `random`, or `none`; random avoids the primary | Respawn |
| `gv_maxHeldWeapons` | 2 | 1–8 guns; 0 allows all eight, with the Quake switch popup | Respawn |
| `gv_shieldMultiplier` | 1 | 0–8; 0 disables shields, 2 doubles capacity | Live capacity; full on respawn |
| `gv_healthMultiplier` | 1 | 0–8; 2 doubles vitality; 0 makes players invulnerable to ordinary damage | Capacity on respawn; invulnerability live |
| `gv_damageMultiplier` | 1 | 0–8; overall combat damage multiplier | Live |
| `gv_meleeMultiplier` | 1 | 0–8; additional melee damage multiplier | Live |
| `gv_grenadeMultiplier` | 1 | 0–8; additional frag/plasma damage multiplier | Live |
| `gv_shieldRechargeDelay` | 1 | 0–8; source recharge-delay multiplier | Next damage/respawn |
| `gv_shieldRechargeRate` | 1 | 0–8; recharge-speed multiplier; 0 disables recharge | Live |
| `gv_fragGrenades` | 2 | 0–4 starting frag grenades | Respawn |
| `gv_plasmaGrenades` | 2 | 0–4 starting plasma grenades | Respawn |
| `gv_grenades` | 1 | 0/1; enables grenade throwing and starting grenades | Live / respawn |
| `gv_infiniteAmmo` | 0 | 0/1; unlimited reserve ammunition/battery; magazines still empty and reload | Live |
| `gv_infiniteGrenades` | 0 | 0/1; throws do not consume the existing grenade stock | Live |
| `gv_weaponPickup` | 1 | 0/1; enables weapon pickup/replacement | Live |
| `gv_weaponDrop` | 1 | 0/1; manual/death weapon drops; replacement requires dropping the old weapon | Live |
| `gv_respawnTime` | 1.7 | 0–60 seconds before respawn is allowed | Next death |
| `gv_suicidePenalty` | 0 | 0–60 additional respawn-delay seconds for suicide | Next death |
| `gv_weaponRespawn` | 5 | 1–120 whole seconds for map weapon respawn | Next pickup |
| `gv_mapWeaponSet` | default | `default`, `none`, `random`, `pistols`, `shotguns`, `rifles`, `rockets`, `snipers`, `plasma`, `needlers` | Map |
| `gv_friendlyFire` | 0 | 0/1; teammate damage | Live |
| `gv_scoreLimit` | 20 | 0–10000; 0 removes the score limit | Live |
| `gv_timeLimit` | 0 | 0–1440 whole minutes; 0 removes the time limit | Live |
| `gv_gameType` | slayer | `slayer`, `team_slayer`, `ctf` (current engine modes) | Map |
| `gv_invisibility` | 0 | 0/1; active camouflage for living players and held weapons | Live, expires shortly after disabling |

Weapon names: `pistol`, `assaultrifle`, `shotgun`, `rocketlauncher` (`rocket`),
`sniperrifle` (`sniper`), `plasmarifle`, `plasmapistol`, `needler`.
All multipliers apply on top of the imported Halo values and the existing coordinate
scale. Grenade/projectile gravity stays at its imported value; `gv_gravity` is a
player trait. Invulnerability permits deliberate console suicide/telefrag protection
bypass; it is not a replacement for every server death rule.

## Bots and crowded lobbies

```text
set sv_maxclients 128
map_restart 0
bot_add 0 127
```

This fills the remaining slots around one human. A dedicated server with no human
can run `bot_add 0 128`. `bot_add <skill> [count]` defaults to one bot. Skills 1–5
are fixed. Skill 0 independently draws each bot with probabilities 5%, 20%, 50%,
20%, 5% for skills 1 through 5. All added bots, including legacy `addbot`, receive
names `Spartan 001`, `Spartan 002`, etc. and randomized RGB armor. The serial
continues across maps in the current server process. Team games use team armor
colors. Skill affects AI, not a Quake health handicap.

The engine now supports 4096 entity slots, 512 visible entities per snapshot,
128 clients, a larger bot-state allocator and enough bot item-goal records for
crowded matches. At the entity ceiling, the oldest dropped weapon is reclaimed;
map objects, players, flags and live projectiles are preserved. Protocol **98** is incompatible with earlier builds. Server tick
and requested snapshot defaults are 30 Hz. The launcher also sets `rate 250000`;
the old 90000-byte/sec server cap is raised to 1000000 to avoid starving large
snapshots. 128 human connections have not been load-tested; bandwidth and machine
capacity still impose limits. Tiny arenas also produce frequent spawn telefrags.

## Controls and behavior

| Input | Action |
|---|---|
| Right mouse | Throw selected grenade |
| G | Switch frag/plasma grenade type |
| Q | Cycle carried weapons, including empty weapons |
| Wheel up / down | Zoom in / out one supported level |
| Ctrl | Crouch |
| R / F / E | Reload / melee / pick up or replace weapon |

`qce_drop` remains available in the console. Weapons remain selected when empty;
magazines automatically reload when reserves exist. Grenade throws, melee,
switches and pickups cancel reloads while preserving ammunition already inserted.
No gauntlet or Quake switch whoosh is used in Halo mode. Pickup draw starts in the
raising state. The launcher refreshes world pickup scale to `cg_qceWorldWeaponScale 1.65`,
including older archived settings. Extra launcher arguments can override it.

`sv_cheats 1` / `sv_cheats 0` now work on the host console. The launcher starts a
`devmap`, which enables cheats. Starting a normal map uses the engine's normal map
cheat initialization; cheats can subsequently be set explicitly.

Rocket/shotgun audio plays the firing-effect gunshot separately from the graph's
mechanical firing animation sound. Halo mode no longer halves self-inflicted blast
damage. Radial falloff keeps Halo's separate minimum/lower/upper damage bounds,
shield/body materials and line-of-sight checks. Collision still substitutes an
arena hull center for the animated Halo bounding-sphere center.

Walking composes the map's authored `first-person moving` track additively with
all weapon actions in both renderers, including muzzle/ejection/light attachments.
Movement begins immediately; stopping while idle blends back over six 30-Hz ticks.
Quake walk bob/lean is removed from the camera. Retail visual/audio comparison
is still needed before claiming perfect Xbox parity. This changes the renderer and
cgame interfaces, so rebuild the engine, renderers and modules together and re-run
both conversion commands in [LOCAL-TESTING.md](LOCAL-TESTING.md) after updating.

## Verification

```sh
./scripts/test-variants.sh
./scripts/test-entities.sh
./scripts/test-iqm-tags.sh
./scripts/test-weapon-presentation.sh
python3 scripts/test-bot-lobby.py --mode native --duration 60
python3 scripts/test-bot-lobby.py --mode qvm --duration 30
```

The lobby check requires the client build and local Quake assets. It uses an
isolated temporary home folder and writes logs to `build/tests/`. It verifies
128 bot names, independent skills, RGB userinfo, sustained randomized-loadout
combat, map restart, editable cheats, and variant save/load. Native and QVM runs
passed, using entity numbers above 3500. Native and QVM client smoke runs in
OpenGL 1/2 also dispatched rocket/shotgun gunshots and mechanical sounds separately.
A separate dedicated-server client check confirmed two rapid zoom steps and
grenade-type changes with chat flood protection enabled. These checks validate
behavior and registration, not audible or visual retail parity.

See [combat presentation](COMBAT-PRESENTATION.md) for visor RGB, projectile rendering, crosshairs/autoaim, shield effects, and bot grenade/melee behavior.
