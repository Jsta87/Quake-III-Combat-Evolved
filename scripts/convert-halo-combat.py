#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Owner-local Xbox cache exports for combat presentation and aim parameters.
Called by convert-halo-world.py; never downloads or commits game media.
"""
import json,re,struct

def export(p,cache,assets,report,write):
    # Crosshair layouts: build-2276 weapon HUD 0x84 crosshair reflexive;
    # 0x68 element, 0x6c item. Verified against the Halo reference structs.
    def block(offset,size):
        count,ptr=struct.unpack_from('<II',cache.data,offset)
        if count>128:raise p.a.halo.CacheError('Combat reflexive exceeds bounds')
        start=cache.pointer(ptr,count*size) if count else 0
        return [start+i*size for i in range(count)]
    effects=[];crosshairs=[];aim=[]
    for slot,path in p.a.halo.WEAPONS.items():
        if slot=='WP_GRENADE_LAUNCHER' and not any(t['path']==path and t['class']=='weap' for t in cache.index):path='weapons\\needler\\needler'
        values=cache.tag(path,'weap')['values']
        aim.append((slot,[values[k] for k in ('autoaim angle','autoaim range','magnetism angle','magnetism range','deviation angle')],bool(values['weapon flags']&32)))
        hud=cache.tags[values['hud interface']['id']]
        rows=[]
        for element in block(hud['offset']+132,104):
            kind=struct.unpack_from('<h',cache.data,element)[0]
            if kind!=0:continue # aim reticle; zoom captions handled by scope UI
            bitmap=cache.tags[struct.unpack_from('<I',cache.data,element+48)[0]]
            sequences=assets.reflexive('Bitmap','bitmap group sequence',bitmap['offset'],'BitmapGroupSequence')
            for item in block(element+52,108):
                dx,dy,sx,sy=struct.unpack_from('<hhff',cache.data,item)
                fps,sequence,flags=struct.unpack_from('<hhI',cache.data,item+68)
                if flags&2:continue # non-sprite scope decorations
                if not 0<=sequence<len(sequences):raise p.a.halo.CacheError('Invalid reticle sequence')
                sprites=assets.reflexive('BitmapGroupSequence','sprites',sequences[sequence],'BitmapGroupSprite')
                for frame,sprite in enumerate(sprites):
                    bi=struct.unpack_from('<h',cache.data,sprite)[0]
                    left,right,top,bottom,regx,regy=struct.unpack_from('<6f',cache.data,sprite+8)
                    w,h,pixels=p.read_tga(assets.output/f'textures/qce/halo/{p.a.safe_name(bitmap["path"])}/{bi:03}.tga')
                    x0,x1,y0,y1=round(left*w),round(right*w),round(top*h),round(bottom*h)
                    if not 0<=x0<x1<=w or not 0<=y0<y1<=h:raise p.a.halo.CacheError('Invalid reticle crop')
                    crop=bytearray(b''.join(pixels[(y*w+x0)*4:(y*w+x1)*4] for y in range(y0,y1)))
                    # Original reticles are already neutral luminance + alpha.
                    output=f'textures/qce/crosshairs/{slot.lower()}-{len(rows):02}-{frame:02}.tga'
                    write(output,p.a.tga(x1-x0,y1-y0,crop),{'source':bitmap,'sequence':sequence,'sprite':frame})
                    name=f'qce/halo/crosshair/{slot.lower()}-{len(rows):02}-{frame:02}'
                    effects.append(f'{name}\n{{\n nopicmip\n {{ map {output}\n blendFunc blend\n rgbGen vertex\n alphaGen vertex }}\n}}\n')
                    rows.append({'shader':name,'width':(x1-x0)*sx,'height':(y1-y0)*sy,'x':dx-regx*w*sx,'y':dy-regy*h*sy,'flags':flags,'fps':fps,'frame':frame})
        # Preserve source frame sequence for aim-state selection in the runtime.
        crosshairs.append({'weapon':slot,'items':rows})
    write('ui/qce/halo-crosshairs.json',json.dumps(crosshairs,indent=2).encode(),{'source':'weapon HUD aim sprite sequences'})
    # Compact text avoids introducing a JSON parser inside cgame/QVM.
    weapon_ids={'WP_MACHINEGUN':2,'WP_SHOTGUN':3,'WP_GRENADE_LAUNCHER':4,'WP_ROCKET_LAUNCHER':5,'WP_LIGHTNING':6,'WP_RAILGUN':7,'WP_PLASMAGUN':8,'WP_BFG':9}
    lines=[]
    for record in crosshairs:
        if len(record['items'])>32:raise p.a.halo.CacheError('Too many crosshair frames')
        for item in record['items']:lines.append(' '.join(str(x) for x in (weapon_ids[record['weapon']],item['shader'],item['width'],item['height'],item['x'],item['y'],item['flags'],item['frame'])))
    write('ui/qce/halo-crosshairs.cfg',('\n'.join(lines)+'\n').encode(),{'source':'weapon HUD aim sprites'})
    # Keep small, provenance-backed gameplay parameters in git, never media.
    header=['/* Generated from owner-local bloodgulch.map build 2276. Units scaled by 80. */','static const qce_aimdef_t qce_aimdefs[WP_NUM_WEAPONS] = {',' {0}, {0},']
    byslot={s:(v,z) for s,v,z in aim}
    for slot in ('WP_MACHINEGUN','WP_SHOTGUN','WP_GRENADE_LAUNCHER','WP_ROCKET_LAUNCHER','WP_LIGHTNING','WP_RAILGUN','WP_PLASMAGUN','WP_BFG'):
        v,z=byslot[slot];v=[v[0],v[1]*80,v[2],v[3]*80,v[4]]
        header.append(' {'+', '.join(f'{x:.9f}f' for x in v)+f', {int(z)}'+'}, /* '+slot+' */')
    header+=[' {0},','#ifdef MISSIONPACK',' {0}, {0}, {0},','#endif','};']
    (p.ROOT/'engine/code/game/bg_qce_aim.generated.h').write_text('\n'.join(header)+'\n')
    # Plasma has an untextured mgs2 light-volume widget (the map dependency
    # is null in all three retail tags), not a Quake plasma-ball sprite.
    volume=[]
    for source in ('weapons\\plasma rifle\\bolt','weapons\\plasma pistol\\bolt','weapons\\plasma rifle\\charged bolt'):
        tag=next(t for t in cache.index if t['path']==source and t['class']=='mgs2')
        if struct.unpack_from('<I',cache.data,tag['offset']+104)[0]!=0xffffffff:raise p.a.halo.CacheError('Textured light volume unsupported')
        frames=block(tag['offset']+288,176)
        if len(frames)!=1:raise p.a.halo.CacheError('Animated light volume requires another runtime path')
        o=frames[0];offset,exponent,length=struct.unpack_from('<3f',cache.data,o+16)
        r0,r1,radius_exponent=struct.unpack_from('<3f',cache.data,o+60)
        colors=struct.unpack_from('<10f',cache.data,o+104)
        count=struct.unpack_from('<h',cache.data,tag['offset']+110)[0]
        volume.append([count,offset*80,length*80,r0*80,r1*80,radius_exponent,*colors[:8]])
    import math
    for vi,row in enumerate(volume):
        count,offset,length,r0,r1,radius_exponent,*colors=row
        if not 1<=count<=20 or radius_exponent not in (1.5,5.0) or not all(math.isfinite(x) for x in row):raise p.a.halo.CacheError('Unsupported light-volume frame')
        surfaces=[]
        for i in range(count):
            t=i/(count-1) if count>1 else 0;radius=r0+(r1-r0)*t**radius_exponent;x=offset+length*t
            name=f'qce/halo/plasma-volume/{vi}/{i}'
            vertices=[{'position':[x,y,z],'normal':[1,0,0],'uv':uv} for y,z,uv in ((-radius,-radius,[0,0]),(radius,-radius,[1,0]),(radius,radius,[1,1]),(-radius,radius,[0,1]))]
            surfaces.append({'vertices':vertices,'triangles':[[0,1,2],[0,2,3]],'shader':name})
            rgba=[colors[c]*(1-t)+colors[4+c]*t for c in range(4)]
            effects.append(f'{name}\n{{\n cull none\n deformVertexes autosprite\n {{ map textures/qce/combat/volume.tga\n blendFunc GL_SRC_ALPHA GL_ONE\n rgbGen const ( {rgba[1]:g} {rgba[2]:g} {rgba[3]:g} )\n alphaGen const {rgba[0]:g} }}\n}}\n')
        write(f'models/qce/halo/world/plasma-volume-{vi}.md3',p.a.md3(surfaces),{'source':'retail light volume frame','frame':row,'renderer':'camera-facing radial samples, one entity'})
    write('models/qce/halo/world/plasma-volumes.cfg',('\n'.join(' '.join(str(v) for v in row) for row in volume)+'\n').encode(),{'source':'retail untextured light volume frames','scale':80})
    # A radial kernel samples the volume along its axis. It approximates CE's
    # volume rasterizer while preserving tagged radii, colors and length.
    import math
    pixels=bytearray()
    for y in range(32):
        for x in range(32):
            d=((x+0.5-16)/16)**2+((y+0.5-16)/16)**2
            pixels.extend((255,255,255,round(max(0,1-d)**2*255)))
    write('textures/qce/combat/volume.tga',p.a.tga(32,32,pixels),{'procedural':'untextured Halo light-volume radial kernel'})
    effects.append('qce/halo/plasma-volume\n{\n cull none\n { map textures/qce/combat/volume.tga\n blendFunc GL_SRC_ALPHA GL_ONE\n rgbGen entity\n alphaGen entity }\n}\n')
    # Xbox muzzle/smoke bitmaps replace Quake flash geometry and colored rail beams.
    muzzle_sources={2:('flash\\bitmaps\\flash h ar fp',6),3:('flash\\bitmaps\\flash h generic muzzle',10),4:('energy\\bitmaps\\flash c generic muzzle',5),5:('flash\\bitmaps\\flash h generic muzzle',14),6:('energy\\bitmaps\\flash c generic muzzle',5),7:('flash\\bitmaps\\flash h generic muzzle',8),8:('energy\\bitmaps\\flash c generic muzzle',5),9:('flash\\bitmaps\\flash h pistol',7)}
    muzzle_rows=[]
    for weapon,(suffix,radius) in muzzle_sources.items():
        tag=next(t for t in cache.index if t['class']=='bitm' and t['path']=='effects\\particles\\'+suffix)
        source=f'textures/qce/halo/{p.a.safe_name(tag["path"])}'
        if not (assets.output/(source+'/000.tga')).exists():assets.texture(tag)
        sequences=assets.reflexive('Bitmap','bitmap group sequence',tag['offset'],'BitmapGroupSequence')
        sprites=assets.reflexive('BitmapGroupSequence','sprites',sequences[0],'BitmapGroupSprite') if sequences else []
        colors=[0,0,0];weight=0
        for frame,sprite in enumerate(sprites[:16] or [None]):
            bi=struct.unpack_from('<h',cache.data,sprite)[0] if sprite is not None else 0
            w,h,pixels=p.read_tga(assets.output/(source+f'/{bi:03}.tga'))
            if sprite is not None:
                l,rr,t,b=struct.unpack_from('<4f',cache.data,sprite+8);x0,x1,y0,y1=round(l*w),round(rr*w),round(t*h),round(b*h)
                if not 0<=x0<x1<=w or not 0<=y0<y1<=h:raise p.a.halo.CacheError('Invalid muzzle sprite bounds')
                pixels=b''.join(pixels[(y*w+x0)*4:(y*w+x1)*4] for y in range(y0,y1));w,h=x1-x0,y1-y0
            for i in range(0,len(pixels),4):
                a=pixels[i+3];weight+=a
                for c in range(3):colors[c]+=pixels[i+c]*a
            output=f'textures/qce/combat/muzzle-{weapon}-{frame}.tga';write(output,p.a.tga(w,h,pixels),{'source':tag,'sprite':frame})
            effects.append(f'qce/halo/muzzle-{weapon}-{frame}\n{{\n cull none\n {{ map {output}\n blendFunc GL_SRC_ALPHA GL_ONE\n rgbGen vertex\n alphaGen vertex }}\n}}\n')
        peak=max(colors) or 1;rgb=[x/peak for x in colors] if weight else [1,1,1]
        muzzle_rows.append([weapon,min(16,len(sprites)) or 1,radius,*rgb])
    write('models/qce/halo/world/muzzle.cfg',('\n'.join(' '.join(map(str,row)) for row in muzzle_rows)+'\n').encode(),{'source':'Xbox muzzle bitmap sprites','approximation':'sprite radius and 67-ms presentation envelope are calibrated, not full particle simulation'})
    tag=next(t for t in cache.index if t['class']=='bitm' and t['path']=='weapons\\sniper rifle\\bitmaps\\sniper contrail')
    source=f'textures/qce/halo/{p.a.safe_name(tag["path"])}/000.tga'
    if not (assets.output/source).exists():assets.texture(tag)
    w,h,pixels=p.read_tga(assets.output/source);pixels=bytearray(pixels)
    for i in range(0,len(pixels),4):pixels[i:i+3]=bytes([max(pixels[i:i+3])])*3
    write('textures/qce/combat/sniper-smoke.tga',p.a.tga(w,h,pixels),{'source':tag,'neutral_smoke':True})
    effects.append('qce/halo/sniper-smoke\n{\n cull none\n { map textures/qce/combat/sniper-smoke.tga\n blendFunc blend\n rgbGen vertex\n alphaGen vertex }\n}\n')
    paths={'shield-shell':'characters\\cyborg\\bitmaps\\cyborg','shield-break':'effects\\particles\\energy\\bitmaps\\shield jackal depletion','shield-hit':'effects\\particles\\flash\\bitmaps\\flash h shield impact'}
    for alias,path in paths.items():
        tag=next(t for t in cache.index if t['path']==path and t['class']=='bitm')
        src=f'textures/qce/halo/{p.a.safe_name(tag["path"])}/000.tga'
        if not (assets.output/src).exists():assets.texture(tag)
        w,h,pixels=p.read_tga(assets.output/src)
        seq=assets.reflexive('Bitmap','bitmap group sequence',tag['offset'],'BitmapGroupSequence')
        sprites=assets.reflexive('BitmapGroupSequence','sprites',seq[0],'BitmapGroupSprite') if seq else []
        if sprites:
            sprite=sprites[0];bi=struct.unpack_from('<h',cache.data,sprite)[0]
            w,h,pixels=p.read_tga(assets.output/f'textures/qce/halo/{p.a.safe_name(tag["path"])}/{bi:03}.tga')
            l,r,t,b=struct.unpack_from('<4f',cache.data,sprite+8);x0,x1,y0,y1=round(l*w),round(r*w),round(t*h),round(b*h)
            pixels=b''.join(pixels[(y*w+x0)*4:(y*w+x1)*4] for y in range(y0,y1));w,h=x1-x0,y1-y0
        output=f'textures/qce/combat/{alias}.tga';write(output,p.a.tga(w,h,pixels),{'source':tag})
        extra=' tcMod scroll 0.3 0.2\n' if alias=='shield-shell' else ''
        if alias=='shield-shell':
            pixels=bytearray(pixels)
            for i in range(0,len(pixels),4):pixels[i:i+4]=bytes([max(90,max(pixels[i:i+3]))])*3+bytes([255])
            write(output,p.a.tga(w,h,pixels),{'source':tag,'opaque_shield_noise':True})
        effects.append(f'qce/halo/{alias}\n{{\n cull none\n {{ map {output}\n blendFunc GL_SRC_ALPHA GL_ONE\n rgbGen entity\n alphaGen entity\n{extra} }}\n}}\n')
    for alias,path in {'shield-hit':'sound\\sfx\\ui\\shield_hit','shield-break':'sound\\sfx\\ui\\shield_depleted','plasma-flyby':'sound\\sfx\\impulse\\impacts\\plasrif_projectile','needle-flyby':'sound\\sfx\\impulse\\impacts\\needler_projectile','plasma-impact':'sound\\sfx\\weapons\\plasma rifle\\plasmahit','needle-impact':'sound\\sfx\\weapons\\needler\\expl'}.items():
        tag=next(t for t in cache.index if t['class']=='snd!' and t['path']==path)
        records=[r for r in report['assets'] if r.get('id')==tag['id'] and r.get('range')==0]
        if not records:
            before=len(assets.records);assets.sound(tag);records=assets.records[before:];report['assets']+=records
        if records:
            audio,calibration=assets.calibrated_sound(records[0]);write(f'sound/qce/halo/combat/{alias}.wav',audio,{'source':tag,'calibration':calibration})
    effects.append('qce/halo/shield-view\n{\n { map textures/qce/combat/shield-hit.tga\n blendFunc GL_SRC_ALPHA GL_ONE\n rgbGen vertex\n alphaGen vertex }\n}\n')
    # Split visor into a second skin pass to give it an independent RGB channel.
    meta=json.loads((assets.output/'model-sources'/f'{p.a.safe_name("characters\\cyborg\\cyborg")}.json').read_text())
    skin=[];visor=[];surface=0
    shadertext=(assets.output/'scripts/qce-halo.shader').read_text()
    for part in meta['parts']:
        name=f'qce/halo/{meta["id"]:08x}/{part["geometry"]}_{part["part"]}'
        pieces=p.a.split_surface(part['vertices'],part['triangles'],name)
        isvisor=part['shader'].endswith('\\visor')
        if isvisor:
            start=shadertext.index(name+'\n');end=shadertext.index('\n}\n',start)+3
            text=shadertext[start:end].replace(name,name+'-rgb',1)
            # Greyscale the original colored visor before applying the chosen RGB.
            for src in set(re.findall(r'\bmap (textures/[^\s]+)',text)):
                w,h,pixels=p.read_tga(assets.output/src);pixels=bytearray(pixels)
                for i in range(0,len(pixels),4):pixels[i:i+3]=bytes([max(pixels[i:i+3])])*3
                output=src[:-4]+'_visor.tga';write(output,p.a.tga(w,h,pixels),{'source':src,'visor_luminance':True});text=text.replace(src,output)
            text=re.sub(r'rgbGen (?:lightingDiffuse|identity|const \([^\n]*\))','rgbGen entity',text)
            effects.append(text)
        for piece in pieces:
            skin.append(f'part{surface},{"qce/halo/invisible" if isvisor else name}')
            visor.append(f'part{surface},{name+"-rgb" if isvisor else "qce/halo/invisible"}');surface+=1
    effects.append('qce/halo/invisible\n{\n { map $whiteimage\n blendFunc blend\n alphaGen const 0 }\n}\n')
    write('models/qce/halo/player/body.skin',('\n'.join(skin)+'\n').encode(),{'role':'body without visor'})
    write('models/qce/halo/player/visor.skin',('\n'.join(visor)+'\n').encode(),{'role':'independent visor RGB'})
    write('scripts/qce-halo-combat.shader','\n'.join(effects).encode(),{'source':'Xbox combat effects, reticles and visor'})
    report['combat_presentation']={'crosshairs':crosshairs,'aim':aim,'textures':paths,'plasma_volumes':volume,'limitations':['Shield shell uses the source noise texture with additive stages; Halo GPU combiners are approximated','Autoaim pill uses game collision dimensions, not animated pelvis/head matrices','Controller stick magnetism remains separate from projectile autoaim']}

if __name__=='__main__':
    import argparse,importlib.util
    from pathlib import Path
    root=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('map',type=Path);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--pk3',type=Path,required=True);args=parser.parse_args()
    spec=importlib.util.spec_from_file_location('presentation',root/'scripts/animate-halo-weapons.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
    cache=p.a.halo.XboxMap(args.map);report=json.loads((args.output/'manifest.json').read_text())
    if report['map']['sha256']!=cache.header['sha256']:raise p.a.halo.CacheError('Manifest source mismatch')
    assets=p.a.Assets(cache,args.output);assets.files=report['files']
    def write(path,data,meta):assets.files.pop(path,None);assets.write(path,data,meta)
    export(p,cache,assets,report,write);report['files']=assets.files
    (args.output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n');assets.package(args.pk3)
