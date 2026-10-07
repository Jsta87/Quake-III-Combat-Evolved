# Gameplay pass: items 1–8

Each requested item has a working implementation and controlled regression
coverage. The asset-conversion milestone can begin. These are implementation
results, not a claim of perfect retail Halo parity.

| Item | Implemented | Remaining validation/dependency |
| --- | --- | --- |
| 1. Projectile motion | Finite bullets/pellets; initial/final speed, slowdown, air/water gravity, swept collision, travelled range | Retail2276 clock/media behavior and bounce-step remainder |
| 2. Damage/materials | Random full-strength bounds, minimum damage, speed falloff, inner/outer blast radii, 33 weapon material tables and arena selection | Animated bounding spheres, all arena materials, impulses/self-damage and needle blast details |
| 3. Fire-rate ramps | Predicted per-slot growth/recovery, quantized rate, preserved/recovered drop state | Retail tick/latency comparison |
| 4. Action timing | Ready/swap timing, full/empty reload durations, all melee impacts at keyframes, overheat recovery timer | Converted animation channels/events; reload cancellation/chamber and rendered variants |
| 5. Battery | Fractional energy consumption, charged cost, HUD percent, conservation across drops/pickups | Cooling age precision and retail rounding |
| 6. Precision/zoom | 2× Magnum/rocket; 2×/8× sniper; authoritative cycling, scoped sniper accuracy, dezoom, ellipsoid ray resolution | Animated player collision/head nodes and rendered scopes |
| 7. Movement | Directional speeds, absolute acceleration/braking, air control, jump, gravity, hull radius, camera transition and slopes | BSP stepping, water/landing/stun/impulse behavior and retail comparison |
| 8. Grenades | Tag throw speed/origin/gravity, surface bounce, player sticking, settle-to-arm, fuse, blast curves and safe attachments | Release keyframe, spawn counts, broader material mapping and unarmed safety lifetime |

Verification: `test-systems.sh`, `test-projectiles.sh`, `test-blasts.sh`, existing
regression suites, native/QVM client/server builds and offscreen game smoke.
For values, provenance and precise adaptations see [HALO-IMPORT.md](HALO-IMPORT.md).

The next asset pass should inventory dependencies from both uploaded maps, then
establish one end-to-end weapon conversion: model, textures, animations and sound.
Player model/collision conversion should follow so temporary head geometry can
be replaced with animated head nodes. UI/HUD resources can be converted next.

## Verification recorded 2026-10-07

- All sixteen suites pass: two Python suites and fourteen shell suites (some
  execute multiple fixtures).
- Client and server builds pass for native and QVM baseq3/missionpack modules.
- Native and QVM q3dm1 rendering/startup pass. In-game checks confirm finite
  weapon firing, sniper zoom levels1/2, crouch progress0/10000, grenade inventory
  consumption, and charged plasma-pistol energy1000000→890000 (ammo500→445).
- Full owned-map profile import is idempotent and generated definitions are current.
- Network fixtures roundtrip the extended player state and the new high button
  bit. Runtime smoke caught and resolved missing engine zoom-button registration.
- Commercial assets remain ignored; no new commit or push was made in this pass.

Build/game logs are local under `/tmp/qce-eight-*.log`. These checks use controlled
fixtures and a local offscreen game; retail Xbox and remote human multiplayer
comparison are still pending.
