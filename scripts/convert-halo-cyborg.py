#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Owner-local Digsite E3-1999 Cyborg import; no source art is redistributed.
Requires reclaimer==2.11.2 and an existing converted Spartan IQM package.
Digsite's MCC-only usage restriction remains applicable to its assets.
"""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('animation', ROOT / 'scripts/animate-halo-weapons.py')
p = importlib.util.module_from_spec(spec); spec.loader.exec_module(p)

def retail_animation(path, names):
    blob = path.read_bytes()
    if blob[:16] != b'INTERQUAKEMODEL\0': raise ValueError('Invalid retail IQM')
    h = struct.unpack_from('<27I', blob, 16)
    if h[0] != 2 or h[1] != len(blob): raise ValueError('Invalid IQM version/size')
    text = blob[h[4]:h[4]+h[3]]
    def string(o): return text[o:text.index(b'\0', o)].decode()
    joints = [struct.unpack_from('<Ii10f', blob, h[14]+i*48) for i in range(h[13])]
    byname = {string(j[0]):i for i,j in enumerate(joints)}
    mapping = [byname[name] for name in names]
    poses = [struct.unpack_from('<iI20f', blob, h[16]+i*88) for i in range(h[15])]
    channels = struct.unpack_from('<'+'H'*(h[19]*h[20]), blob, h[21])
    frames = []
    for frame in range(h[19]):
        cursor=frame*h[20]; local=[]
        for pose in poses:
            values=list(pose[2:12])
            for k in range(10):
                if pose[1] & (1<<k): values[k]+=channels[cursor]*pose[12+k];cursor+=1
            if max(abs(x-1) for x in values[7:])>.001: raise ValueError('Unsupported scaled skeleton')
            local.append((p.transpose(p.qmatrix(values[3:7])), tuple(values[:3])))
        frames.append([local[i] for i in mapping])
    clips=[]
    for i in range(h[17]):
        name,first,count,fps,flags=struct.unpack_from('<3IfI', blob, h[18]+i*20)
        clips.append({'name':string(name),'first':first,'count':count,'fps':fps})
    hand=joints[byname['tag_hand']]; parent=string(joints[hand[1]][0])
    marker={'name':'tag_hand','node':names.index(parent),'rotation':[-x for x in hand[5:8]]+[hand[8]],'translation':[x/80 for x in hand[2:5]]}
    return frames,clips,marker

def strip_triangles(indices, vertices):
    result=[]
    for i in range(len(indices)-2):
        tri=[indices[i],indices[i+1+(i%2==0)],indices[i+2-(i%2==0)]]
        if min(tri)<0 or len(set(tri))<3:continue
        if max(tri)>=len(vertices):raise ValueError('Cyborg triangle index outside vertex array')
        a,b,c=[vertices[j] for j in tri]
        x=[b['position'][j]-a['position'][j] for j in range(3)];y=[c['position'][j]-a['position'][j] for j in range(3)]
        cross=(x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0])
        # Quake renders the winding opposite to an outward geometric normal.
        if sum(cross[j]*a['normal'][j] for j in range(3))>0:tri[1],tri[2]=tri[2],tri[1]
        result.append(tri)
    return result

def shader_kind(path):
    name=path.replace('\\','/').split('/')[-1].lower()
    if name=='emblem':return None
    if name.endswith('visor'):return 'visor'
    if name.endswith('armor'):return 'armor'
    raise ValueError(f'Unsupported Cyborg shader: {path}')

def convert(tags,retail,output,pk3,variant='99_e3'):
    from reclaimer.hek.defs.mod2 import mod2_def
    from reclaimer.hek.defs.bitm import bitm_def
    if variant not in ('99_e3','00_mac'):raise ValueError('Unsupported Cyborg variant')
    directory=tags/f'digsite/characters/cyborg/{variant}'
    source=directory/f'{variant}.gbxmodel';d=mod2_def.build(filepath=source).data.tagdata
    if not 1<=len(d.nodes.STEPTREE)<=128:raise ValueError('Invalid Cyborg skeleton')
    bones=[{'name':n.name,'parent':n.parent_node} for n in d.nodes.STEPTREE]
    if any(b['parent']>=i or b['parent']< -1 for i,b in enumerate(bones)):raise ValueError('Invalid Cyborg bone hierarchy')
    bind=p.globals_for(bones,[(p.qmatrix(n.rotation[:]),tuple(x*80 for x in n.translation[:])) for n in d.nodes.STEPTREE])
    frames,clips,marker=retail_animation(retail/'models/qce/halo/player/spartan.iqm',[b['name'] for b in bones])
    if len(clips)!=65:raise ValueError('Reconvert the retail Spartan: 65 gameplay/seat clips required')
    files={}
    def write(name,data):
        files[name]=data;dest=output/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
    pixels={}
    for name in ('diff','multi'):
        bitmap=bitm_def.build(filepath=directory/f'bitmaps/{"e3_99" if variant=="99_e3" else "atlas"}_{name}.bitmap').data.tagdata
        image=bitmap.bitmaps.STEPTREE[0];raw=bitmap.processed_pixel_data.data
        rgba=p.a.decode_pixels(raw[image.pixels_offset:],image.width,image.height,image.format.data)
        pixels[name]=(image.width,image.height,rgba)
        if name=='diff':write('textures/qce/cyborg/diff.tga',p.a.tga(image.width,image.height,rgba))
    w,h,diff=pixels['diff'];mw,mh,multi=pixels['multi']
    if (w,h)!=(mw,mh):multi=p.resample_rgba(multi,mw,mh,w,h)
    color=bytearray(diff)
    for i in range(0,len(color),4):color[i+3]=multi[i+2]
    write('textures/qce/cyborg/color.tga',p.a.tga(w,h,color))
    surfaces=[];visors=[]
    for part in d.geometries.STEPTREE[0].parts.STEPTREE:
        if not 0<=part.shader_index<len(d.shaders.STEPTREE):raise ValueError('Invalid Cyborg shader index')
        kind=shader_kind(d.shaders.STEPTREE[part.shader_index].shader.filepath)
        if kind is None:continue
        verts=[]
        for v in part.uncompressed_vertices.STEPTREE:
            values=list(v)
            if not all(math.isfinite(x) for x in values):raise ValueError('Nonfinite Cyborg vertex')
            nodes=[v.node_0_index,v.node_1_index]
            if nodes[0]<0 or nodes[0]>=len(bones) or not 0<=v.node_0_weight<=1:raise ValueError('Invalid skin influence')
            if v.node_0_weight<1 and not 0<=nodes[1]<len(bones):raise ValueError('Invalid second influence')
            verts.append({'position':[x*80 for x in values[:3]],'normal':values[3:6],'uv':[v.u,v.v], 'nodes':nodes,'node0_weight':v.node_0_weight})
        indices=[index for tri in part.triangles.STEPTREE for index in tri]
        pieces=p.a.split_surface(verts,strip_triangles(indices,verts),'qce/cyborg/visor' if kind=='visor' else 'qce/cyborg/armor')
        for piece in pieces:visors.append(kind=='visor');surfaces.append(piece)
    if not surfaces:raise ValueError('No Cyborg mesh')
    posed=p.PoseFrames(surfaces)
    for frame in frames:posed.poses.append([p.skin_matrices(bind,p.globals_for(bones,frame))]*len(surfaces))
    iqm=p.animated_iqm(surfaces,bones,bind,frames,[(bind,list(range(len(bones))))]*len(surfaces),[marker],list(range(len(bones))),80,clips,posed)
    write('models/qce/halo/player/cyborg.iqm',iqm)
    write('models/qce/halo/player/cyborg-source.txt',(variant+'\n').encode())
    def skins(prefix,parts,flags):
        write(prefix+'-body.skin',('\n'.join(f'part{i},{"qce/halo/invisible" if visor else "qce/cyborg/armor"}' for i,visor in enumerate(flags))+'\n').encode())
        write(prefix+'-visor.skin',('\n'.join(f'part{i},{"qce/cyborg/visor" if visor else "qce/halo/invisible"}' for i,visor in enumerate(flags))+'\n').encode())
    skins('models/qce/halo/player/cyborg',surfaces,visors)
    head=[];headvisors=[]
    for surf,visor in zip(surfaces,visors):
        tris=[t for t in surf['triangles'] if all(bones[surf['vertices'][i]['nodes'][0]]['name']=='bip01 head' for i in t)]
        for piece in p.a.split_surface(surf['vertices'],tris,surf['shader']):head.append(piece);headvisors.append(visor)
    if head:
        center=[(min(v['position'][i] for s in head for v in s['vertices'])+max(v['position'][i] for s in head for v in s['vertices']))*.5 for i in range(3)]
        for surf in head:surf['vertices']=[{**v,'position':[v['position'][i]-center[i] for i in range(3)]} for v in surf['vertices']]
        write('models/qce/halo/player/cyborg-head.md3',p.a.md3(head));skins('models/qce/halo/player/cyborg-head',head,headvisors)
    shader='''qce/cyborg/armor
{
 { map textures/qce/cyborg/diff.tga
 rgbGen lightingDiffuse }
 { map textures/qce/cyborg/color.tga
 blendFunc blend
 rgbGen entity }
}
qce/cyborg/visor
{
 { map $whiteimage
 rgbGen entity }
 { map $whiteimage
 blendFunc GL_SRC_ALPHA GL_ONE
 rgbGen lightingDiffuse
 alphaGen const 0.15 }
}
'''
    write('scripts/qce-cyborg.shader',shader.encode())
    report={'variant':variant,'source_model':str(source.relative_to(tags)),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'clips':len(clips),'frames':len(frames),'surfaces':len(surfaces),'triangles':sum(len(s['triangles']) for s in surfaces),'license':'Digsite assets are MCC-only; no redistribution permission is supplied by this converter.'}
    output.mkdir(parents=True,exist_ok=True);(output/'cyborg-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    if pk3:
        pk3.parent.mkdir(parents=True,exist_ok=True)
        with zipfile.ZipFile(pk3,'w',zipfile.ZIP_DEFLATED) as z:
            for name,data in files.items():z.writestr(name,data)
    print(json.dumps(report))

if __name__=='__main__':
    a=argparse.ArgumentParser(description=__doc__);a.add_argument('--tags',type=Path,required=True);a.add_argument('--retail-output',type=Path,required=True);a.add_argument('--output',type=Path,required=True);a.add_argument('--pk3',type=Path);a.add_argument('--variant',choices=('99_e3','00_mac'),default='99_e3');args=a.parse_args()
    convert(args.tags,args.retail_output,args.output,args.pk3,args.variant)
