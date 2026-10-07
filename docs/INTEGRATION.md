# Halo gameplay on Quake III arenas

Retain ioquake3 rendering, Quake III BSP maps, collision and client/server
transport. Replace gameplay progressively with Halo CE behavior. Experimental
movement, shields, two-weapon inventory, reload, weapon replacement, grenade
types/sticky attachment, melee lunge/backsmacks, damage classes and precision
exceptions are implemented. No retail Halo accuracy is claimed.

| System | Integration point | Status |
| --- | --- | --- |
| Movement/prediction | `bg_pmove.c`, shared player stats | Shared provisional simulation; retail calibration pending |
| Shields/damage | `g_combat.c`, `g_client.c`, `g_active.c` | Recharge, overflow, category scaling, headshots/backsmacks |
| Weapons/inventory | `bg_misc.c`, `bg_pmove.c`, `g_cmds.c`, `g_weapon.c` | Profile-driven definitions, two slots, reload, explicit replacement |
| Grenades | `g_missile.c`, `g_weapon.c`, shared inventory/events | Frag/plasma selection, sticky moving-target attachment, safe detach |
| Presentation | `cg_draw.c`, `cg_event.c`, `cg_weapons.c` | Inventory/action HUD and Quake placeholder models/sounds/animations |
| Data pipeline | `data/gameplay-profile.json`, `scripts/generate-profile.py` | Validated shared C/QVM definitions and canonical hash; extraction pending |
| Vehicles | Future authority/physics/seats/weapons simulation | Deferred |
| Maps | Existing Quake arena entity/collision behavior | Kept for now; compatibility/vehicle placements deferred |

The next phase begins with owned retail Xbox maps and version identification;
see [upload requirements and conversion workflow](HALO-DATA.md). Extract weapon,
damage, projectile, player/global and animation references offline into a
reviewable, versioned profile. Convert visual resources separately. The runtime
will use Quake-compatible data and generated shared gameplay definitions.

Focused fixture tests and native/QVM builds verify the current implementation.
Live human targeting, remote multiplayer/latency, audio, bot adaptation and
retail parity remain pending. Gameplay profiles take effect at spawn; clients
must use rebuilt modules. Complete vehicles and map adaptations are later
milestones, not prerequisites to the first tag extraction.
