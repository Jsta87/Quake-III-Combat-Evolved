#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bake build-2276 Halo weapon/Spartan arm tracks, tags and materials to skeletal IQM.
Run after convert-halo-assets.py. Original art and generated files remain local.
"""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('assets',ROOT/'scripts/convert-halo-assets.py')
a = importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
ACTIONS = ('idle','fire','ready','putaway','reloadfull','reloadempty','melee','grenade','overheat','charge','chargedfire','recover','reloadenter','reloadexit','reloadexitempty','chargeenter','hotidle','chargedhot')
NAMES = {'idle':'idle','fire':'fire-1','ready':'ready','putaway':'put-away','reloadfull':'reload-full','reloadempty':'reload-empty','melee':'melee','grenade':'throw-grenade','overheat':'overheating','charge':'overcharged','chargedfire':'fire-2','recover':'o-h-exit','reloadenter':'enter','reloadexit':'exit-full','reloadexitempty':'exit-empty','chargeenter':'overcharged','hotidle':'overheated','chargedhot':'o-h-s-enter'}
WEAPON_NAMES = ('machinegun','shotgun','rocket','railgun','plasma','lightning','bfg','grenade')
IDENTITY = ((1.,0.,0.),(0.,1.,0.),(0.,0.,1.))


def qmatrix(q):
    x,y,z,w = q;d = sum(v*v for v in q)
    if not math.isfinite(d) or d<1e-12:raise a.halo.CacheError('Invalid animation quaternion')
    # Halo/JMA's quaternion convention is the transpose of the common
    # column-vector formula. Keep that convention for both bind and pose data.
    return ((1-2*(y*y+z*z)/d,2*(x*y+z*w)/d,2*(x*z-y*w)/d),
            (2*(x*y-z*w)/d,1-2*(x*x+z*z)/d,2*(y*z+x*w)/d),
            (2*(x*z+y*w)/d,2*(y*z-x*w)/d,1-2*(x*x+y*y)/d))


def mul(r,s):return tuple(tuple(sum(r[i][k]*s[k][j] for k in range(3)) for j in range(3)) for i in range(3))
def rotate(r,p):return tuple(sum(r[i][j]*p[j] for j in range(3)) for i in range(3))
def add(p,q):return tuple(p[i]+q[i] for i in range(3))
def transpose(r):return tuple(zip(*r))


def globals_for(bones,local):
    result = [None]*len(bones);visiting = set()
    def resolve(i):
        if result[i] is not None:return result[i]
        if i in visiting:raise a.halo.CacheError('Cyclic skeleton')
        visiting.add(i);r,t = local[i];parent = bones[i]['parent']
        if parent>=0:
            if parent>=len(bones):raise a.halo.CacheError('Skeleton parent outside graph')
            pr,pt = resolve(parent);r,t = mul(pr,r),add(rotate(pr,t),pt)
        result[i] = r,t;visiting.remove(i);return result[i]
    for i in range(len(bones)):resolve(i)
    return result


def data_blob(cache,offset):
    cache.check(offset,20)
    size,flags,file_offset,pointer,ident = struct.unpack_from('<5I',cache.data,offset)
    if not size:return b''
    start = cache.pointer(pointer,size)
    return bytes(cache.data[start:start+size])


def decode_tracks(animation,default,frames):
    nodes = animation['node count'];count = animation['frame count']
    if not 1<=nodes<=64 or not 1<=count<=1024:raise a.halo.CacheError('Animation size limit')
    if animation['flags']&3 or animation['offset to compressed data']:
        raise a.halo.CacheError('Compressed/world-relative animation tracks are not supported')
    masks = []
    for field in ('node rotation flag data','node transform flag data','node scale flag data'):
        low,high = animation[field];masks.append(low|(high<<32))
    if any(mask>>nodes for mask in masks):raise a.halo.CacheError('Animation mask outside node count')
    fmts = ('<4h','<3f','<f');sizes = (8,12,4)
    frame_size = sum(sizes[k] for i in range(nodes) for k in range(3) if masks[k]&(1<<i))
    if frame_size != animation['frame size'] or len(frames)!=frame_size*count or len(default)!=nodes*24-frame_size:
        raise a.halo.CacheError('Animation buffer length mismatch')
    defaults = [[None]*3 for _ in range(nodes)];cursor = 0
    for i in range(nodes):
        for k in range(3):
            if not masks[k]&(1<<i):
                defaults[i][k] = struct.unpack_from(fmts[k],default,cursor);cursor += sizes[k]
    result = [];cursor = 0
    for f in range(count):
        local = []
        for i in range(nodes):
            fields = []
            for k in range(3):
                if masks[k]&(1<<i):
                    value = struct.unpack_from(fmts[k],frames,cursor);cursor += sizes[k]
                else:value = defaults[i][k]
                fields.append(value)
            q,t,scale = fields;r = qmatrix(q);scale = scale[0]
            if not math.isfinite(scale) or not 0<scale<=16 or not all(math.isfinite(x) for x in t):
                raise a.halo.CacheError('Invalid animation transform/scale')
            local.append((tuple(tuple(x*scale for x in row) for row in r),t))
        result.append(local)
    return result


def skin_vertex(vertex,bind,pose):
    result = [0.,0.,0.];normal = [0.,0.,0.]
    weight = vertex['node0_weight']
    if not 0<=weight<=1:raise a.halo.CacheError('Invalid vertex weight')
    contributions = list(zip(vertex['nodes'],(weight,1-weight)))
    if all(n<0 for n,w in contributions):return vertex
    for node,w in contributions:
        if w==0:continue
        if not 0<=node<len(bind):raise a.halo.CacheError('Missing weighted skin node')
        br,bt = bind[node];pr,pt = pose[node]
        # Mesh points are in model bind space. Undo bind before applying pose.
        inv = transpose(br);local = rotate(inv,tuple(vertex['position'][i]-bt[i] for i in range(3)))
        p = add(rotate(pr,local),pt);n = rotate(pr,rotate(inv,vertex['normal']))
        for i in range(3):result[i] += p[i]*w;normal[i] += n[i]*w
    return {**vertex,'position':result,'normal':normal}


def skin_matrices(bind,pose):
    result = []
    for (br,bt),(pr,pt) in zip(bind,pose):
        r = mul(pr,transpose(br));shift = rotate(r,bt)
        result.append((tuple(v for row in r for v in row),tuple(pt[i]-shift[i] for i in range(3))))
    return result


def fast_skin(vertex,matrices):
    p = vertex['position'];n = vertex['normal'];weight = vertex['node0_weight'];nodes = vertex['nodes']
    if all(i<0 for i in nodes):return {'position':p,'normal':n}
    position = [0.,0.,0.];normal = [0.,0.,0.]
    for node,w in zip(nodes,(weight,1-weight)):
        if not w:continue
        if not 0<=node<len(matrices) or not 0<=w<=1:raise a.halo.CacheError('Invalid skin influence')
        r,t = matrices[node]
        for i in range(3):
            j = i*3
            position[i] += (r[j]*p[0]+r[j+1]*p[1]+r[j+2]*p[2]+t[i])*w
            normal[i] += (r[j]*n[0]+r[j+1]*n[1]+r[j+2]*n[2])*w
    return {'position':position,'normal':normal}


class PoseFrames:
    def __init__(self,surfaces):self.surfaces = surfaces;self.poses = []
    def __len__(self):return len(self.poses)
    def __iter__(self):
        for pose in self.poses:
            yield [[fast_skin(v,matrix) for v in surface['vertices']] for surface,matrix in zip(self.surfaces,pose)]


def animated_md3(surfaces,frames,tags):
    count = len(frames);tag_names = tuple(tags[0]) if tags else ()
    if not 1<=count<=1024 or len(tag_names)>16 or not 0<len(surfaces)<=32:raise a.halo.CacheError('MD3 limits')
    frame_data = bytearray();tag_data = bytearray();body = bytearray();xyz_buffers = [bytearray() for _ in surfaces]
    for fi,frame in enumerate(frames):
        positions = [v['position'] for surface in frame for v in surface]
        mins = [min(p[i] for p in positions) for i in range(3)];maxs = [max(p[i] for p in positions) for i in range(3)]
        radius = max(math.sqrt(sum(x*x for x in p)) for p in positions)
        frame_data.extend(struct.pack('<10f16s',*(mins+maxs+[0,0,0,radius]),f'frame{fi}'.encode()))
        for si,vertices in enumerate(frame):
            if len(vertices)!=len(surfaces[si]['vertices']):raise a.halo.CacheError('MD3 vertex topology changes')
            for v in vertices:
                p = [round(x*64) for x in v['position']]
                if any(not -32768<=x<=32767 for x in p):raise a.halo.CacheError('Animated MD3 position overflow')
                x,y,z = v['normal'];length = math.sqrt(x*x+y*y+z*z)
                lat = round(math.atan2(y,x)*255/(2*math.pi))&255
                lng = round(math.acos(max(-1,min(1,z/length if length else 1)))*255/(2*math.pi))&255
                xyz_buffers[si].extend(struct.pack('<3hH',*p,(lat<<8)|lng))
        if tags:
            if tuple(tags[fi])!=tag_names:raise a.halo.CacheError('Attachment topology changes')
            for name in tag_names:
                r,t = tags[fi][name]
                # MD3 stores three axis vectors (matrix columns).
                tag_data.extend(struct.pack('<64s12f',name.encode(),*t,*(v for axis in transpose(r) for v in axis)))
    for si,surface in enumerate(surfaces):
        verts = surface['vertices'];tris = surface['triangles']
        if len(verts)>=1000 or len(tris)*3>=6000:raise a.halo.CacheError('MD3 renderer surface limit')
        triangles = b''.join(struct.pack('<3i',*t) for t in tris)
        shader = struct.pack('<64si',surface['shader'].encode(),0)
        st = b''.join(struct.pack('<2f',*v['uv']) for v in verts)
        xyz = xyz_buffers[si]
        to = 108;so = to+len(triangles);sto = so+len(shader);xo = sto+len(st);end = xo+len(xyz)
        body.extend(struct.pack('<4s64s10i',b'IDP3',f'part{si}'.encode(),0,count,1,len(verts),len(tris),to,so,sto,xo,end))
        body.extend(triangles+shader+st+xyz)
    tags_offset = 108+len(frame_data);surfaces_offset = tags_offset+len(tag_data);end = surfaces_offset+len(body)
    return struct.pack('<4si64s9i',b'IDP3',15,b'halo_weapon',0,count,len(tag_names),len(surfaces),0,108,tags_offset,surfaces_offset,end)+frame_data+tag_data+body


def transform_channels(transform):
    r,t = transform
    scale = math.sqrt(sum(r[i][0]**2 for i in range(3)))
    m = [[x/scale for x in row] for row in r]
    # Standard column-vector quaternion, matching IQM JointToMatrix.
    k = max(range(4),key=lambda i: (1+m[0][0]+m[1][1]+m[2][2],1+m[0][0]-m[1][1]-m[2][2],1-m[0][0]+m[1][1]-m[2][2],1-m[0][0]-m[1][1]+m[2][2])[i])
    if k==0:
        w=math.sqrt(max(0,1+sum(m[i][i] for i in range(3))))/2;d=4*w
        q=((m[2][1]-m[1][2])/d,(m[0][2]-m[2][0])/d,(m[1][0]-m[0][1])/d,w)
    else:
        i=k-1;j=(i+1)%3;l=(i+2)%3
        v=math.sqrt(max(0,1+m[i][i]-m[j][j]-m[l][l]))/2;d=4*v;q=[0.]*4
        q[i]=v;q[j]=(m[i][j]+m[j][i])/d;q[l]=(m[i][l]+m[l][i])/d;q[3]=(m[l][j]-m[j][l])/d
    return list(t)+list(q)+[scale]*3


def mesh_tangents(surface):
    vertices=surface['vertices'];tangents=[[0.]*3 for v in vertices];bitangents=[[0.]*3 for v in vertices]
    for triangle in surface['triangles']:
        v0,v1,v2=[vertices[i] for i in triangle]
        e1=[v1['position'][i]-v0['position'][i] for i in range(3)];e2=[v2['position'][i]-v0['position'][i] for i in range(3)]
        u1,v1uv=[v1['uv'][i]-v0['uv'][i] for i in range(2)];u2,v2uv=[v2['uv'][i]-v0['uv'][i] for i in range(2)]
        determinant=u1*v2uv-u2*v1uv
        if abs(determinant)<1e-12:continue
        t=[(e1[i]*v2uv-e2[i]*v1uv)/determinant for i in range(3)];b=[(e2[i]*u1-e1[i]*u2)/determinant for i in range(3)]
        for vi in triangle:
            for i in range(3):tangents[vi][i]+=t[i];bitangents[vi][i]+=b[i]
    result=[]
    for v,t,b in zip(vertices,tangents,bitangents):
        n=v['normal'];length=math.sqrt(sum(x*x for x in n))
        if not length:raise a.halo.CacheError('Zero mesh normal')
        n=[x/length for x in n];dot=sum(n[i]*t[i] for i in range(3));t=[t[i]-dot*n[i] for i in range(3)]
        length=math.sqrt(sum(x*x for x in t))
        if length<1e-12:
            axis=min(range(3),key=lambda i:abs(n[i]));t=[float(i==axis)-n[axis]*n[i] for i in range(3)];length=math.sqrt(sum(x*x for x in t))
        t=[x/length for x in t];cross=(n[1]*t[2]-n[2]*t[1],n[2]*t[0]-n[0]*t[2],n[0]*t[1]-n[1]*t[0])
        result.append(t+[-1. if sum(cross[i]*b[i] for i in range(3))<0 else 1.])
    return result


def animated_iqm(surfaces,bones,bind_world,local_frames,skin_groups,attached,gun_mapping,scale,clips,posed_frames):
    # IQM evaluates parents sequentially. Halo node indices need not be ordered.
    order=[]
    def visit(i):
        if i in order:return
        if bones[i]['parent']>=0:visit(bones[i]['parent'])
        order.append(i)
    for i in range(len(bones)):visit(i)
    remap={old:new for new,old in enumerate(order)}
    joints=[{'name':bones[i]['name'],'parent':remap.get(bones[i]['parent'],-1)} for i in order]
    bind=[]
    for i in order:
        r,t=bind_world[i];p=bones[i]['parent']
        if p>=0:
            pr,pt=bind_world[p];r,t=mul(transpose(pr),r),rotate(transpose(pr),tuple(t[k]-pt[k] for k in range(3)))
        bind.append((r,t))
    extra=[('tag_weapon',-1,(IDENTITY,(0.,0.,0.)))]
    extra += [(m['name'],remap[gun_mapping[m['node']]],(qmatrix(m['rotation']),tuple(x*scale for x in m['translation']))) for m in attached]
    for name,parent,transform in extra:joints.append({'name':name,'parent':parent});bind.append(transform)
    channels=[]
    for local in local_frames:
        row=[transform_channels(local[i]) for i in order]+[transform_channels(t) for _,_,t in extra]
        if channels:
            for i,c in enumerate(row):
                if sum(c[k]*channels[-1][i][k] for k in range(3,7))<0:c[3:7]=[-v for v in c[3:7]]
        channels.append(row)
    text_data=bytearray();names={}
    def name(value):
        if value not in names:names[value]=len(text_data);text_data.extend(value.encode()+b'\0')
        return names[value]
    mesh=bytearray();triangles=bytearray();positions=bytearray();normals=bytearray();uv=bytearray();indices=bytearray();weights=bytearray();tangent_data=bytearray();nv=nt=0
    for si,(surface,(_,mapping)) in enumerate(zip(surfaces,skin_groups)):
        verts=surface['vertices'];tris=surface['triangles']
        tangent_data.extend(b''.join(struct.pack('<4f',*t) for t in mesh_tangents(surface)))
        mesh.extend(struct.pack('<6I',name('part'+str(si)),name(surface['shader']),nv,len(verts),nt,len(tris)))
        for v in verts:
            positions.extend(struct.pack('<3f',*v['position']));normals.extend(struct.pack('<3f',*v['normal']));uv.extend(struct.pack('<2f',*v['uv']))
            if all(n<0 for n in v['nodes']):raise a.halo.CacheError('Unbound IQM vertex')
            ids=[remap[mapping[n]] if n>=0 else 0 for n in v['nodes']]
            w=round(v['node0_weight']*255);indices.extend(bytes(ids+[0,0]));weights.extend(bytes((w,255-w,0,0)))
        triangles.extend(b''.join(struct.pack('<3I',*(nv+i for i in t)) for t in tris));nv+=len(verts);nt+=len(tris)
    joint_data=b''.join(struct.pack('<Ii10f',name(j['name']),j['parent'],*transform_channels(b)) for j,b in zip(joints,bind))
    pose=bytearray();ranges=[];nc=0
    for i,j in enumerate(joints):
        low=[min(f[i][k] for f in channels) for k in range(10)];step=[(max(f[i][k] for f in channels)-low[k])/65535 for k in range(10)]
        mask=sum(1<<k for k in range(10) if step[k]);nc+=mask.bit_count();ranges.append((low,step))
        pose.extend(struct.pack('<iI20f',j['parent'],mask,*low,*step))
    frames=bytearray()
    for f in channels:
        for c,(low,step) in zip(f,ranges):
            for k in range(10):
                if step[k]:frames.extend(struct.pack('<H',max(0,min(65535,round((c[k]-low[k])/step[k])))))
    anim=b''.join(struct.pack('<3IfI',name(c['name']),c['first'],c['count'],c['fps'],0) for c in clips)
    bounds=bytearray()
    for frame in posed_frames:
        p=[v['position'] for s in frame for v in s];lo=[min(v[k] for v in p) for k in range(3)];hi=[max(v[k] for v in p) for k in range(3)]
        bounds.extend(struct.pack('<8f',*lo,*hi,max(math.hypot(v[0],v[1]) for v in p),max(math.sqrt(sum(x*x for x in v)) for v in p)))
    body=bytearray()
    def chunk(data):
        while len(body)%4:body.append(0)
        offset=124+len(body);body.extend(data);return offset
    ot=chunk(text_data);om=chunk(mesh);otr=chunk(triangles);oj=chunk(joint_data);op=chunk(pose);oa=chunk(anim);of=chunk(frames);ob=chunk(bounds)
    arrays=[]
    for typ,fmt,size,data in [(0,7,3,positions),(1,7,2,uv),(2,7,3,normals),(3,7,4,tangent_data),(4,1,4,indices),(5,1,4,weights)]:arrays.append(struct.pack('<5I',typ,0,fmt,size,chunk(data)))
    ov=chunk(b''.join(arrays))
    return struct.pack('<16s27I',b'INTERQUAKEMODEL\0',2,124+len(body),0,len(text_data),ot,len(surfaces),om,6,nv,ov,nt,otr,0,len(joints),oj,len(joints),op,len(clips),oa,len(channels),nc,of,ob,0,0,0,0)+body


def markers(cache,assets,tag):
    result = []
    aliases = {'primary trigger':'tag_flash','secondary trigger':'tag_flash2','primary ejection':'tag_eject','secondary ejection':'tag_eject2','flashlight':'tag_light','right hand':'tag_hand'}
    for m in assets.reflexive('Model','markers',tag['offset'],'ModelMarker'):
        source = cache.cstring(m,32)
        if source not in aliases:continue
        instances = assets.reflexive('ModelMarker','instances',m,'ModelMarkerInstance')
        chosen = [p for p in instances if cache.data[p+1]==0]
        if not chosen:continue
        p = chosen[0];node = cache.data[p+2];t = struct.unpack_from('<3f',cache.data,p+4);q = struct.unpack_from('<4f',cache.data,p+16)
        result.append({'name':aliases[source],'source':source,'node':node,'translation':t,'rotation':q})
    return result


def pose_tags(markers,pose,scale):
    tags = {'tag_weapon':(IDENTITY,(0.,0.,0.))}
    for marker in markers:
        if not 0<=marker['node']<len(pose):raise a.halo.CacheError('Marker node outside model')
        r,t = pose[marker['node']];local = tuple(x*scale for x in marker['translation'])
        tags[marker['name']] = mul(r,qmatrix(marker['rotation'])),add(rotate(r,local),t)
    return tags


def read_tga(path):
    data = path.read_bytes();w,h = struct.unpack_from('<2H',data,12)
    if data[2]!=2 or data[16]!=32 or data[17]!=8 or len(data)!=18+w*h*4:raise a.halo.CacheError('Expected converter bottom-up TGA')
    pixels = bytearray(data[18:]);pixels[0::4],pixels[2::4] = data[20::4],data[18::4]
    stride = w*4
    return w,h,b''.join(pixels[y*stride:(y+1)*stride] for y in reversed(range(h)))


def material_layers(base,multi,xbox=True):
    if len(base)!=len(multi):raise a.halo.CacheError('Material dimensions differ')
    diffuse = bytearray(base);emissive = bytearray(len(base));specular = bytearray(len(base))
    # Xbox: R specular, G illumination, B color change, A auxiliary.
    # PC: B specular, G illumination, A color change, R auxiliary.
    for i in range(0,len(base),4):
        light = multi[i+1];spec = multi[i+(0 if xbox else 2)]
        for c in range(3):
            diffuse[i+c] = base[i+c]*(255-light)//255
            emissive[i+c] = base[i+c]*light//255
            specular[i+c] = spec
        emissive[i+3] = 255;specular[i+3] = 255
    return bytes(diffuse),bytes(emissive),bytes(specular)


def resample_rgba(pixels,sw,sh,dw,dh):
    """Sample mask data at the same normalized UVs as the base bitmap."""
    result = bytearray(dw*dh*4)
    for y in range(dh):
        fy = max(0,min(sh-1,(y+.5)*sh/dh-.5));y0 = int(fy);y1 = min(sh-1,y0+1);ty = fy-y0
        for x in range(dw):
            fx = max(0,min(sw-1,(x+.5)*sw/dw-.5));x0 = int(fx);x1 = min(sw-1,x0+1);tx = fx-x0
            for c in range(4):
                upper = pixels[(y0*sw+x0)*4+c]*(1-tx)+pixels[(y0*sw+x1)*4+c]*tx
                lower = pixels[(y1*sw+x0)*4+c]*(1-tx)+pixels[(y1*sw+x1)*4+c]*tx
                result[(y*dw+x)*4+c] = round(upper*(1-ty)+lower*ty)
    return bytes(result)


def materials(cache,assets,models):
    text = [];records = [];seen = set()
    for model in models:
        meta = json.loads((assets.output/model['source_mesh']).read_text())
        for part in meta['parts']:
            ident = (model['id'],part['geometry'],part['part'])
            if ident in seen:continue
            seen.add(ident);shader = next(t for t in cache.index if t['path']==part['shader'] and t['class'].startswith('s'))
            name = f'qce/halo/{ident[0]:08x}/{ident[1]}_{ident[2]}'
            if shader['class']!='soso':
                classes = {'schi':'ShaderTransparentChicago','sotr':'ShaderTransparentGeneric','sgla':'ShaderTransparentGlass','smet':'ShaderTransparentMeter'}
                kind = classes.get(shader['class']);maps = [];parameters = {}
                if kind:
                    for key,field in a.LAYOUT['structs'][kind]['fields'].items():
                        pos = shader['offset']+field['offset']
                        if field['type']=='TagDependency':
                            source = cache.tags.get(struct.unpack_from('<I',cache.data,pos+12)[0]);parameters[key] = source
                            if source and source['class']=='bitm':maps.append((key,source))
                        elif field['type']=='TagReflexive' and key=='maps':
                            for entry in assets.reflexive(kind,key,shader['offset'],field['struct']):
                                dep = a.LAYOUT['structs'][field['struct']]['fields']['map']['offset']
                                source = cache.tags.get(struct.unpack_from('<I',cache.data,entry+dep+12)[0])
                                if source:maps.append(('map',source))
                    if shader['class']=='sgla':maps.sort(key=lambda item:item[0]!='diffuse map')
                source = next((t for k,t in maps if (assets.output/f'textures/qce/halo/{a.safe_name(t["path"])}/000.tga').exists()),None)
                path = f'textures/qce/halo/{a.safe_name(source["path"])}/000.tga' if source else 'textures/qce/material/transparent.tga'
                if not source and path not in assets.files:assets.write(path,a.tga(1,1,bytes(4)),{'purpose':'unsupported transparent layer is invisible'})
                # First-layer presentation only; do not invent GPU combiner equations.
                blend = 'blend' if shader['class']=='sgla' else 'add'
                color='identity'
                if shader['class']=='smet':
                    fields=a.LAYOUT['structs'][kind]['fields']
                    rgb=struct.unpack_from('<3f',cache.data,shader['offset']+fields['background color']['offset'])
                    color='const ( '+' '.join(f'{x:g}' for x in rgb)+' )'
                text.append(f'{name}\n{{\n cull none\n {{ map {path}\n blendFunc {blend}\n rgbGen {color} }}\n}}\n')
                records.append({'shader':shader['path'],'class':shader['class'],'source_maps':maps,'runtime_shader':name,'status':'first-layer transparent approximation','approximations':['Halo register combiners/glass equations/meter value/numeric counter animation pending']});continue
            o = shader['offset'];f = a.LAYOUT['structs']['ShaderModel']['fields']
            def value(key):
                field = f[key];p = o+field['offset'];size = field['size']
                if field['type']=='TagDependency':
                    tag = struct.unpack_from('<I',cache.data,p+12)[0];return cache.tags.get(tag)
                return struct.unpack_from('<' + ('f'*(size//4) if size%4==0 else 'H'),cache.data,p)[0] if size<=4 else list(struct.unpack_from('<'+'f'*(size//4),cache.data,p))
            flags = value('shader model flags');base_tag = value('base map');multi_tag = value('multipurpose map');detail = value('detail map')
            if not base_tag:raise a.halo.CacheError('Model material missing base bitmap')
            base_path = f'textures/qce/halo/{a.safe_name(base_tag["path"])}/000.tga'
            w,h,base = read_tga(assets.output/base_path);emission = None;spec = None
            if multi_tag:
                mw,mh,multi = read_tga(assets.output/f'textures/qce/halo/{a.safe_name(multi_tag["path"])}/000.tga')
                if (mw,mh)!=(w,h):multi = resample_rgba(multi,mw,mh,w,h)
                diffuse,emission,spec = material_layers(base,multi)
                derived = f'textures/qce/material/{shader["id"]:08x}'
                diffuse_path = derived+'_d.tga';emission_path = derived+'_e.tga';spec_path = derived+'_s.tga'
                if diffuse_path not in assets.files:
                    for path,pixels in ((diffuse_path,diffuse),(emission_path,emission),(spec_path,spec)):
                        assets.write(path,a.tga(w,h,pixels),{'shader':shader['path'],'xbox_multipurpose':True})
                base_path = diffuse_path
            alpha_blend = bool(flags&8)
            stages = [f' {{\n  map {base_path}\n  rgbGen lightingDiffuse\n'+('  blendFunc blend\n' if alpha_blend else '  alphaGen const 1\n')+' }']
            approximations = []
            if detail and value('detail mask')==0:
                detail_path = f'textures/qce/halo/{a.safe_name(detail["path"])}/000.tga'
                function = value('detail function')
                if function in (0,1) and (assets.output/detail_path).exists():
                    ds = value('detail map scale');dv = value('detail map v scale') or ds
                    blend = 'GL_DST_COLOR GL_SRC_COLOR' if function==0 else 'GL_DST_COLOR GL_ZERO'
                    stages.append(f' {{ map {detail_path}\n blendFunc {blend}\n rgbGen identity\n tcMod scale {ds:g} {dv:g}\n }}')
                else:approximations.append('biased-add or unavailable detail map deferred')
            elif detail:approximations.append('masked detail deferred')
            if emission and any(emission[0::4]+emission[1::4]+emission[2::4]):
                stages.append(f' {{ map {emission_path}\n blendFunc add\n rgbGen identity\n }}')
            brightness = max(value('perpendicular brightness'),value('parallel brightness'))
            if spec and brightness>0:
                stages.append(f' {{ map {spec_path}\n blendFunc GL_SRC_ALPHA GL_ONE\n rgbGen const ( {brightness:g} {brightness:g} {brightness:g} )\n alphaGen lightingSpecular\n }}')
                approximations.append('Quake specular lobe; view-dependent Halo cubemap reflection deferred')
            reflection=value('reflection cube map')
            if reflection and brightness>0 and shader['path'].endswith('\\visor'):
                cube=f'textures/qce/halo/{a.safe_name(reflection["path"])}'
                faces=[read_tga(assets.output/(cube+f'/000_face{i}.tga')) for i in range(6)]
                import math
                ew,eh=256,128;pixels=bytearray()
                for y in range(eh):
                    latitude=math.pi*(y+0.5)/eh
                    for x in range(ew):
                        longitude=2*math.pi*(x+0.5)/ew
                        d=(math.sin(latitude)*math.cos(longitude),math.sin(latitude)*math.sin(longitude),math.cos(latitude))
                        axis=max(range(3),key=lambda i:abs(d[i]));v=d[axis]
                        face=axis*2+(v<0)
                        uv=((-d[2],-d[1]) if axis==0 and v>0 else (d[2],-d[1]) if axis==0 else (d[0],d[2]) if axis==1 and v>0 else (d[0],-d[2]) if axis==1 else (d[0],-d[1]) if v>0 else (-d[0],-d[1]))
                        w,h,data=faces[face];u=max(0,min(w-1,int((uv[0]/abs(v)+1)*0.5*w)));t=max(0,min(h-1,int((uv[1]/abs(v)+1)*0.5*h)))
                        pixels.extend(data[(t*w+u)*4:(t*w+u)*4+4])
                env=f'textures/qce/material/{shader["id"]:08x}_env.tga'
                if env not in assets.files:assets.write(env,a.tga(ew,eh,pixels),{'source':reflection,'approximation':'2D environment projection of source cubemap'})
                stages.append(f' {{ map {env}\n tcGen environment\n blendFunc add\n rgbGen const ( {brightness:g} {brightness:g} {brightness:g} )\n }}')
                approximations.append('2D environment mapping approximates Halo cubemap reflection')
            text.append(name+'\n{\n '+('cull none' if flags&2 else '')+'\n'+'\n'.join(stages)+'\n}\n')
            parameters = {k:value(k) for k in ('shader model flags','detail function','detail mask','detail map scale','detail map v scale','perpendicular brightness','parallel brightness','perpendicular tint color','parallel tint color','animation period','animation color lower bound','animation color upper bound','map u scale','map v scale')}
            records.append({'shader':shader['path'],'id':shader['id'],'parameters':parameters,'channel_order':'Xbox RGBA specular/illumination/color-change/auxiliary','approximations':approximations,'runtime_shader':name})
    assets.files.pop('scripts/qce-halo.shader',None)
    assets.write('scripts/qce-halo.shader','\n'.join(text).encode(),{'generated_for':'Halo weapon materials'})
    return records


def runtime_loop(clip):
    # Retail hot/charge entries can refer outside their separate track's frame
    # range. Keep that source index in the manifest; runtime clips loop locally.
    return clip['loop_frame'] if 0<=clip['loop_frame']<clip['count'] else 0


def weapon_sound_overrides(cache,assets,slot):
    if slot!='WP_LIGHTNING':return {}
    weapon=cache.tag('weapons\\plasma pistol\\plasma pistol','weap')['values']
    overrides={}
    charging=next((v['type'] for v in weapon['attachments'] if v['type'] and v['type']['class']=='lsnd'),None)
    if charging:
        tag=cache.tags[charging['id']]
        tracks=assets.reflexive('SoundLooping','tracks',tag['offset'],'SoundLoopingTrack')
        if len(tracks)!=1:raise a.halo.CacheError('Expected one charging audio track')
        p=tracks[0]
        for action,offset in [('chargeenter',48),('charge',64)]:
            source=cache.tags.get(struct.unpack_from('<I',cache.data,p+offset+12)[0])
            if source:overrides[action]={'source':source,'frame':0,'loop':action=='charge','binding':charging,'gain':struct.unpack_from('<f',cache.data,p+4)[0]}
    firing=weapon['triggers'][1]['firing effects'][0]['firing effect']
    if firing and firing['class']=='effe':
        effect=cache.tag(firing['path'],'effe')['values']
        sounds=[p['type'] for event in effect['events'] if event['delay bounds']==[0.,0.] for p in event['parts'] if p['type'] and p['type']['class']=='snd!']
        if len(sounds)==1:overrides['chargedfire']={'source':cache.tags[sounds[0]['id']],'frame':0,'loop':False,'binding':firing}
    return overrides


def animation_sounds(cache,graph,clips,assets,records,runtime,overrides=None):
    count,pointer=struct.unpack_from('<II',cache.data,graph['offset']+84)
    if count>256:raise a.halo.CacheError('Animation sound reference limit')
    start=cache.pointer(pointer,count*20) if count else 0
    refs=[cache.tags.get(struct.unpack_from('<I',cache.data,start+i*20+12)[0]) for i in range(count)]
    events=[];overrides=overrides or {}
    for action,clip in zip(ACTIONS,clips):
        outputs=[];source=None;frame=0;override=overrides.get(action)
        if clip and (override or clip['sound_index']>=0):
            if not override and clip['sound_index']>=count:raise a.halo.CacheError('Animation sound index outside graph')
            source=override['source'] if override else refs[clip['sound_index']]
            if not source or source['class']!='snd!':raise a.halo.CacheError('Invalid animation sound reference')
            frame=override['frame'] if override else clip['sound_frame']
            if action in ('overheat','chargedhot'):frame=0 # heat feedback begins on the final shot
            if not 0<=frame<clip['count']:raise a.halo.CacheError('Animation sound frame outside clip')
            candidates=[r for r in records if r['id']==source['id'] and 'permutation' in r and r['range']==0]
            for i,record in enumerate(candidates[:4]):
                path=f'sound/qce/halo/events/{runtime}/{action}{i+1}.wav'
                audio,calibration=assets.calibrated_sound(record)
                assets.files.pop(path,None);assets.write(path,audio,{'alias_of':record['outputs'][0],'source_event_frame':frame,'calibration':calibration})
                outputs.append(path)
        events.append({'action':action,'frame':frame,'source':source,'outputs':outputs,'loop':bool(override and override['loop']),'binding':override.get('binding') if override else None})
    path=f'models/qce/halo/view/{runtime}.events'
    lines=['2 '+str(len(ACTIONS))]
    lines += [f'{e["frame"]} {len(e["outputs"])} {int(e["loop"])}'+(' '+' '.join(e['outputs']) if e['outputs'] else '') for e in events]
    assets.files.pop(path,None);assets.write(path,('\n'.join(lines)+'\n').encode(),{'weapon':runtime,'animation_events':True})
    return events


def ammunition_skins(assets,surfaces,gun_meta,runtime,scale=60):
    if runtime!='machinegun':return None
    digits=[]
    for i,part in enumerate(gun_meta['parts']):
        if part['shader'].endswith('\\numbers'):
            center=sum(v['position'][1] for v in part['vertices'])/len(part['vertices'])
            name=f'qce/halo/{gun_meta["id"]:08x}/{part["geometry"]}_{part["part"]}'
            digits.append((center,name))
    if len(digits)!=2:raise a.halo.CacheError('Expected two assault-rifle digit surfaces')
    # Camera +Y is left; source positive-Y digit is the tens place.
    digits.sort(reverse=True);places={digits[0][1]:'t',digits[1][1]:'u'}
    source=next(t for t in assets.cache.index if t['path']=='weapons\\assault rifle\\fp\\bitmaps\\numbers_plate')
    shader=[]
    for digit in range(10):
        path=f'textures/qce/halo/{a.safe_name(source["path"])}/{digit:03}.tga'
        if path not in assets.files:raise a.halo.CacheError('Missing ammo digit bitmap')
        shader.append(f'qce/halo/digits/{digit}\n{{\n cull none\n {{ map {path}\n blendFunc add\n rgbGen identity }}\n}}\n')
    for ammo in range(scale+1):
        lines=[]
        for i,surface in enumerate(surfaces):
            place=places.get(surface['shader']);value=ammo//10 if place=='t' else ammo%10
            material=f'qce/halo/digits/{value}' if place else surface['shader']
            lines.append(f'part{i},{material}')
        path=f'models/qce/halo/view/{runtime}_{ammo}.skin';assets.files.pop(path,None)
        assets.write(path,('\n'.join(lines)+'\n').encode(),{'magazine':ammo,'source_digits':source['path']})
    path='scripts/qce-halo-digits.shader';assets.files.pop(path,None);assets.write(path,'\n'.join(shader).encode(),{'source':source['path']})
    return {'capacity':scale,'digits':digits,'source':source,'skins':scale+1}


def main():
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('map',type=Path);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--pk3',type=Path,required=True);parser.add_argument('--materials-only',action='store_true');args = parser.parse_args()
    cache = a.halo.XboxMap(args.map);report = json.loads((args.output/'manifest.json').read_text())
    if report['map']['sha256']!=cache.header['sha256']:raise a.halo.CacheError('Converted manifest belongs to another map')
    assets = a.Assets(cache,args.output);inventory = assets.inventory()
    g = cache.tag('globals\\globals','matg')['offset'];n,p = struct.unpack_from('<II',cache.data,g+380)
    if n!=1:raise a.halo.CacheError('Expected one first-person interface')
    interface = cache.pointer(p,16);hands_id = struct.unpack_from('<I',cache.data,interface+12)[0];hands = cache.tags[hands_id]
    enriched = {**inventory,'hands':{'models':[{'id':hands_id,'path':hands['path'],'role':'hands'}]}}
    if args.materials_only:
        models = []
        for weapon in enriched.values():
            for model in weapon['models']:
                source_mesh = 'model-sources/'+a.safe_name(model['path'])+'.json'
                meta = json.loads((args.output/source_mesh).read_text())
                if meta['id']!=model['id']:raise a.halo.CacheError('Mismatched model source sidecar')
                models.append({'id':model['id'],'source_mesh':source_mesh})
        models.extend(report.get('world_presentation',{}).get('models',[]))
        assets.files = {k:v for k,v in report['files'].items() if not k.startswith('textures/qce/material/')}
        report['materials'] = materials(cache,assets,models);report['files'] = assets.files
        if report.get('world_presentation'):
            spec = importlib.util.spec_from_file_location('halo_world',ROOT/'scripts/convert-halo-world.py');world = importlib.util.module_from_spec(spec);spec.loader.exec_module(world);world.export(cache,assets,report)
        (args.output/'manifest.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n');assets.package(args.pk3)
        print('Rebuilt Halo materials:',args.pk3);return
    models = assets.weapon_models(enriched)
    assets.files = {**{k:v for k,v in report['files'].items() if not k.startswith('textures/qce/material/')},**assets.files}
    stop = "sound/qce/halo/events/stop.wav"
    assets.files.pop(stop,None);assets.write(stop,a.wav(bytes(2),1,22050),{'purpose':'stop cancelled animation channel'})
    material_report = materials(cache,assets,models)
    model_map = {m['id']:m for m in models};hands_meta = json.loads((args.output/model_map[hands_id]['source_mesh']).read_text())
    exports = []
    for (slot,weapon),runtime in zip(inventory.items(),WEAPON_NAMES):
        model = next(m for m in weapon['models'] if m['role']=='first person model');source = model_map[model['id']]
        gun_meta = json.loads((args.output/source['source_mesh']).read_text());tag = cache.tags[model['id']]
        graph = cache.tag(weapon['first person animations']['path'],'antr');values = graph['values']
        node_count,node_pointer = struct.unpack_from('<II',cache.data,graph['offset']+104);node_offset = cache.pointer(node_pointer,node_count*64)
        bones = []
        for i in range(node_count):
            o = node_offset+i*64;parent = struct.unpack_from('<h',cache.data,o+36)[0]
            if parent==i:parent = -1
            bones.append({'name':cache.cstring(o,32),'parent':parent})
        names = {bone['name']:i for i,bone in enumerate(bones)}
        if len(names)!=len(bones):raise a.halo.CacheError('Duplicate animation bone names')
        surfaces = [];skin_groups = [];bind_by_name = {}
        for meta in (gun_meta,hands_meta):
            bind = globals_for(meta['bones'],[(qmatrix(b['rotation']),tuple(x*meta['scale'] for x in b['translation'])) for b in meta['bones']])
            bind_by_name.update({b["name"]:t for b,t in zip(meta["bones"],bind)})
            mapping = []
            for bone in meta['bones']:
                if bone['name'] not in names:raise a.halo.CacheError('Animation missing model bone '+bone['name'])
                mapping.append(names[bone['name']])
            for part in meta['parts']:
                name = f'qce/halo/{meta["id"]:08x}/{part["geometry"]}_{part["part"]}'
                pieces = a.split_surface(part['vertices'],part['triangles'],name)
                for piece in pieces:surfaces.append(piece);skin_groups.append((bind,mapping))
        frame_list = PoseFrames(surfaces);tag_list = [];clips = [];local_frames = []
        # Gameplay layout includes graph/animation structs; root animation array is offset 116.
        animation_count,animation_pointer = struct.unpack_from('<II',cache.data,graph['offset']+116)
        animation_start = cache.pointer(animation_pointer,animation_count*180)
        attached = markers(cache,assets,tag)
        for ai,animation in enumerate(values['animations']):
            if animation['type']!=0:continue # Overlay/additive tracks need composition, not absolute baking.
            if animation['node count']!=len(bones) or animation['frame info type']!=0:
                raise a.halo.CacheError('Unexpected weapon animation skeleton/root motion')
            if animation['flags']&1:raise a.halo.CacheError('Unexpected compressed retail track')
            raw = animation_start+ai*180;default = data_blob(cache,raw+140);frames = data_blob(cache,raw+160)
            decoded = decode_tracks(animation,default,frames);start = len(frame_list)
            for local in decoded:
                scaled = [(r,tuple(x*gun_meta['scale'] for x in t)) for r,t in local];local_frames.append(scaled)
                world = globals_for(bones,scaled)
                deforms = []
                gun_pose = [world[names[b['name']]] for b in gun_meta['bones']]
                for surface,(bind,mapping) in zip(surfaces,skin_groups):
                    pose = [world[i] for i in mapping]
                    deforms.append(skin_matrices(bind,pose))
                frame_list.poses.append(deforms);tag_list.append(pose_tags(attached,gun_pose,gun_meta['scale']))
            clips.append({'name':animation['name'],'first':start,'count':len(decoded),'fps':25 if animation['flags']&4 else 30,'key_frame':animation['key frame index'],'second_key_frame':animation['second key frame index'],'sound_index':animation['sound'],'sound_frame':animation['sound frame index'],'loop_frame':animation['loop frame index'],'default_sha256':hashlib.sha256(default).hexdigest(),'frames_sha256':hashlib.sha256(frames).hexdigest()})
        assets.files.pop(f'models/qce/halo/view/{runtime}.md3',None)
        path = f'models/qce/halo/view/{runtime}.iqm';assets.files.pop(path,None)
        fallback = globals_for(bones,local_frames[0]);bind_world = [bind_by_name.get(b['name'],fallback[i]) for i,b in enumerate(bones)]
        assets.write(path,animated_iqm(surfaces,bones,bind_world,local_frames,skin_groups,attached,[names[b['name']] for b in gun_meta['bones']],gun_meta['scale'],clips,frame_list),{'weapon':slot,'animated':True,'spartan_hands':True})
        config = [];bindings = [];bound_clips = []
        for action in ACTIONS:
            name = NAMES[action]
            if action=='fire' and slot=='WP_MACHINEGUN':name = 'firing'
            clip = next((c for c in clips if c['name']=='first-person '+name),None)
            if action=='reloadfull' and not clip:clip = next((c for c in clips if c['name']=='first-person reload-empty'),None)
            if action=='reloadempty' and not clip:clip = next((c for c in clips if c['name']=='first-person reload-full'),None)
            config.append(f'{clip["first"] if clip else 0} {clip["count"] if clip else 0} {clip["fps"] if clip else 30} {runtime_loop(clip) if clip else 0}')
            bound_clips.append(clip)
            bindings.append({'action':action,'source':clip['name'] if clip else None,'runtime_loop_frame':runtime_loop(clip) if clip else 0})
        cfg = f'models/qce/halo/view/{runtime}.cfg';assets.files.pop(cfg,None);assets.write(cfg,('\n'.join(config)+'\n').encode(),{'weapon':slot,'actions':ACTIONS})
        event_report = animation_sounds(cache,graph,bound_clips,assets,report['assets'],runtime,weapon_sound_overrides(cache,assets,slot))
        counter = ammunition_skins(assets,surfaces,gun_meta,runtime)
        exports.append({'sound_events':event_report,'ammo_display':counter,'weapon':slot,'model':path,'config':cfg,'frames':len(frame_list),'surfaces':len(surfaces),'clips':clips,'bindings':bindings,'markers':attached,'overlays_deferred':[c['name'] for c in values['animations'] if c['type']!=0]})
        print(runtime,len(frame_list),'frames',len(surfaces),'surfaces',len(attached),'attachments',flush=True)
    report['schema_version'] = 2;report['files'] = assets.files;report['animated_weapons'] = exports;report['materials'] = material_report
    report['limitations'] = ['Additive movement/ammunition/aim overlays pending','Animation sound gain/pitch/attenuation pending','Halo cubemap reflection and biased/masked detail approximated/deferred','Runtime animation transitions and retail view calibration require comparison']
    spec = importlib.util.spec_from_file_location('halo_world',ROOT/'scripts/convert-halo-world.py')
    world = importlib.util.module_from_spec(spec);spec.loader.exec_module(world);world.export(cache,assets,report)
    (args.output/'manifest.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n');assets.package(args.pk3)
    print('Packaged animated Halo weapons:',args.pk3)

if __name__=='__main__':main()
