# Halo gameplay on Quake III arenas

ioquake3 supplies rendering, Quake III BSP collision and client/server transport.
The shared gameplay layer imports Xbox build2276 values and implements shields,
two-weapon inventory, reload, melee/backsmacks, precision rules, plasma heat/EMP,
Needler tracking/combine, finite projectile travel, damage/material curves,
trigger ramps, battery energy, zoom, directional movement and hand grenades.
See [the eight-item pass](GAMEPLAY-PASS.md) and [parity limits](HALO-IMPORT.md).

| System | Integration points |
| --- | --- |
| Shared prediction/profile | `bg_pmove.c`, `bg_misc.c`, `bg_public.h`, generated definitions |
| Authoritative travel/material impact | `g_missile.c`, `g_qce_projectile.h`, `g_qce_needle.h` |
| Damage/shields/head geometry/blasts | `g_combat.c`, `g_active.c`, `g_client.c` |
| Inventory and dropped state | `g_items.c`, `g_cmds.c`, `g_combat.c` |
| HUD/camera/effects | `cg_draw.c`, `cg_view.c`, `cg_weapons.c`, `cg_event.c` |
| Replicated state | `q_shared.h`, `msg.c`, project protocol93 |
| Offline pipeline | Xbox layout/reader, profile importer/generator, schema3 |

Rebuild client/server engines and all native/QVM modules together. The project
protocol is incompatible with stock and previous project builds/demos. Weapon
projectiles are authoritative; shotgun client effects no longer fabricate
instant hits before a pellet reaches its target.

Converted art/audio/animation channels and player collision nodes are next.
Values are already compiled into Quake-compatible shared definitions; retail
maps are not required by the runtime. Keep uploaded assets/full manifests and
build/runtime outputs out of Git. Vehicles, arena entity adaptation, bots and
remote multiplayer/latency validation remain later work.
