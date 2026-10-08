# Halo weapon presentation

Run the two conversion commands in [Asset conversion](ASSET-CONVERSION.md), then
rebuild and launch the client. No additional upload is needed. The animation stage
uses the supplied Xbox build-2276 Blood Gulch map and generated source sidecars.
Original art, converted assets and the runtime PK3 remain Git-ignored.

## Implemented

All eight weapon slots have their original first-person weapon geometry and
Spartan arms. The exporter decodes uncompressed default and animated rotation,
translation and scale channels, matches bones by name, preserves two-bone skin
weights, and converts Halo's quaternion convention. It exports 3,856 source
frames across eight skeletal IQM models. Skeletal storage avoids the model-memory
exhaustion caused by baking every vertex of every frame into MD3.

The client selects source clips for idle, firing, ready/put-away, full/empty
reload, melee, grenade throws, overheat recovery and plasma-pistol charge/fire
when those clips exist. Missing clips fall back to idle or the available reload.
Animation advances at the source NTSC/PAL frame rate; engine frame interpolation
provides smooth poses. Without the optional package, Quake view models remain.

Primary/secondary muzzle, ejection and flashlight markers become named joints
attached to the original weapon bones. The client obtains its muzzle-light
position from the interpolated `tag_flash`. Ejection and flashlight markers are
exported for subsequent effect integration; Quake brass effects still use their
existing placement.

Model materials use source base/detail textures and Xbox multipurpose channels:
red specular, green self-illumination, blue color change, alpha auxiliary. Diffuse
and emissive layers split base color to avoid counting its illumination twice.
Specular masks drive Quake's specular lighting approximation. Supported unmasked
detail modes are layered; unsupported biased or masked modes are recorded.
Transparent Chicago/generic/glass/meter materials use a recorded first-layer
approximation rather than a missing white texture. Source parameters, clip event
indices, buffer hashes and approximation status are preserved in the manifest.

## Remaining fidelity work

This does not yet establish perfect retail visual parity. Additive movement,
aim and ammunition overlays, animated ammo counters, source sound-event playback,
exact reload-phase synchronization, interrupted transitions and Xbox view/FOV
calibration remain. Halo cubemap reflection, transparent map combiners, color
change and several detail modes require renderer work. The assault-rifle digital
counter currently shows a static source digit texture.

Use `cg_debuganim 1` to report selected clip/frame ranges. `testgun
models/qce/halo/view/shotgun.iqm` inspects a converted model independently of
inventory; enter `testgun` to clear it. These are developer diagnostics.

## Validation

Synthetic tests cover channel defaults and malformed buffers, quaternion signs,
hierarchies, inverse-bind skinning, moving markers, MD3 inspection frames, IQM
header/channel/weight/attachment serialization, material channel semantics and
mask resampling. Asset extraction/profile checks and native/QVM builds are also
required. Local renderer smoke checks exercise idle, fire, reload, melee and
grenade clips and register all eight models. Retail side-by-side validation is
still outstanding.

Format/layout reference: Invader commit
`696830ff80af227e84e7237c2ef26eb2301ed110`; Halo quaternion convention cross-checked
against Reclaimer commit `a0a56ca7e95957e4cef165f606ae6ab6b659113a`.
