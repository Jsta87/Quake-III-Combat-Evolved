#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline build-2276 textures/audio -> TGA/WAV and deterministic local PK3.

No third-party Python modules. Original maps and generated art stay in assets/.
Layout/algorithm references: Invader 696830ff80af227e84e7237c2ef26eb2301ed110.
"""
import argparse
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import re
import struct
import tempfile
import wave
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('halo_cache', ROOT / 'scripts/extract-halo.py')
halo = importlib.util.module_from_spec(spec)
spec.loader.exec_module(halo)
LAYOUT = json.loads((ROOT / 'data/halo-layout/assets-xbox.json').read_text())
FORMATS = {0:'a8',1:'y8',2:'ay8',3:'a8y8',6:'rgb565',8:'argb1555',9:'argb4444',10:'xrgb8888',11:'argb8888',14:'dxt1',15:'dxt3',16:'dxt5'}
BPP = {0:1,1:1,2:1,3:2,6:2,8:2,9:2,10:4,11:4}
FIRE = {'machinegun':'assault rifle', 'shotgun':'shotgun', 'rocket':'rocket launcher', 'railgun':'sniper rifle', 'plasma':'plasma rifle', 'lightning':'plasma rifle', 'bfg':'pistol', 'grenade':'needler'}
STEP = (7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767)
INDEX = (-1,-1,-1,-1,2,4,6,8)


def safe_name(path):
    """Never use map-controlled text as a filesystem or shader path."""
    parts = path.replace('\\', '/').split('/')
    if any(p in ('', '.', '..') for p in parts) or path.startswith('/'):
        raise halo.CacheError('Unsafe asset path')
    name = re.sub('[^a-z0-9_-]', '_', parts[-1].lower())[:12]
    # Full source-path digest disambiguates spaces/underscores and case collisions.
    return name + '_' + hashlib.sha256(path.encode()).hexdigest()[:16]


def rgb565(value):
    return ((value >> 11) * 255 // 31, ((value >> 5) & 63) * 255 // 63, (value & 31) * 255 // 31)


def unswizzle(data, width, height, bpp):
    if width & (width-1) or height & (height-1):
        raise halo.CacheError('Swizzled dimensions must be powers of two')
    if width <= 2 or height <= 1:
        return data
    output = bytearray(len(data))
    def index(x, y):
        result = 0
        bit = 1
        dest = 1
        while bit < max(width, height):
            if bit < width:
                if x & bit: result |= dest
                dest <<= 1
            if bit < height:
                if y & bit: result |= dest
                dest <<= 1
            bit <<= 1
        return result
    for y in range(height):
        for x in range(width):
            src = index(x,y)*bpp
            dst = (y*width+x)*bpp
            output[dst:dst+bpp] = data[src:src+bpp]
    return bytes(output)


def decode_pixels(data, width, height, fmt, swizzled=False):
    if not 1 <= width <= 4096 or not 1 <= height <= 4096:
        raise halo.CacheError('Invalid/oversized bitmap dimensions')
    if fmt not in FORMATS:
        raise halo.CacheError('Unsupported bitmap format')
    compressed = fmt in (14,15,16)
    size = ((width+3)//4)*((height+3)//4)*(8 if fmt==14 else 16) if compressed else width*height*BPP[fmt]
    if len(data) < size:
        raise halo.CacheError('Truncated pixel buffer')
    data = data[:size]
    if swizzled:
        if compressed: raise halo.CacheError('DXT bitmap marked swizzled')
        data = unswizzle(data,width,height,BPP[fmt])
    output = bytearray(width*height*4)
    if compressed:
        cursor = 0
        for by in range(0,height,4):
            for bx in range(0,width,4):
                block = data[cursor:cursor+(8 if fmt==14 else 16)]
                cursor += len(block)
                colors = block if fmt==14 else block[8:]
                c0,c1,bits = struct.unpack('<HHI',colors)
                palette = [(*rgb565(c0),255),(*rgb565(c1),255)]
                if c0>c1 or fmt!=14:
                    palette += [tuple((2*palette[0][j]+palette[1][j])//3 for j in range(3))+(255,),tuple((palette[0][j]+2*palette[1][j])//3 for j in range(3))+(255,)]
                else:
                    palette += [tuple((palette[0][j]+palette[1][j])//2 for j in range(3))+(255,),(0,0,0,0)]
                if fmt==15:
                    alpha_bits = int.from_bytes(block[:8],'little')
                elif fmt==16:
                    a,b = block[:2]
                    alphas = [a,b]
                    alphas += [((7-i)*a+i*b)//7 for i in range(1,7)] if a>b else [((5-i)*a+i*b)//5 for i in range(1,5)]+[0,255]
                    alpha_bits = int.from_bytes(block[2:8],'little')
                for i in range(16):
                    x,y = bx+i%4,by+i//4
                    if x>=width or y>=height: continue
                    pixel = palette[(bits>>(2*i))&3]
                    if fmt==15: pixel = pixel[:3]+(((alpha_bits>>(4*i))&15)*17,)
                    elif fmt==16: pixel = pixel[:3]+(alphas[(alpha_bits>>(3*i))&7],)
                    output[(y*width+x)*4:(y*width+x+1)*4] = bytes(pixel)
    else:
        for i in range(width*height):
            value = int.from_bytes(data[i*BPP[fmt]:(i+1)*BPP[fmt]],'little')
            if fmt==0: pixel = (255,255,255,value)
            elif fmt==1: pixel = (value,value,value,255)
            elif fmt==2: pixel = (value,value,value,value)
            elif fmt==3: pixel = (value&255,)*3+(value>>8,)
            elif fmt==6: pixel = rgb565(value)+(255,)
            elif fmt==8: pixel = (((value>>10)&31)*255//31,((value>>5)&31)*255//31,(value&31)*255//31,255 if value&32768 else 0)
            elif fmt==9: pixel = (((value>>8)&15)*17,((value>>4)&15)*17,(value&15)*17,(value>>12)*17)
            else: pixel = ((value>>16)&255,(value>>8)&255,value&255,(value>>24)&255 if fmt==11 else 255)
            output[i*4:i*4+4] = bytes(pixel)
    return bytes(output)


def tga(width, height, pixels):
    # ioquake3 ignores the top-origin flag: write actual bottom-up BGRA rows.
    header = struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,width,height,32,0x08)
    bgra = bytearray(pixels)
    bgra[0::4],bgra[2::4] = pixels[2::4],pixels[0::4]
    stride = width*4
    return header+b''.join(bgra[y*stride:(y+1)*stride] for y in reversed(range(height)))


def decode_adpcm(data, channels):
    if channels not in (1,2) or len(data) % (36*channels):
        raise halo.CacheError('Incomplete Xbox ADPCM block')
    samples = []
    for start in range(0,len(data),36*channels):
        states = []
        decoded = []
        for c in range(channels):
            predictor,index,reserved = struct.unpack_from('<hBB',data,start+c*4)
            if index>88 or reserved: raise halo.CacheError('Invalid Xbox ADPCM header')
            states.append([predictor,index]); decoded.append([predictor])
        for chunk in range(8):
            for c in range(channels):
                codes = int.from_bytes(data[start+4*channels+(chunk*channels+c)*4:start+4*channels+(chunk*channels+c+1)*4],'little')
                for i in range(8 if chunk<7 else 7):
                    code = (codes>>(i*4))&15
                    predictor,index = states[c]; step = STEP[index]
                    delta = (step>>3)+(step if code&4 else 0)+(step>>1 if code&2 else 0)+(step>>2 if code&1 else 0)
                    predictor = max(-32768,min(32767,predictor+(-delta if code&8 else delta)))
                    index = max(0,min(88,index+INDEX[code&7]))
                    states[c] = [predictor,index]; decoded[c].append(predictor)
        for i in range(64):
            samples.extend(decoded[c][i] for c in range(channels))
    return struct.pack('<'+'h'*len(samples),*samples)


def wav(pcm, channels, rate):
    if len(pcm) % (channels*2): raise halo.CacheError('Misaligned PCM frames')
    result = io.BytesIO()
    with wave.open(result,'wb') as stream:
        stream.setnchannels(channels);stream.setsampwidth(2);stream.setframerate(rate);stream.writeframes(pcm)
    return result.getvalue()


def unpack_fraction(value, bits):
    # Halo's negative packed floats use bias/sign encoding, not two's complement.
    mask = (1 << (bits-1))-1
    return (value & mask)/mask - (1 if value & (1 << (bits-1)) else 0)


def strip_triangles(indices, count):
    result = []
    for i in range(len(indices)-2):
        tri = list(indices[i:i+3])
        if 65535 in tri: continue
        if any(x>=count for x in tri): raise halo.CacheError('Out-of-range model index')
        if len(set(tri))<3:continue
        if i&1:tri[1],tri[2] = tri[2],tri[1]
        result.append(tri)
    return result


def md3(surfaces):
    """Static inspection/bind-pose MD3. No animation or attachment tags yet."""
    if not surfaces or len(surfaces)>32:raise halo.CacheError('MD3 surface limit')
    positions = [v['position'] for s in surfaces for v in s['vertices']]
    mins = [min(p[i] for p in positions) for i in range(3)]
    maxs = [max(p[i] for p in positions) for i in range(3)]
    radius = max(math.sqrt(sum(x*x for x in p)) for p in positions)
    frame = struct.pack('<10f16s',*(mins+maxs+[0,0,0,radius]),b'bind_pose')
    body = bytearray()
    for si,surface in enumerate(surfaces):
        verts = surface['vertices'];tris = surface['triangles']
        if not 0<len(verts)<1000 or len(tris)*3>=6000:raise halo.CacheError('Renderer surface limit')
        triangles = b''.join(struct.pack('<3i',*t) for t in tris)
        shader = surface['shader'].encode('ascii')
        if len(shader)>=64:raise halo.CacheError('Shader name too long')
        shader = struct.pack('<64si',shader,0)
        st = b''.join(struct.pack('<2f',*v['uv']) for v in verts)
        xyz = bytearray()
        for v in verts:
            coords = [round(x*64) for x in v['position']]
            if any(not -32768<=x<=32767 for x in coords):raise halo.CacheError('MD3 position overflow')
            nx,ny,nz = v['normal'];length = math.sqrt(nx*nx+ny*ny+nz*nz)
            longitude = round(math.acos(max(-1,min(1,nz/length if length else 1)))*255/(2*math.pi))&255
            latitude = round(math.atan2(ny,nx)*255/(2*math.pi))&255
            xyz.extend(struct.pack('<3hH',*coords,(latitude<<8)|longitude))
        tri_off = 108;shader_off = tri_off+len(triangles);st_off = shader_off+len(shader);xyz_off = st_off+len(st);end = xyz_off+len(xyz)
        body.extend(struct.pack('<4s64s10i',b'IDP3',f'part{si}'.encode(),0,1,1,len(verts),len(tris),tri_off,shader_off,st_off,xyz_off,end))
        body.extend(triangles+shader+st+xyz)
    end = 108+len(frame)+len(body)
    return struct.pack('<4si64s9i',b'IDP3',15,b'halo_bind_pose',0,1,0,len(surfaces),0,108,164,164,end)+frame+body


def split_surface(vertices, triangles, shader):
    """Respect both ioquake renderer limits while preserving triangle topology."""
    surfaces = [];mapping = {};out_vertices = [];out_triangles = []
    for tri in triangles:
        needed = sum(v not in mapping for v in tri)
        if len(out_vertices)+needed>=1000 or (len(out_triangles)+1)*3>=6000:
            surfaces.append({'vertices':out_vertices,'triangles':out_triangles,'shader':shader})
            mapping = {};out_vertices = [];out_triangles = []
        new = []
        for v in tri:
            if v not in mapping:mapping[v] = len(out_vertices);out_vertices.append(vertices[v])
            new.append(mapping[v])
        out_triangles.append(new)
    if out_triangles:surfaces.append({'vertices':out_vertices,'triangles':out_triangles,'shader':shader})
    return surfaces


class Assets:
    def __init__(self,cache,output):
        self.cache = cache; self.output = output; self.files = {}; self.records = []; self.skipped = []
    def field(self,kind,name,offset):
        return offset+LAYOUT['structs'][kind]['fields'][name]['offset']
    def reflexive(self,kind,name,offset,child):
        count,pointer = struct.unpack_from('<II',self.cache.data,self.field(kind,name,offset))
        if count>65535: raise halo.CacheError('Oversized asset reflexive')
        size = LAYOUT['structs'][child]['size']
        start = self.cache.pointer(pointer,count*size) if count else 0
        return [start+i*size for i in range(count)]
    def write(self,path,data,source):
        if path in self.files: raise halo.CacheError('Output collision')
        self.files[path] = {**source,'sha256':hashlib.sha256(data).hexdigest(),'size':len(data)}
        dest = self.output/path; dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
        return path
    def texture(self,tag):
        entries = self.reflexive('Bitmap','bitmap data',tag['offset'],'BitmapData')
        for i,offset in enumerate(entries):
            width,height,depth,kind,fmt,flags = struct.unpack_from('<6H',self.cache.data,offset+4)
            mipmaps = struct.unpack_from('<H',self.cache.data,offset+20)[0]
            raw_offset,size = struct.unpack_from('<II',self.cache.data,offset+24)
            record = {'tag':tag['path'],'id':tag['id'],'index':i,'width':width,'height':height,'depth':depth,'type':kind,'format':FORMATS.get(fmt,str(fmt)),'flags':flags,'mipmap_count':mipmaps,'raw_offset':raw_offset,'source_size':size}
            if kind not in (0,2) or depth!=1 or fmt not in FORMATS:
                self.skipped.append({**record,'reason':'3D/palettized/unsupported texture'});continue
            if bool(flags&2) != (fmt in (14,15,16)):raise halo.CacheError('Bitmap compression flag mismatch')
            if not 1<=width<=4096 or not 1<=height<=4096 or mipmaps>12:raise halo.CacheError('Invalid texture dimensions/mipmap count')
            # Retail Xbox offsets use the high bit as a runtime flag, not an address.
            source_offset = raw_offset&0x7fffffff
            self.cache.check(source_offset,size)
            raw = self.cache.data[source_offset:source_offset+size]
            record['source_sha256'] = hashlib.sha256(raw).hexdigest()
            faces = 6 if kind==2 else 1
            face_stride = 0
            for m in range(mipmaps+1):
                w,h = max(1,width>>m),max(1,height>>m)
                if fmt in (14,15,16):
                    if w<4 or h<4:break # Xbox's smaller DXT mip levels are invalid.
                    face_stride += w*h*(1 if fmt!=14 else .5)
                else:face_stride += w*h*BPP[fmt]
            face_stride = (int(face_stride)+127)&~127
            paths = []
            for face in range(faces):
                start = face*face_stride if kind==2 else 0
                top_size = width*height*(1 if fmt in (15,16) else .5 if fmt==14 else BPP[fmt])
                if start+top_size>size:raise halo.CacheError('Truncated cubemap face')
                pixels = decode_pixels(raw[start:],width,height,fmt,bool(flags&8))
                # Xbox swaps Y+ and X- relative to the PC ordering.
                canonical_face = (0,2,1,3,4,5)[face]
                suffix = f'_face{canonical_face}' if kind==2 else ''
                path = f'textures/qce/halo/{safe_name(tag["path"])}/{i:03}{suffix}.tga'
                paths.append(self.write(path,tga(width,height,pixels),{'tag':tag['path'],'index':i,'face':canonical_face if kind==2 else None}))
            self.records.append({**record,'outputs':paths})
    def sound(self,tag):
        o = tag['offset'];self.cache.check(o,164)
        rate = struct.unpack_from('<H',self.cache.data,o+6)[0]
        channels = struct.unpack_from('<H',self.cache.data,o+108)[0]+1
        if rate not in (0,1) or channels not in (1,2):raise halo.CacheError('Invalid sound rate/channels')
        rate = (22050,44100)[rate]
        for ri,r in enumerate(self.reflexive('Sound','pitch ranges',o,'SoundPitchRange')):
            parts = self.reflexive('SoundPitchRange','permutations',r,'SoundPermutation')
            actual = struct.unpack_from('<H',self.cache.data,r+44)[0]
            if actual>len(parts):raise halo.CacheError('Invalid actual sound permutation count')
            for pi in range(actual):
                current = pi; visited = set();segments = [];pcm = bytearray(); names = []
                while current != 65535:
                    if current in visited or not 0<=current<len(parts):raise halo.CacheError('Cyclic/invalid sound permutation chain')
                    visited.add(current);p = parts[current]
                    fmt,next_index = struct.unpack_from('<HH',self.cache.data,p+40)
                    size,flags,file_offset = struct.unpack_from('<III',self.cache.data,p+64)
                    file_offset &= 0x7fffffff
                    self.cache.check(file_offset,size);raw = self.cache.data[file_offset:file_offset+size]
                    if fmt not in (0,1):
                        self.skipped.append({'tag':tag['path'],'range':ri,'permutation':pi,'reason':f'Unsupported sound format {fmt}'});break
                    segment = decode_adpcm(raw,channels) if fmt==1 else raw
                    pcm.extend(segment);names.append(self.cache.cstring(p,32))
                    segments.append({'index':current,'offset':file_offset,'size':size,'format':fmt,'sha256':hashlib.sha256(raw).hexdigest()})
                    current = next_index
                else:
                    path = f'sound/qce/halo/{safe_name(tag["path"])}/{ri:02}_{pi:03}.wav'
                    self.write(path,wav(bytes(pcm),channels,rate),{'tag':tag['path'],'range':ri,'permutation':pi})
                    root_min,root_max,root_skip,pitch_min,pitch_max = struct.unpack_from('<5f',self.cache.data,o+8)
                    natural,bend_min,bend_max = struct.unpack_from('<3f',self.cache.data,r+32)
                    skip,gain = struct.unpack_from('<2f',self.cache.data,parts[pi]+32)
                    settings = {'minimum_distance_halo_units':root_min,'maximum_distance_halo_units':root_max,'skip_fraction':root_skip,'random_pitch_bounds':[pitch_min,pitch_max],'natural_pitch':natural,'bend_bounds':[bend_min,bend_max],'permutation_skip_fraction':skip,'permutation_gain':gain}
                    self.records.append({'tag':tag['path'],'id':tag['id'],'range':ri,'permutation':pi,'names':names,'channels':channels,'sample_rate':rate,'frames':len(pcm)//(2*channels),'duration_seconds':len(pcm)/(2*channels*rate),'playback_settings':settings,'segments':segments,'outputs':[path]})
    def calibrated_sound(self,record):
        """Bake source gain/playback rate once into a runtime alias; retain raw PCM."""
        tag=self.cache.tags[record['id']]
        gain=struct.unpack_from('<f',self.cache.data,tag['offset']+40)[0]
        ranges=self.reflexive('Sound','pitch ranges',tag['offset'],'SoundPitchRange')
        r=ranges[record['range']]
        playback=struct.unpack_from('<f',self.cache.data,r+48)[0]
        parts=self.reflexive('SoundPitchRange','permutations',r,'SoundPermutation')
        permutation_gain=struct.unpack_from('<f',self.cache.data,parts[record['permutation']]+36)[0]
        factor=gain*permutation_gain
        if not math.isfinite(factor) or not 0<=factor<=4 or not math.isfinite(playback) or not 0.25<=playback<=4:
            raise halo.CacheError('Invalid source sound calibration')
        with wave.open(str(self.output/record['outputs'][0]),'rb') as stream:
            if stream.getsampwidth()!=2:raise halo.CacheError('Expected signed 16-bit PCM')
            channels,rate=stream.getnchannels(),stream.getframerate()
            raw=stream.readframes(stream.getnframes())
        samples=struct.unpack('<'+'h'*(len(raw)//2),raw)
        calibrated=[max(-32768,min(32767,round(x*factor))) for x in samples]
        pcm=struct.pack('<'+'h'*len(calibrated),*calibrated)
        info={'definition_gain':gain,'permutation_gain':permutation_gain,'applied_gain':factor,'playback_rate':playback,
              'clipped_samples':sum(abs(x*factor)>32767 for x in samples),
              'raw_peak':max((abs(x) for x in samples),default=0),'calibrated_peak':max((abs(x) for x in calibrated),default=0)}
        return wav(pcm,channels,round(rate*playback)),info
    def inventory(self):
        result = {}
        for slot,path in halo.WEAPONS.items():
            if slot == 'WP_GRENADE_LAUNCHER' and not any(t['path']==path for t in self.cache.index):
                path = 'weapons\\needler\\needler'
            w = self.cache.tag(path,'weap')['values']
            result[slot] = {k:w[k] for k in ('model','first person model','first person animations')}
            models = []
            for key in ('model','first person model'):
                dep = w[key]
                if not dep:continue
                tag = self.cache.tags[dep['id']]
                if tag['class']!='mode':raise halo.CacheError('Expected Xbox model')
                shaders = []
                for s in self.reflexive('Model','shaders',tag['offset'],'ModelShaderReference'):
                    ident = struct.unpack_from('<I',self.cache.data,s+12)[0]
                    if ident==0xffffffff:continue
                    shader = self.cache.tags[ident];maps = {}
                    if shader['class']=='soso':
                        for name in ('base map','multipurpose map','detail map','reflection cube map'):
                            q = self.field('ShaderModel',name,shader['offset'])
                            ident = struct.unpack_from('<I',self.cache.data,q+12)[0]
                            maps[name] = self.cache.tags[ident]['path'] if ident!=0xffffffff else None
                    shaders.append({'path':shader['path'],'class':shader['class'],'bitmaps':maps})
                models.append({'role':key,**dep,'shaders':shaders,'status':'mesh/animation conversion pending'})
            result[slot]['models'] = models
        return result
    def weapon_models(self, inventory):
        converted = {};shader_text = []
        scale = json.loads((ROOT/'data/gameplay-profile.json').read_text())['units']['halo_to_quake_scale']
        for weapon in inventory.values():
            for dep in weapon['models']:
                if dep['id'] in converted:continue
                tag = self.cache.tags[dep['id']];o = tag['offset']
                geometries = self.reflexive('Model','geometries',o,'ModelGeometry')
                shader_refs = self.reflexive('Model','shaders',o,'ModelShaderReference')
                uv_scale = struct.unpack_from('<2f',self.cache.data,o+48)
                if not all(math.isfinite(x) for x in uv_scale):raise halo.CacheError('Invalid model UV scale')
                selected = set();selections = []
                for region in self.reflexive('Model','regions',o,'ModelRegion'):
                    permutations = self.reflexive('ModelRegion','permutations',region,'ModelRegionPermutation')
                    if not permutations:continue
                    p = permutations[0]
                    levels = struct.unpack_from('<5h',self.cache.data,p+64)
                    geometry = next((x for x in reversed(levels) if x>=0),None)
                    if geometry is None:continue
                    if geometry>=len(geometries):raise halo.CacheError('Invalid selected model geometry')
                    selected.add(geometry)
                    selections.append({'region':self.cache.cstring(region,32),'permutation':self.cache.cstring(p,32),'geometry':geometry,'lod_indices':levels})
                surfaces = [];mesh_parts = []
                for gi in sorted(selected):
                    for pi,p in enumerate(self.reflexive('ModelGeometry','parts',geometries[gi],'ModelGeometryPart')):
                        shader_index = struct.unpack_from('<h',self.cache.data,p+4)[0]
                        index_count = struct.unpack_from('<I',self.cache.data,p+72)[0]+2
                        index_ptr = struct.unpack_from('<I',self.cache.data,p+76)[0]
                        vertex_count = struct.unpack_from('<I',self.cache.data,p+88)[0]
                        vertex_header = struct.unpack_from('<I',self.cache.data,p+100)[0]
                        if vertex_count>65535 or index_count>65537:raise halo.CacheError('Oversized model part')
                        if not vertex_count:continue
                        vh = self.cache.pointer(vertex_header,12)
                        vp = struct.unpack_from('<I',self.cache.data,vh+4)[0]
                        vo = self.cache.pointer(vp,vertex_count*32)
                        io = self.cache.pointer(index_ptr,index_count*2)
                        indices = struct.unpack_from('<'+'H'*index_count,self.cache.data,io)
                        triangles = strip_triangles(indices,vertex_count)
                        vertices = []
                        for vi in range(vertex_count):
                            x,y,z,normal,binormal,tangent,u,v,n0,n1,weight = struct.unpack_from('<3f3I2H2bH',self.cache.data,vo+vi*32)
                            if not all(math.isfinite(t) for t in (x,y,z)):raise halo.CacheError('Nonfinite model vertex')
                            vertices.append({'position':[x*scale,y*scale,z*scale],'normal':[unpack_fraction(normal,11),unpack_fraction(normal>>11,11),unpack_fraction(normal>>22,10)],'uv':[unpack_fraction(u,16)*(uv_scale[0] or 1),unpack_fraction(v,16)*(uv_scale[1] or 1)],'nodes':[n0//3 if n0>=0 else -1,n1//3 if n1>=0 else -1],'node0_weight':unpack_fraction(weight,16)})
                        if not 0<=shader_index<len(shader_refs):raise halo.CacheError('Invalid model shader reference')
                        ident = struct.unpack_from('<I',self.cache.data,shader_refs[shader_index]+12)[0]
                        if ident not in self.cache.tags:raise halo.CacheError('Missing model shader')
                        shader_tag = self.cache.tags[ident]
                        shader_name = f'qce/halo/{tag["id"]:08x}/{gi}_{pi}'
                        if shader_tag['class']=='soso':
                            base_id = struct.unpack_from('<I',self.cache.data,self.field('ShaderModel','base map',shader_tag['offset'])+12)[0]
                            bitmap = self.cache.tags.get(base_id)
                            if bitmap:
                                image_path = f'textures/qce/halo/{safe_name(bitmap["path"])}/000.tga'
                                shader_text.append(f'{shader_name}\n{{\n cull none\n {{\n  map {image_path}\n  rgbGen lightingDiffuse\n  alphaGen const 1\n }}\n}}\n')
                            else:shader_text.append(f'{shader_name}\n{{\n {{ map $whiteimage\n rgbGen lightingDiffuse }}\n}}\n')
                        else:
                            shader_text.append(f'{shader_name}\n{{\n {{ map $whiteimage\n rgbGen lightingDiffuse }}\n}}\n')
                        surfaces.extend(split_surface(vertices,triangles,shader_name))
                        mesh_parts.append({'geometry':gi,'part':pi,'shader':shader_tag['path'],'vertices':vertices,'triangles':triangles,'vertex_source_sha256':hashlib.sha256(self.cache.data[vo:vo+vertex_count*32]).hexdigest(),'index_source_sha256':hashlib.sha256(self.cache.data[io:io+index_count*2]).hexdigest()})
                bones = []
                for n in self.reflexive('Model','nodes',o,'ModelNode'):
                    bones.append({'name':self.cache.cstring(n,32),'parent':struct.unpack_from('<h',self.cache.data,n+36)[0],'translation':struct.unpack_from('<3f',self.cache.data,n+40),'rotation':struct.unpack_from('<4f',self.cache.data,n+52)})
                path = f'models/qce/halo/{safe_name(tag["path"])}.md3'
                self.write(path,md3(surfaces),{'tag':tag['path'],'static_bind_pose':True})
                metadata = {'tag':tag['path'],'id':tag['id'],'scale':scale,'selections':selections,'bones':bones,'parts':mesh_parts,'md3':path,'status':'static bind pose; no animation or attachments; simplified diffuse materials'}
                sidecar = self.output/'model-sources'/f'{safe_name(tag["path"])}.json'
                sidecar.parent.mkdir(parents=True,exist_ok=True);sidecar.write_text(json.dumps(metadata,allow_nan=False)+'\n')
                converted[dep['id']] = {'tag':tag['path'],'id':tag['id'],'output':path,'source_mesh':str(sidecar.relative_to(self.output)),'vertices':sum(len(s['vertices']) for s in surfaces),'triangles':sum(len(s['triangles']) for s in surfaces),'surfaces':len(surfaces),'bones':len(bones),'status':metadata['status']}
        if shader_text:self.write('scripts/qce-halo.shader','\n'.join(shader_text).encode(),{'generated_for':'static weapon inspection models'})
        return list(converted.values())
    def package(self,path):
        # Include only this run's recorded files; stale output files never enter the PK3.
        path = Path(path);path.parent.mkdir(parents=True,exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=path.parent,prefix='.qce-assets-',suffix='.tmp',delete=False) as file:
            temporary = Path(file.name)
        try:
            with zipfile.ZipFile(temporary,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
                for name in sorted(self.files):
                    entry = zipfile.ZipInfo(name,date_time=(1980,1,1,0,0,0));entry.compress_type=zipfile.ZIP_DEFLATED;entry.external_attr=0o644<<16
                    archive.writestr(entry,(self.output/name).read_bytes())
            temporary.replace(path)
        finally:
            temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('map',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--pk3',type=Path)
    parser.add_argument('--textures-only',action='store_true')
    parser.add_argument('--sounds-only',action='store_true')
    args = parser.parse_args()
    if args.textures_only and args.sounds_only:parser.error('Choose one filter')
    cache = halo.XboxMap(args.map);assets = Assets(cache,args.output)
    for tag in cache.index:
        if tag['class']=='bitm' and not args.sounds_only:assets.texture(tag)
        if tag['class']=='snd!' and not args.textures_only:assets.sound(tag)
    # Stable runtime paths. Plasma pistol uses the normal plasma fire tag too.
    for weapon,source in FIRE.items():
        candidates = [r for r in assets.records if r['tag']==f'sound\\sfx\\weapons\\{source}\\fire' and 'permutation' in r and r['range']==0]
        for i in range(min(4,len(candidates))):
            src = candidates[i]['outputs'][0]
            assets.write(f'sound/qce/halo/fire/{weapon}{i+1}.wav',(args.output/src).read_bytes(),{'alias_of':src})
    inventory = assets.inventory() if cache.header['name']=='bloodgulch' else {}
    models = assets.weapon_models(inventory) if inventory and not args.sounds_only and not args.textures_only else []
    for weapon in inventory.values():
        for model in weapon['models']:
            converted = next((m for m in models if m['id']==model['id']),None)
            if converted:model.update({'output':converted['output'],'status':converted['status']})
    report = {'schema_version':1,'map':cache.header,'layout_reference':{k:LAYOUT[k] for k in ('source','commit','license')},'tag_counts':{g:sum(t['class']==g for t in cache.index) for g in sorted({t['class'] for t in cache.index})},'assets':assets.records,'models':models,'skipped':assets.skipped,'files':assets.files,'weapons':inventory,'limitations':['Top mip only; Quake regenerates mipmaps','3D and palettized bump textures skipped','Static weapon MD3s are inspection models: first permutation/highest LOD, no animations or attachment tags','Simplified diffuse materials; Halo shader rendering pending','Raw audio variations/chains preserved; runtime gain/pitch/attenuation integration pending','Animated weapon/player integration, HUD, effects and vehicles pending']}
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'manifest.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    if args.pk3:
        args.pk3.parent.mkdir(parents=True,exist_ok=True);assets.package(args.pk3)
    print(f"Converted {cache.header['name']}: {len(assets.records)} assets, {len(assets.files)} files; {len(assets.skipped)} explicitly skipped")

if __name__=='__main__':main()
