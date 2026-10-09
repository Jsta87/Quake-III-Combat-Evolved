# Halo weapon presentation

Follow [Local testing](LOCAL-TESTING.md) to supply game data, build, convert and
launch. No additional upload is needed for this pass. The converter uses the
supplied Xbox build-2276 Blood Gulch map; original art and generated PK3s remain
Git-ignored.

## Implemented

All eight slots use original first-person weapon geometry and Spartan arms.
The exporter decodes uncompressed rotation, translation and scale tracks,
matches bones by name, preserves two-bone weights and converts Halo's quaternion
convention. It exports 3,856 source frames in eight compact skeletal IQM models.

The client selects idle, firing, ready/put-away, full/empty reload, melee, grenade,
charge, overheat and recovery clips. Missing clips fall back safely. Empty reload
selection is latched until completion; shell insertion restarts on each loaded
round, and weapon changes cancel the previous presentation. Ready, drop and
reload phases fit their gameplay timers, with interpolation between source poses.
Missing put-away tracks use a downward translation as an explicit adaptation.

Shotgun gameplay now has opening (500 ms), per-shell insertion (400 ms), closing
with ammunition (800 ms) and empty closing (1,400 ms). Durations come from source
tracks. Pressing fire during insertion when a shell is loaded begins closing
before firing. Switching to another carried gun cancels reload without granting
unfinished ammunition. Magazine reloads block firing but are interrupted by melee, grenades, switches
and pickups. Ammunition already inserted remains loaded when an action interrupts.

Plasma-pistol presentation distinguishes charging, charged hold, charged fire,
secondary overheat entry, hot hold and recovery. Charged hold keeps the ending
pose while its additive jitter overlay remains deferred. A shot that locks the weapon hot
enters overheat immediately on its final shot, retaining the firing sound. Out-of-range retail loop
indices are retained in the manifest and mapped to local clip frame zero; their
linked-track semantics still require retail comparison.

The assault-rifle weapon display selects original digit textures using a cached
skin for each predicted magazine count from 00 through 60. Reloading updates the
display when ammunition is actually transferred. It does not rely on global
shader remaps, which could change another player's weapon. The world-model
counter remains static; mechanical ammunition/needle overlays are still pending.
World weapons and Spartan/RGB presentation are described in
[PLAYTEST-FIXES.md](PLAYTEST-FIXES.md).

Animation graph sound references and their source frame indices become runtime
sound events, played once per clip instance. Repeated shots and shell insertions
can each trigger a new event; cancelled reloads stop their embedded audio.
Ready, reload, melee and recovery sounds are connected where source clips supply
them. Plasma-pistol start/held charge sounds come from its actual looping-sound
attachment; charged fire audio comes from its secondary trigger effect. Held
charge uses an engine looping sound and stops on release/cancellation. Source
fire animations with embedded shot audio replace the ordinary local shot sound
so it is not played twice. Sound events continue when the view model is hidden.

Muzzle, ejection and light markers become joints on the original weapon bones.
Muzzle flashes/lights and ballistic casing ejection use their interpolated
positions and orientations. L toggles a local flashlight preview traced from
`tag_light`. Both renderers now interpolate IQM tags in the same direction as
model poses. Effects currently use Quake flash/casing resources and adapted
velocity/lifetime parameters; the full Halo particle/effect definitions have not
been converted. Remote players retain existing third-person effects. The local
flashlight preview has no replicated gameplay state or Halo battery/timer rules.

Materials use source base/detail textures and Xbox multipurpose channels: red
specular, green illumination, blue color change, alpha auxiliary. Diffuse and
emissive layers split base color to avoid double counting. Specular masks use
Quake's lighting approximation. Supported detail modes are layered; unsupported
biased/masked modes are recorded. Transparent Chicago/generic/glass/meter
materials use recorded first-layer approximations. Source parameters, animation
buffers/events, audio bindings and output hashes remain in the manifest.

## Remaining fidelity work

Perfect retail parity is not yet established. Additive aim, ammunition
and charged jitter overlays, retail view/FOV placement, interrupted-pose blending,
linked-animation loop semantics and exact ejection event scheduling remain.
Exact distance attenuation, charge-track fades, randomized event delay rules and
third-person source animation sounds require further work. Halo cubemap
reflection, transparent map combiners, weapon color change and several detail modes
still need renderer work.

Use `cg_debuganim 1` to report clip changes and sound events. `testgun
models/qce/halo/view/shotgun.iqm` inspects a model independently of inventory;
enter `testgun` to clear it. `cg_qceFlashlight 1` enables the local light preview.

## Validation

Synthetic fixtures cover track defaults/malformed data, quaternion signs,
hierarchies, inverse-bind skinning, moving markers, MD3 inspection frames, IQM
serialization, material masks, sound event export/bounds and all counter skins.
Client timing tests cover reload latching/restarts/cancellation, charge/fire/hot/
recovery priority, interpolation and once-per-instance sound scheduling.
IQM meshes include orthogonal tangents required by OpenGL 2. Renderer tests
exercise actual IQM tag interpolation in OpenGL 1 and 2, including
endpoints, reverse interpolation and child bind transforms. Gameplay tests cover
shotgun phases, interruption, conservation, prediction and protocol-100 state.

Native/QVM client and dedicated-server builds pass. Local offscreen runs in
OpenGL 1 and 2 validate source audio loading,
weapon displays, reload/switch phases and model registration. SDL's dummy audio
backend verifies playback dispatch, not an audible comparison against Xbox Halo.
Retail side-by-side validation remains outstanding.

Format/layout reference: Invader commit
`696830ff80af227e84e7237c2ef26eb2301ed110`; quaternion convention cross-checked
against Reclaimer commit `a0a56ca7e95957e4cef165f606ae6ab6b659113a`.

See [the playtest follow-up](PLAYTEST-FOLLOWUP.md) for protocol 96, 128 slots,
reload interruption, new material conversion and scoreboard controls.

The [latest playtest pass](GAME-VARIANTS.md) adds independent additive retail
movement tracks in both renderers and attachment queries. Gunshots are dispatched
separately from mechanical animation audio, and camera walk bob is removed.
