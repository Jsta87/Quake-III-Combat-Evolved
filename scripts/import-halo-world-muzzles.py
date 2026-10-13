#!/usr/bin/env python3
"""Extract numeric world-gun muzzle positions; no source art is redistributed."""
import argparse
import importlib.util
import json
import math
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('animation',ROOT/'scripts/animate-halo-weapons.py')
p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
def extract(path):
 cache=p.a.halo.XboxMap(path);assets=p.a.Assets(cache,ROOT/'assets/halo/converted/bloodgulch')
 result={}
 for slot,weapon in assets.inventory().items():
  tag=cache.tags[weapon['model']['id']];bones=[]
  for n in assets.reflexive('Model','nodes',tag['offset'],'ModelNode'):
   bones.append({'name':cache.cstring(n,32),'parent':struct.unpack_from('<h',cache.data,n+36)[0],'translation':struct.unpack_from('<3f',cache.data,n+40),'rotation':struct.unpack_from('<4f',cache.data,n+52)})
  world=p.globals_for(bones,[(p.qmatrix(b['rotation']),tuple(x*80 for x in b['translation'])) for b in bones])
  markers=p.markers(cache,assets,tag);tags=p.pose_tags(markers,world,80)
  if 'tag_flash' not in tags:raise ValueError(f'{slot} lacks primary trigger marker')
  result[slot]=list(tags['tag_flash'][1])
 return {'source_map':cache.header,'halo_to_quake_scale':80,'muzzles':result}
def render(data):
 lines=['/* Generated numeric Xbox world-model trigger markers. */','#ifndef BG_QCE_WORLD_MUZZLES_H','#define BG_QCE_WORLD_MUZZLES_H','static const vec3_t qceWorldMuzzles[WP_NUM_WEAPONS] = {',' {0,0,0}, {0,0,0},']
 for slot in ('WP_MACHINEGUN','WP_SHOTGUN','WP_GRENADE_LAUNCHER','WP_ROCKET_LAUNCHER','WP_LIGHTNING','WP_RAILGUN','WP_PLASMAGUN','WP_BFG'):
  values=data['muzzles'][slot]
  if len(values)!=3 or not all(math.isfinite(x) for x in values):raise ValueError('Invalid muzzle position')
  lines.append(' {'+','.join(f'{x:.9f}f' for x in values)+'}, /* '+slot+' */')
 return '\n'.join(lines+['};','#endif',''])
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('map',nargs='?',type=Path);a.add_argument('--check',action='store_true');args=a.parse_args()
 audit=ROOT/'data/halo-world-muzzles.json';header=ROOT/'engine/code/game/bg_qce_world_muzzles.generated.h'
 data=extract(args.map) if args.map else json.loads(audit.read_text());text=render(data)
 if args.check:
  if header.read_text()!=text:raise SystemExit('World muzzle header is stale')
 else:audit.write_text(json.dumps(data,indent=2)+'\n');header.write_text(text)
