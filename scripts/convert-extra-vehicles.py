#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Owner-local Ghost/Scorpion models, moving parts, audio and numeric handling tags."""
import argparse,importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('p',ROOT/'scripts/animate-halo-weapons.py');p=importlib.util.module_from_spec(s);s.loader.exec_module(p)
def convert(path,output,pk3,header=None):
 c=p.a.halo.XboxMap(path);a=p.a.Assets(c,output);profile=[];reports=[];textures=set();shader_text=[]
 for name,tagpath,physics in [('ghost','vehicles\\ghost\\ghost_mp','vehicles\\ghost\\ghost'),('scorpion','vehicles\\scorpion\\scorpion_mp','vehicles\\scorpion\\scorpion')]:
  tag=next(t for t in c.index if t['class']=='vehi' and t['path']==tagpath);o=tag['offset'];model=c.tags[struct.unpack_from('<I',c.data,o+52)[0]]
  refs=a.reflexive('Model','shaders',model['offset'],'ModelShaderReference')
  for ref in refs:
   shader=c.tags.get(struct.unpack_from('<I',c.data,ref+12)[0])
   if shader and shader['class']=='soso':
    for field in ('base map','multipurpose map','reflection cube map'):
     bit=c.tags.get(struct.unpack_from('<I',c.data,a.field('ShaderModel',field,shader['offset'])+12)[0])
     if bit and bit['id'] not in textures:a.texture(bit);textures.add(bit['id'])
  a.files.pop('scripts/qce-halo.shader',None)
  models=a.weapon_models({name:{'models':[model]}});meta=json.loads((output/'model-sources'/f'{p.a.safe_name(model["path"])}.json').read_text())
  shader_text.append((output/'scripts/qce-halo.shader').read_text())
  # Split articulated turret joints; model vertices are exported in bind world space.
  bind=p.globals_for(meta['bones'],[(p.qmatrix(b['rotation']),tuple(x*80 for x in b['translation'])) for b in meta['bones']]);groups={'body':[],'turret':[],'gun':[]};pivots={k:(0,0,0) for k in groups}
  names=[b['name'] for b in meta['bones']];turret=next((i for i,n in enumerate(names) if 'turret' in n.lower()),-1);gun=next((i for i,n in enumerate(names) if any(x in n.lower() for x in ('cannon','barrel'))),-1)
  if turret>=0:pivots['turret']=bind[turret][1]
  if gun>=0:pivots['gun']=bind[gun][1]
  def group(node):
   while node>=0:
    if node==gun:return 'gun'
    if node==turret:return 'turret'
    node=meta['bones'][node]['parent']
   return 'body'
  for part in meta['parts']:
   shader=f'qce/halo/{meta["id"]:08x}/{part["geometry"]}_{part["part"]}'
   for key in groups:
    tris=[t for t in part['triangles'] if group(part['vertices'][t[0]]['nodes'][0])==key]
    verts=[{**v,'position':[v['position'][i]-pivots[key][i] for i in range(3)]} for v in part['vertices']]
    groups[key]+=p.a.split_surface(verts,tris,shader)
  for key,surfaces in groups.items():
   if surfaces:a.write(f'models/qce/halo/{name}/{key}.md3',p.a.md3(surfaces),{'vehicle':name,'part':key})
  positions=[v['position'] for part in meta['parts'] for v in part['vertices']];bounds=[[min(v[i] for v in positions) for i in range(3)],[max(v[i] for v in positions) for i in range(3)]]
  marker_positions={}
  for marker in a.reflexive('Model','markers',model['offset'],'ModelMarker'):
   markername=c.cstring(marker,32)
   instances=a.reflexive('ModelMarker','instances',marker,'ModelMarkerInstance')
   chosen=[instance for instance in instances if c.data[instance+1]==0]
   if chosen:
    instance=chosen[0];node=c.data[instance+2];translation=struct.unpack_from('<3f',c.data,instance+4)
    marker_positions[markername]=p.add(p.rotate(bind[node][0],tuple(x*80 for x in translation)),bind[node][1])
  source=next(t for t in c.index if t['class']=='phys' and t['path']==physics);po=source['offset'];values=struct.unpack_from('<23f',c.data,po);count,ptr=struct.unpack_from('<II',c.data,po+116);start=c.pointer(ptr,count*128);points=[]
  if not 1<=count<=32 or values[2]<=0 or values[9]<=0 or min(values[20:23])<=0 or values[11]<=values[12]:raise p.a.halo.CacheError("Invalid vehicle physics profile")
  for i in range(count):
   q=start+i*128;powered,node=struct.unpack_from('<hh',c.data,q+32);mass=struct.unpack_from('<f',c.data,q+44)[0];pos=struct.unpack_from('<3f',c.data,q+56);kind=struct.unpack_from('<h',c.data,q+92)[0];pa,pe,radius=struct.unpack_from('<3f',c.data,q+96)
   points.append([list(x*80 for x in pos),mass/values[2],radius*80,pa,pe,powered,kind,node])
  power_count,power_ptr=struct.unpack_from('<II',c.data,po+104);power_start=c.pointer(power_ptr,power_count*128);powers=[struct.unpack_from('<I6f',c.data,power_start+i*128+32) for i in range(power_count)]
  numeric={'powers':powers,'name':name,'speed':[struct.unpack_from('<f',c.data,o+i)[0] for i in (760,764,768,772)],'physics':list(values),'points':points,'bounds':bounds,'pivots':pivots,'bones':names,'markers':marker_positions};reports.append(numeric)
  # Engine and firing sounds are converted from the cache, never downloaded.
  for role,fragment in [('engine','engine'),('fire','fire')]:
   sound=next((t for t in c.index if t['class']=='snd!' and ((name in t['path'] or (name=='scorpion' and 'tank' in t['path'])) and (fragment in t['path'] or (role=='engine' and t['path'].endswith('_lp'))))),None)
   if sound:
    before=len(a.records);a.sound(sound);records=a.records[before:]
    if records:
     record=records[0];sourcePath=output/record['outputs'][0];a.write(f'sound/qce/halo/{name}/{role}.wav',sourcePath.read_bytes(),{'source':sound['path']})
  weapon=c.tag('vehicles\\'+name+'\\'+('ghost gun' if name=='ghost' else 'scorpion cannon'),'weap')['values']
  for trigger_index,trigger in enumerate(weapon['triggers']):
   for effect in trigger['firing effects']:
    dep=effect['firing effect']
    if not dep or dep['class']!='effe':continue
    graph=c.dependency(dep)
    for event in graph['values']['events']:
     for part in event['parts']:
      dep=part['type']
      if dep and dep['class']=='snd!':
       before=len(a.records);a.sound(c.tags[dep['id']])
       if len(a.records)>before:
        audio,calibration=a.calibrated_sound(a.records[before]);role='fire' if trigger_index==0 else 'secondary'
        a.write(f'sound/qce/halo/{name}/{role}.wav',audio,{'source':dep,'calibration':calibration})
  profile.append((name,values,points))
 # Generate source-derived physics profiles, same mass-point formulation as Warthog.
 if header:
  def f(x):return f'{x:.9f}f'
  lines=['/* SPDX-License-Identifier: GPL-3.0-only',' * Numeric Xbox 2276 Ghost/Scorpion phys, vehi and model marker values. */']
  for name,v,points in profile:
   cap=name.title();lines.append(f'static const qce_hogpoint_t qce{cap}Points[]={{')
   for pos,m,r,pa,pe,powered,kind,node in points:lines.append(' {{'+','.join(f(x) for x in pos)+'},'+','.join(f(x) for x in (m,r,pa,pe))+','+','.join(str(x) for x in (powered,kind,node))+'},')
   lines.append('};');lines.append(f'static const qce_vehiclephysics_t qce{cap}Physics={{'+','.join(f(x) for x in (v[2],v[9]*80,v[10]*30,v[8]*30,v[18]*30,v[7],v[12],v[11]))+',{'+','.join(f(x*80) for x in v[3:6])+'},{'+','.join(f(x*6400/v[2]) for x in v[20:23])+'},qce'+cap+'Points,'+str(len(points))+'};')
  for report in reports:
   if report['name']=='ghost':
    lines.append('static const float qceGhostAntigrav[][6]={'+','.join('{'+','.join(f(x) for x in power[1:])+'}' for power in report['powers'])+'};')
   for key,value in {**report['markers'],**{f'pivot {k}':v for k,v in report['pivots'].items()}}.items():
    if key in ('driver','passenger lb','passenger lf','passenger rb','passenger rf','primary trigger','secondary trigger','pivot turret','pivot gun'):
     symbol='qce'+report['name'].title()+''.join(x.title() for x in key.split())
     lines.append('static const vec3_t '+symbol+'={'+','.join(f(x) for x in value)+'};')
  header.write_text('\n'.join(lines)+'\n')
 # weapon_models writes diffuse declarations; retain that shader under its own name.
 shader=output/'scripts/qce-halo.shader'
 if shader.exists():a.write('scripts/qce-extra-vehicles.shader','\n'.join(shader_text).encode(),{})
 a.files.pop('scripts/qce-halo.shader',None);a.write('extra-vehicles-manifest.json',(json.dumps({'source':c.header,'vehicles':reports},indent=2)+'\n').encode(),{});a.package(pk3)
 print(json.dumps({'vehicles':[(r['name'],r['bones'],r['bounds']) for r in reports]}))
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('map',type=Path);a.add_argument('--output',type=Path,required=True);a.add_argument('--pk3',type=Path,required=True);a.add_argument('--header',type=Path);args=a.parse_args();convert(args.map,args.output,args.pk3,args.header)
