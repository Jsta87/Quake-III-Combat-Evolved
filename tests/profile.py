#!/usr/bin/env python3
"""Check malformed data rejection and that generated definitions stay current."""
import copy,importlib.util,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('profile_generator',root/'scripts/generate-profile.py')
generator=importlib.util.module_from_spec(spec);spec.loader.exec_module(generator)
profile=json.loads((root/'data/gameplay-profile.json').read_text())
generator.validate(profile)
for field,value in [('magazine',201),('reload_ms',0),('damage',float('nan')),('damage',1000001),('fire_kind','unknown'),('headshot_mode',3),('shield_multiplier',0),('melee_impact_ms',800),('pellets',65)]:
 bad=copy.deepcopy(profile);bad['weapons']['WP_MACHINEGUN'][field]=value
 try:generator.validate(bad)
 except ValueError:pass
 else:raise AssertionError(f'Accepted invalid {field}={value}')
bad=copy.deepcopy(profile);bad['status']='reference'
try:generator.validate(bad)
except ValueError:pass
else:raise AssertionError('Missing provenance accepted')
bad=copy.deepcopy(profile);bad['grenades'][0]['max_count']=5
try:generator.validate(bad)
except ValueError:pass
else:raise AssertionError('Packed stat overflow accepted')
changed=copy.deepcopy(profile);changed['weapons']['WP_MACHINEGUN']['magazine']=17
assert(generator.render(changed)!=generator.render(profile))
subprocess.run(['python3',str(root/'scripts/generate-profile.py'),'--check'],check=True)
print('PASS: profile validation, numeric limits, timing/policy errors, packed grenade capacity, provenance and generated-source freshness')
