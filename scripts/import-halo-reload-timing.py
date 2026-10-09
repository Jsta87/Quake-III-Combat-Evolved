#!/usr/bin/env python3
"""Derive visible magazine insertion times from locally owned animation tracks.
This is a geometric presentation estimate, not a retail engine notification.
"""
import argparse,importlib.util,json,math,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('presentation',ROOT/'scripts/animate-halo-weapons.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
def timings(cache,graph):
 nc,np=struct.unpack_from('<II',cache.data,graph['offset']+104);no=cache.pointer(np,nc*64)
 names=[cache.cstring(no+i*64,32) for i in range(nc)]
 bones=[]
 for i,name in enumerate(names):
  parent=struct.unpack_from('<h',cache.data,no+i*64+36)[0];bones.append({'name':name,'parent':-1 if parent==i else parent})
 ac,ap=struct.unpack_from('<II',cache.data,graph['offset']+116);ao=cache.pointer(ap,ac*180);result={}
 for i,anim in enumerate(graph['values']['animations']):
  if anim['name'] not in ('first-person reload-full','first-person reload-empty'):continue
  frames=p.decode_tracks(anim,p.data_blob(cache,ao+i*180+140),p.data_blob(cache,ao+i*180+160));frame=len(frames)-1;method='completion fallback (no magazine return track)'
  magazine='frame magazine' if 'frame magazine' in names else 'frame tubes' if 'frame tubes' in names else None
  if magazine and 'frame gun' in names:
   positions=[]
   for local in frames:
    world=p.globals_for(bones,local);r,t=world[names.index('frame gun')];mr,mt=world[names.index(magazine)];positions.append(p.rotate(p.transpose(r),tuple(mt[j]-t[j] for j in range(3))))
   # End pose is the inserted magazine; find its return after greatest separation.
   dist=[math.dist(x,positions[-1]) for x in positions];peak=max(range(len(dist)),key=dist.__getitem__)
   if max(dist)>0.01:
    frame=next((j for j in range(peak+1,len(dist)) if dist[j]<=max(dist)*0.025),frame);method='magazine returns within 2.5% of maximum excursion from seated final pose'
  result[anim['name']]={'frame':frame,'count':len(frames),'ms':round(frame*1000/30),'method':method}
 return result

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('map',type=Path);args=parser.parse_args();cache=p.a.halo.XboxMap(args.map)
 profile=json.loads((ROOT/'data/gameplay-profile.json').read_text());audit={'source_sha256':cache.header['sha256'],'status':'presentation estimate; retail notification parity pending','weapons':{}}
 for slot,w in profile['weapons'].items():
  w['reload_commit_ms']=w['reload_ms'];w['reload_empty_commit_ms']=w['reload_empty_ms']
  source=p.a.halo.WEAPONS.get(slot)
  if not source:continue
  if slot=='WP_GRENADE_LAUNCHER':source='weapons\\needler\\needler'
  tag=cache.tag(source,'weap')['values']['first person animations'];result=timings(cache,cache.tag(tag['path'],'antr'));audit['weapons'][slot]=result
  for key,clip in [('reload_commit_ms','first-person reload-full'),('reload_empty_commit_ms','first-person reload-empty')]:
   selected=result.get(clip) or result.get('first-person reload-full')
   if selected and w['reload_rounds']>1:w[key]=max(1,min(w['reload_empty_ms'] if 'empty' in key else w['reload_ms'],selected['ms']))
 spec=importlib.util.spec_from_file_location('profile_import',ROOT/'scripts/import-halo-profile.py');formatter=importlib.util.module_from_spec(spec);spec.loader.exec_module(formatter)
 (ROOT/'data/gameplay-profile.json').write_text(formatter.profile_json(profile));(ROOT/'data/halo-reload-timing.json').write_text(json.dumps(audit,indent=2)+'\n')
if __name__=='__main__':main()
