#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package Halo world weapons, animated Spartans and grenade/footstep audio.
Uses only the owner's local Xbox cache; generated media stays ignored.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('presentation',ROOT/'scripts/animate-halo-weapons.py')
p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
# Stable order consumed by CG_HaloPlayer: root displacement comes from Quake movement.
PLAYER_CLIPS=('stand rifle idle','stand rifle move-front','stand rifle move-back',
 'stand rifle move-left','stand rifle move-right','crouch rifle idle','crouch rifle move-front',
 'stand rifle airborne','stand rifle ar melee','stand rifle throw-grenade','s-kill front chest')

def export(cache,assets,report):
    def write(path,data,meta):
        assets.files.pop(path,None);assets.write(path,data,meta)
    inventory=assets.inventory()
    for (slot,weapon),runtime in zip(inventory.items(),p.WEAPON_NAMES):
        model=next(m for m in weapon['models'] if m['role']=='model')
        source=assets.output/'model-sources'/f'{p.a.safe_name(model["path"])}.json'
        meta=json.loads(source.read_text())
        write(f'models/qce/halo/world/{runtime}.md3',(assets.output/meta['md3']).read_bytes(),{'source':model,'role':'world weapon'})
    deps=[]
    for path in ('characters\\cyborg\\cyborg','weapons\\frag grenade\\frag grenade','weapons\\plasma grenade\\plasma grenade'):
        tag=next(t for t in cache.index if t['path']==path and t['class']=='mode')
        deps.append({'id':tag['id'],'path':path})
    # Convert extra models without discarding the existing weapon shader declarations.
    saved=assets.files.pop('scripts/qce-halo.shader',None)
    for dep in deps:assets.files.pop(f'models/qce/halo/{p.a.safe_name(dep["path"])}.md3',None)
    models=assets.weapon_models({'actors':{'models':deps}})
    assets.files.pop('scripts/qce-halo.shader',None)
    if saved:assets.files['scripts/qce-halo.shader']=saved
    for kind,dep in zip(('spartan','frag','plasma'),deps):
        meta=json.loads((assets.output/f'model-sources/{p.a.safe_name(dep["path"])}.json').read_text())
        if kind!='spartan':
            write(f'models/qce/halo/world/{kind}-grenade.md3',(assets.output/meta['md3']).read_bytes(),{'source':dep})
            continue
        # A source-mesh helmet portrait replaces the old Quake face in HUD/scoreboards.
        head_nodes={i for i,b in enumerate(meta['bones']) if b['name']=='bip01 head'}
        head_surfaces=[]
        for part in meta['parts']:
            def head_vertex(v):return sum(weight for node,weight in zip(v['nodes'],(v['node0_weight'],1-v['node0_weight'])) if node in head_nodes)>0.5
            triangles=[t for t in part['triangles'] if all(head_vertex(part['vertices'][i]) for i in t)]
            if triangles:head_surfaces.extend(p.a.split_surface(part['vertices'],triangles,f'qce/halo/{meta["id"]:08x}/{part["geometry"]}_{part["part"]}'))
        if not head_surfaces:raise p.a.halo.CacheError('No Spartan helmet geometry')
        center=[(min(v['position'][i] for surf in head_surfaces for v in surf['vertices'])+max(v['position'][i] for surf in head_surfaces for v in surf['vertices']))*0.5 for i in range(3)]
        for surf in head_surfaces:
            surf['vertices']=[{**v,'position':[v['position'][i]-center[i] for i in range(3)]} for v in surf['vertices']]
        write('models/qce/halo/player/head.md3',p.a.md3(head_surfaces),{'source':dep,'role':'helmet portrait'})
        graph=cache.tag(dep['path'],'antr');values=graph['values']
        nc,np=struct.unpack_from('<II',cache.data,graph['offset']+104);no=cache.pointer(np,nc*64)
        bones=[]
        for i in range(nc):
            o=no+i*64;parent=struct.unpack_from('<h',cache.data,o+36)[0]
            bones.append({'name':cache.cstring(o,32),'parent':-1 if parent==i else parent})
        names={b['name']:i for i,b in enumerate(bones)};mapping=[names[b['name']] for b in meta['bones']]
        bind=p.globals_for(meta['bones'],[(p.qmatrix(b['rotation']),tuple(x*meta['scale'] for x in b['translation'])) for b in meta['bones']])
        surfaces=[]
        for part in meta['parts']:
            shader=f'qce/halo/{meta["id"]:08x}/{part["geometry"]}_{part["part"]}'
            surfaces.extend(p.a.split_surface(part['vertices'],part['triangles'],shader))
        ac,ap=struct.unpack_from('<II',cache.data,graph['offset']+116);ao=cache.pointer(ap,ac*180)
        local_frames=[];clips=[];poses=p.PoseFrames(surfaces)
        for name in PLAYER_CLIPS:
            ai,anim=next((i,a) for i,a in enumerate(values['animations']) if a['name']==name)
            if anim['type'] not in (0,2) or anim['flags']&1:raise p.a.halo.CacheError('Unsupported Spartan track')
            raw=ao+ai*180;decoded=p.decode_tracks(anim,p.data_blob(cache,raw+140),p.data_blob(cache,raw+160))
            clips.append({'name':name,'first':len(local_frames),'count':len(decoded),'fps':30,'loop_frame':0})
            for local in decoded:
                scaled=[(r,tuple(x*meta['scale'] for x in t)) for r,t in local]
                local_frames.append(scaled);world=p.globals_for(bones,scaled)
                deform=p.skin_matrices(bind,[world[i] for i in mapping]);poses.poses.append([deform]*len(surfaces))
        fallback=p.globals_for(bones,local_frames[0]);byname={b['name']:t for b,t in zip(meta['bones'],bind)}
        bind_world=[byname.get(b['name'],fallback[i]) for i,b in enumerate(bones)]
        # Source hand bone supplies a world weapon attachment; pose follows the authored skeleton.
        hand=next((i for i,b in enumerate(meta['bones']) if b['name']=='bip01 r hand'),None)
        attached=[] if hand is None else [{'name':'tag_hand','node':hand,'rotation':[0,0,0,1],'translation':[0,0,0]}]
        write('models/qce/halo/player/spartan.iqm',p.animated_iqm(surfaces,bones,bind_world,local_frames,[(bind,mapping)]*len(surfaces),attached,mapping,meta['scale'],clips,poses),{'source':dep,'clips':PLAYER_CLIPS})
        write('models/qce/halo/player/spartan.cfg',('\n'.join(f'{c["first"]} {c["count"]} 30 0' for c in clips)+'\n').encode(),{'source':dep})
    # All model materials, including world assets, must remain in the same shader file.
    allmodels={m['id']:m for m in report['models']}
    allmodels.update({m['id']:m for m in models})
    # Hands were added by the animation stage, not the original static manifest.
    for dep in deps+[{'id':t['id'],'path':t['path']} for t in cache.index if t['path']=='characters\\cyborg\\fp\\fp' and t['class']=='mode']:
        allmodels[dep['id']]={'id':dep['id'],'source_mesh':f'model-sources/{p.a.safe_name(dep["path"])}.json'}
    assets.files={k:v for k,v in assets.files.items() if not k.startswith('textures/qce/material/')}
    report['materials']=p.materials(cache,assets,list(allmodels.values()))
    # Xbox multipurpose blue is the armor color mask. Blend entity RGB into those pixels only.
    shaderpath=assets.output/'scripts/qce-halo.shader';shadertext=shaderpath.read_text()
    for material in report['materials']:
        if not material['runtime_shader'].startswith(f'qce/halo/{deps[0]["id"]:08x}/') or 'id' not in material:continue
        tag=cache.tags[material['id']];o=tag['offset'];fields=p.a.LAYOUT['structs']['ShaderModel']['fields']
        ident=struct.unpack_from('<I',cache.data,o+fields['multipurpose map']['offset']+12)[0];multi=cache.tags.get(ident)
        if not multi:continue
        baseid=struct.unpack_from('<I',cache.data,o+fields['base map']['offset']+12)[0];base=cache.tags[baseid]
        w,h,pixels=p.read_tga(assets.output/f'textures/qce/halo/{p.a.safe_name(base["path"])}/000.tga')
        mw,mh,mask=p.read_tga(assets.output/f'textures/qce/halo/{p.a.safe_name(multi["path"])}/000.tga')
        if (mw,mh)!=(w,h):mask=p.resample_rgba(mask,mw,mh,w,h)
        color=bytearray(pixels)
        for i in range(0,len(color),4):color[i+3]=mask[i+2]
        path=f'textures/qce/material/{tag["id"]:08x}_color.tga';write(path,p.a.tga(w,h,color),{'source':multi,'channel':'blue armor color mask'})
        start=shadertext.index(material['runtime_shader']+'\n');end=shadertext.index('\n}\n',start)
        stage=f'\n {{ map {path}\n blendFunc blend\n rgbGen entity\n alphaGen identity\n }}'
        shadertext=shadertext[:end]+stage+shadertext[end:]
    write('scripts/qce-halo.shader',shadertext.encode(),{'generated_for':'weapons, grenades and RGB Spartan armor'})
    aliases={
      'grenade/throw':'sound\\sfx\\weapons\\frag grenade\\throwgren',
      'grenade/frag-explode':'sound\\sfx\\weapons\\frag grenade\\expl',
      'grenade/plasma-explode':'sound\\sfx\\weapons\\plasma grenade\\plasmagrenexpl',
      'grenade/bounce':'sound\\sfx\\weapons\\frag grenade\\bouncedirt',
      'grenade/bounce-metal':'sound\\sfx\\weapons\\frag grenade\\bouncemetal',
      'footstep/normal':'sound\\sfx\\impulse\\footsteps\\cyborg\\grtcement\\run',
      'footstep/metal':'sound\\sfx\\impulse\\footsteps\\cyborg\\grating\\run',
      'footstep/splash':'sound\\sfx\\impulse\\footsteps\\cyborg\\water\\run'}
    for (slot,weapon),runtime in zip(inventory.items(),p.WEAPON_NAMES):
        sourcepath=p.a.halo.WEAPONS[slot]
        if not any(t['path']==sourcepath and t['class']=='weap' for t in cache.index):sourcepath=weapon['model']['path']
        sound=cache.tag(sourcepath,'weap')['values']['pickup sound']
        if sound:aliases[f'pickup/{runtime}']=sound['path']
    aliases['grenade/plasma-loop']='sound\\sfx\\weapons\\plasma grenade\\plasma_projectile'
    for alias,source in aliases.items():
        tag=next(t for t in cache.index if t['path']==source and t['class']=='snd!');records=[r for r in report['assets'] if r.get('id')==tag['id'] and r.get('range')==0]
        for i,record in enumerate(records[:4]):write(f'sound/qce/halo/{alias}{i+1}.wav',(assets.output/record['outputs'][0]).read_bytes(),{'source':source})
    # Dedicated additive billboard avoids uninitialized shaderRGBA (black square).
    write('scripts/qce-halo-effects.shader',b'qce/halo/grenade-explosion\n{\n cull none\n { map gfx/misc/raildisc_mono2.jpg\n blendFunc add\n rgbGen vertex\n alphaGen vertex\n }\n}\n',{'purpose':'temporary additive grenade flash; retail particle system pending'})
    report['world_presentation']={'models':models,'player_clips':PLAYER_CLIPS,'audio':aliases,'limitations':['Root displacement supplied by game movement','Rifle pose shared by weapon classes; aim overlays pending','Grenade flash approximates retail particle effects']}
    report['files']=assets.files
    print('World weapons, Spartan animations/RGB materials and grenade media exported',flush=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('map',type=Path);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--pk3',type=Path,required=True);args=parser.parse_args()
    cache=p.a.halo.XboxMap(args.map);report=json.loads((args.output/'manifest.json').read_text())
    if report['map']['sha256']!=cache.header['sha256']:raise p.a.halo.CacheError('Manifest source mismatch')
    assets=p.a.Assets(cache,args.output);assets.files=report['files'];export(cache,assets,report)
    (args.output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n');assets.package(args.pk3)
if __name__=='__main__':main()
