#!/usr/bin/env python3
"""Synthetic Xbox-cache tests; commercial maps are never needed in CI."""
import copy,importlib.util,json,struct,tempfile,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def module(name,file):
 s=importlib.util.spec_from_file_location(name,ROOT/file);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
extract=module('extract','scripts/extract-halo.py');importer=module('importer','scripts/import-halo-profile.py')
def fixture():
 base=0x803a6000;size=6144;offset=2048;b=bytearray(size)
 b[:4]=b'daeh';b[2044:2048]=b'toof';b[32:36]=b'test';b[64:77]=b'01.10.12.2276'
 struct.pack_into('<5I',b,4,5,size,0,offset,4096)
 struct.pack_into('<4I',b,offset,base+36,0xe1740000,0,1);b[offset+32:offset+36]=b'sgat'
 b[offset+36:offset+40]=b'paew';struct.pack_into('<4I',b,offset+48,0xe1740000,base+68,base+128,0)
 b[offset+68:offset+81]=b'weapons\\test\0'
 layout=extract.LAYOUT['structs']['Weapon']
 for f in layout['fields'].values():
  if f['type']=='TagDependency':struct.pack_into('<I',b,offset+128+f['offset']+12,0xffffffff)
 mag=layout['fields']['magazines']['offset'];struct.pack_into('<2I',b,offset+128+mag,1,base+2048)
 for f in extract.LAYOUT['structs']['WeaponMagazine']['fields'].values():
  if f['type']=='TagDependency':struct.pack_into('<I',b,offset+2048+f['offset']+12,0xffffffff)
 f=extract.LAYOUT['structs']['WeaponMagazine']['fields']['rounds loaded maximum'];struct.pack_into('<h',b,offset+2048+f['offset'],60)
 return b
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.map';b=fixture();p.write_bytes(b[:2048]+zlib.compress(b[2048:]))
 c=extract.XboxMap(p);tag=c.tag('weapons\\test','weap');assert(tag['values']['magazines'][0]['rounds loaded maximum']==60)
 assert(c.header['compressed'] and c.header['tag_count']==1 and len(c.header['sha256'])==64)
 p.write_bytes(b);assert(not extract.XboxMap(p).header['compressed'])
 for mutate in [lambda v:struct.pack_into('<I',v,16,0xffffffff),lambda v:struct.pack_into('<I',v,2048+12,65536),lambda v:struct.pack_into('<I',v,2048+48,0xe1740001),lambda v:v.__setitem__(slice(2044,2048),b'bad!')]:
  bad=bytearray(b);mutate(bad);p.write_bytes(bad)
  try:extract.XboxMap(p)
  except (extract.CacheError,UnicodeError):pass
  else:raise AssertionError('Malformed header/index accepted')
 bad=bytearray(b);f=extract.LAYOUT['structs']['Weapon']['fields']['magazines'];struct.pack_into('<I',bad,2048+128+f['offset']+4,0xffffffff);p.write_bytes(bad)
 try:extract.XboxMap(p).tag('weapons\\test','weap')
 except extract.CacheError:pass
 else:raise AssertionError('Invalid reflexive pointer accepted')
 p.write_bytes(b[:2048]+zlib.compress(b[2048:])[:-3])
 try:extract.XboxMap(p)
 except extract.CacheError:pass
 else:raise AssertionError('Truncated compressed stream accepted')
# Verify the committed numeric import without requiring the uploaded maps.
audit=json.loads((ROOT/'data/halo-import-report.json').read_text());profile=json.loads((ROOT/'data/gameplay-profile.json').read_text())
assert(audit['source_map']['build']=='01.10.12.2276')
assert(profile['weapons']['WP_MACHINEGUN']['magazine']==60 and profile['weapons']['WP_BFG']['magazine']==12)
assert(profile['weapons']['WP_RAILGUN']['headshot_mode']==2 and profile['weapons']['WP_BFG']['headshot_mode']==1)
assert(profile['weapons']['WP_SHOTGUN']['reload_rounds']==1 and profile['weapons']['WP_SHOTGUN']['pellets']==15)
assert(profile['player']=={'health':75,'shield':75,'shield_delay_ms':6000,'shield_recharge_ms':4000})
assert(profile['weapons']['WP_PLASMAGUN']['heat_per_shot']==800 and profile['weapons']['WP_PLASMAGUN']['heat_loss_per_second']==3000)
assert(profile['weapons']['WP_LIGHTNING']['charge_ms']==600 and profile['weapons']['WP_LIGHTNING']['charged_ammo']==55 and profile['weapons']['WP_LIGHTNING']['charged_damage']==70)
assert(profile['status']=='mixed' and audit['remaining'])
print('PASS: compressed/uncompressed Xbox parsing, tag identity, pointer/reflexive bounds, malformed/truncated cache rejection and supported import/mapping values')
