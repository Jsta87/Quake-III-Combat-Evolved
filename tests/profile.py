#!/usr/bin/env python3
"""Check malformed data rejection and that generated definitions stay current."""
import copy,importlib.util,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('profile_generator',root/'scripts/generate-profile.py')
generator=importlib.util.module_from_spec(spec);spec.loader.exec_module(generator)
profile=json.loads((root/'data/gameplay-profile.json').read_text())
generator.validate(profile)
for field,value in [('magazine',10001),('reload_ms',0),('damage',float('nan')),('damage',1000001),('fire_kind','unknown'),('headshot_mode',3),('shield_multiplier',0),('melee_impact_ms',2000),('pellets',65),('tracking_radians',7),('combine_count',65),('attachment_ms',750),('ready_ms',0),('reload_empty_ms',0),('charged_battery_cost',1000001),('zoom_levels',3),('damage_maximum',0)]:
 bad=copy.deepcopy(profile);bad['weapons']['WP_MACHINEGUN'][field]=value
 try:generator.validate(bad)
 except ValueError:pass
 else:raise AssertionError(f'Accepted invalid {field}={value}')
for field,value in [('tracking_radians',7),('combine_count',65),('attachment_ms',600001),('projectile_speed',0),('combine_radius',0)]:
 bad=copy.deepcopy(profile);bad['weapons']['WP_GRENADE_LAUNCHER'][field]=value
 try:generator.validate(bad)
 except ValueError:pass
 else:raise AssertionError(f'Accepted invalid needle {field}={value}')
for weapon,field,value in [('WP_MACHINEGUN','spread_max',0),('WP_SHOTGUN','falloff_end',0),('WP_PLASMAGUN','projectile_final_speed',999999),('WP_PLASMAGUN','heat_per_shot',10001),('WP_PLASMAGUN','heat_recovery',10000),('WP_PLASMAGUN','heat_loss_per_second',0),('WP_PLASMAGUN','heat_age_penalty',1000),('WP_LIGHTNING','charge_ms',600001),('WP_LIGHTNING','charged_speed',0),('WP_LIGHTNING','charged_ammo',501),('WP_LIGHTNING','charged_shield_multiplier',0)]:
 bad=copy.deepcopy(profile);bad['weapons'][weapon][field]=value
 try:generator.validate(bad)
 except ValueError:pass
 else:raise AssertionError(f'Accepted invalid {weapon}.{field}={value}')
bad=copy.deepcopy(profile);bad['status']='reference';bad['provenance']['source_maps']=[]
try:generator.validate(bad)
except ValueError:pass
else:raise AssertionError('Missing provenance accepted')
bad=copy.deepcopy(profile);bad['grenades'][0]['max_count']=5
try:generator.validate(bad)
except ValueError:pass
else:raise AssertionError('Packed stat overflow accepted')
bad=copy.deepcopy(profile);bad['materials']['WP_MACHINEGUN'][0]['response']=5
try:generator.validate(bad)
except ValueError:pass
else:raise AssertionError('Invalid material enum accepted')
changed=copy.deepcopy(profile);changed['weapons']['WP_MACHINEGUN']['magazine']=17
assert(generator.render(changed)!=generator.render(profile))
subprocess.run(['python3',str(root/'scripts/generate-profile.py'),'--check'],check=True)
print('PASS: profile validation, numeric limits, timing/policy errors, packed grenade capacity, provenance and generated-source freshness')
