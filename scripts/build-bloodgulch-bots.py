#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile owner-local Blood Gulch AAS and add it to the converted map package."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile
ROOT=Path(__file__).resolve().parents[1]
def check_game_data(data,home):
    """Follow loose-file / reverse PK3 search order in each baseq3 directory."""
    def read(name):
        for directory in (home/'baseq3',data/'baseq3'):
            loose=directory/name
            if loose.is_file():return loose.read_bytes()
            for package in sorted(directory.glob('*.pk3'),reverse=True):
                with zipfile.ZipFile(package) as archive:
                    if name in archive.namelist():return archive.read(name)
        raise ValueError(f'Missing {name}')
    bsp=read('maps/qce_bloodgulch.bsp')
    return validate_aas(read('maps/qce_bloodgulch.aas'),bsp_checksum(bsp))
def bsp_checksum(data):
    """Quake's XOR of the four MD4 digest words (not a CRC or timestamp)."""
    bits=len(data)*8;data+=b'\x80';data+=b'\0'*((56-len(data))%64)+struct.pack('<Q',bits)
    state=[0x67452301,0xefcdab89,0x98badcfe,0x10325476]
    for offset in range(0,len(data),64):
        words=struct.unpack_from('<16I',data,offset);v=state.copy()
        for round in range(3):
            order=range(16) if round==0 else [i%4*4+i//4 for i in range(16)] if round==1 else [0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15]
            shifts=((3,7,11,19),(3,5,9,13),(3,9,11,15))[round]
            for i,index in enumerate(order):
                k=(-i)%4;b,c,d=[v[(k+j)%4] for j in (1,2,3)]
                f=(b&c)|(~b&d) if round==0 else (b&c)|(b&d)|(c&d) if round==1 else b^c^d
                value=(v[k]+f+words[index]+(0,0x5a827999,0x6ed9eba1)[round]) & 0xffffffff
                shift=shifts[i%4];v[k]=((value<<shift)|(value>>(32-shift))) & 0xffffffff
        state=[(a+b)&0xffffffff for a,b in zip(state,v)]
    return state[0]^state[1]^state[2]^state[3]

def validate_aas(data,checksum):
    if len(data)<124 or data[:4]!=b'EAAS':raise ValueError('Invalid AAS output')
    header=bytearray(data[:124]);version=struct.unpack_from('<i',header,4)[0]
    if version==5:
        for i in range(8,len(header)):header[i]^=(i-8)*119 & 255
    elif version!=4:raise ValueError('Unsupported AAS version')
    if struct.unpack_from('<I',header,8)[0]!=checksum:raise ValueError('AAS belongs to a different BSP')
    lumps=[struct.unpack_from('<2i',header,12+i*8) for i in range(14)]
    for offset,length in lumps:
        if offset<0 or length<0 or offset+length>len(data):raise ValueError('AAS lump outside file')
    areas_length=lumps[7][1]
    if areas_length<96 or areas_length%48 or not lumps[9][1]:raise ValueError('AAS has no connected navigable areas')
    return areas_length//48-1

def build(output,pk3,bspc,import_aas=None):
    bsp=output/'maps/qce_bloodgulch.bsp';aas=bsp.with_suffix('.aas')
    if not bsp.is_file():raise ValueError('Convert Blood Gulch before compiling its bot navigation')
    checksum=bsp_checksum(bsp.read_bytes());log=output/'bot-navigation.log';cached=False
    if import_aas:
        data=import_aas.read_bytes();areas=validate_aas(data,checksum);aas.write_bytes(data);cached=True
    elif aas.exists():
        try:
            data=aas.read_bytes();areas=validate_aas(data,checksum);cached=True
        except ValueError:pass
    if not cached:
        compiler=shutil.which(str(bspc)) or (str(bspc.resolve()) if bspc.is_file() else None)
        if not compiler:raise ValueError('Run scripts/bootstrap-bspc.sh first')
        if aas.exists():aas.unlink() # stale output must not mask a compiler failure
        with log.open('w') as stream:
            result=subprocess.run([compiler,'-bsp2aas','maps/'+bsp.name,'-output','maps','-threads','2','-forcesidesvisible','-optimize','-noverbose'],stdout=stream,stderr=subprocess.STDOUT,cwd=output.resolve())
        if result.returncode or not aas.is_file():raise RuntimeError(f'Bot navigation compilation failed; see {log}')
        data=aas.read_bytes();areas=validate_aas(data,checksum)
    if pk3:
        entries={}
        if pk3.exists():
            with zipfile.ZipFile(pk3) as old:entries={n:old.read(n) for n in old.namelist() if n!='maps/qce_bloodgulch.aas'}
        entries['maps/qce_bloodgulch.aas']=data
        temporary=pk3.with_suffix('.pk3.tmp')
        with zipfile.ZipFile(temporary,'w',zipfile.ZIP_DEFLATED) as package:
            for name,value in entries.items():package.writestr(name,value)
        temporary.replace(pk3)
    print(json.dumps({'aas':str(aas),'bytes':len(data),'areas':areas,'cached':cached,'log':str(log)}))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'assets/halo/converted/map');p.add_argument('--pk3',type=Path,default=ROOT/'assets/baseq3/zz-qce-bloodgulch.pk3');p.add_argument('--bspc',type=Path,default=ROOT/'.tools/bin/bspc');p.add_argument('--import-aas',type=Path)
    p.add_argument('--check-data',type=Path);p.add_argument('--home',type=Path,default=ROOT/'runtime-client');a=p.parse_args()
    if a.check_data:
        try:print(f'Blood Gulch navigation verified: {check_game_data(a.check_data,a.home)} areas')
        except (ValueError,OSError,zipfile.BadZipFile) as error:
            p.exit(2,f'Blood Gulch navigation unavailable: {error}\nRun ./scripts/bootstrap-bspc.sh and python3 scripts/build-bloodgulch-bots.py after converting the map, then restart the game.\nCheck for older loose BSP/AAS files or duplicate map PK3s overriding the new package.\n')
    else:build(a.output,a.pk3,a.bspc,a.import_aas)
