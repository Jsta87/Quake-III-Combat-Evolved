#!/usr/bin/env python3
"""Synthetic fixtures only: no retail maps/art required for CI."""
import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest
import wave
import zipfile

ROOT = Path(__file__).resolve().parents[1]
s = importlib.util.spec_from_file_location('convert',ROOT/'scripts/convert-halo-assets.py')
a = importlib.util.module_from_spec(s);s.loader.exec_module(a)

class FixtureCache:
    def __init__(self):self.data = bytearray(4096)
    def check(self,offset,size):
        if offset<0 or size<0 or offset+size>len(self.data):raise a.halo.CacheError('Fixture bounds')
    def pointer(self,pointer,size):self.check(pointer,size);return pointer
    def cstring(self,offset,limit):return bytes(self.data[offset:offset+limit]).split(b'\0')[0].decode('ascii')

class AssetTests(unittest.TestCase):
    def test_dxt1_transparency_and_selectors(self):
        block = struct.pack('<HHI',0,65535,sum((i%4)<<(2*i) for i in range(16)))
        rgba = a.decode_pixels(block,4,4,14)
        self.assertEqual(rgba[:16],bytes((0,0,0,255,255,255,255,255,127,127,127,255,0,0,0,0)))
    def test_dxt3_explicit_alpha(self):
        alpha = sum(i<<(4*i) for i in range(16))
        block = alpha.to_bytes(8,'little')+struct.pack('<HHI',0xf800,0x07e0,0)
        rgba = a.decode_pixels(block,4,4,15)
        self.assertEqual(list(rgba[3::4]),[i*17 for i in range(16)])
        self.assertEqual(rgba[:3],b'\xff\0\0')
    def test_dxt5_both_alpha_modes(self):
        for first,second,expected in [(255,0,[255,0,218,182,145,109,72,36]),(0,255,[0,255,51,102,153,204,0,255])]:
            bits = sum((i%8)<<(3*i) for i in range(16))
            block = bytes((first,second))+bits.to_bytes(6,'little')+struct.pack('<HHI',0,0,0)
            self.assertEqual(list(a.decode_pixels(block,4,4,16)[3::4]),expected*2)
    def test_rectangular_unswizzle(self):
        # 4x2 Morton order: TL 2x2 followed by TR 2x2.
        src = bytes((0,1,4,5,2,3,6,7))
        self.assertEqual(a.unswizzle(src,4,2,1),bytes(range(8)))
        # Tall counterpart consists of stacked square tiles.
        self.assertEqual(a.unswizzle(bytes((0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15)),4,4,1),bytes(range(16)))
    def test_uncompressed_formats_and_tga(self):
        self.assertEqual(a.decode_pixels(b'\x80',1,1,0),bytes((255,255,255,128)))
        self.assertEqual(a.decode_pixels(b'\x80',1,1,2),bytes((128,)*4))
        self.assertEqual(a.decode_pixels(b'\x40\x80',1,1,3),bytes((64,64,64,128)))
        self.assertEqual(a.decode_pixels(b'\0\xf8',1,1,6),bytes((255,0,0,255)))
        self.assertEqual(a.decode_pixels(b'\x11\x22\x33\x44',1,1,11),bytes((51,34,17,68)))
        image = a.tga(1,1,bytes((51,34,17,68)))
        self.assertEqual(image[17],0x08)
        self.assertEqual(image[18:],bytes((17,34,51,68)))
        rows = a.tga(1,2,bytes((255,0,0,255,0,0,255,255)))
        self.assertEqual(rows[18:],bytes((255,0,0,255,0,0,255,255)))
    def test_adpcm_mono_and_stereo_blocks(self):
        mono = struct.pack('<hBB',1234,0,0)+b'\0'*32
        self.assertEqual(a.decode_adpcm(mono,1),struct.pack('<64h',*([1234]*64)))
        # Two predictors followed by alternating four-byte channel code chunks.
        stereo = struct.pack('<hBBhBB',1000,0,0,-1000,0,0)+b'\0'*64
        pcm = a.decode_adpcm(stereo*2,2)
        self.assertEqual(struct.unpack('<256h',pcm),(1000,-1000)*128)
        with wave.open(io.BytesIO(a.wav(pcm,2,44100))) as sound:
            self.assertEqual((sound.getnchannels(),sound.getnframes(),sound.getframerate()),(2,128,44100))
    def test_adpcm_step_and_saturation(self):
        block = struct.pack('<hBB',0,0,0)+b'\x77'*32
        samples = struct.unpack('<64h',a.decode_adpcm(block,1))
        self.assertEqual(samples[:4],(0,11,41,104))
        self.assertEqual(samples[-1],32767)
    def test_malformed_buffers_and_paths(self):
        for call in [lambda:a.decode_pixels(b'',4,4,14),lambda:a.decode_pixels(b'\0'*8,4,4,14,True),lambda:a.decode_pixels(b'',0,1,1),lambda:a.decode_adpcm(b'\0'*35,1),lambda:a.decode_adpcm(b'\0\0\xff\0'+b'\0'*32,1),lambda:a.unswizzle(b'\0'*6,3,2,1)]:
            with self.assertRaises(a.halo.CacheError):call()
        for name in ('../secret','a\\..\\b','/root','a//b'):
            with self.assertRaises(a.halo.CacheError):a.safe_name(name)
        self.assertNotEqual(a.safe_name('a b'),a.safe_name('a_b'))
        self.assertLess(len('textures/qce/halo/'+a.safe_name('weapons\\'+'x'*100)+'/000_face0.tga'),64)
    def test_halo_biased_packed_fractions(self):
        self.assertEqual(a.unpack_fraction(0,16),0)
        self.assertEqual(a.unpack_fraction(32767,16),1)
        self.assertEqual(a.unpack_fraction(32768,16),-1)
        self.assertEqual(a.unpack_fraction(65535,16),0)
    def test_strip_winding_and_degenerates(self):
        self.assertEqual(a.strip_triangles((0,1,2,3,3,4,4,5,6),7),[[0,1,2],[1,3,2],[4,5,6]])
        with self.assertRaises(a.halo.CacheError):a.strip_triangles((0,1,9),3)
    def test_md3_offsets_and_surface_split(self):
        v = {'position':[1,2,3],'normal':[0,0,1],'uv':[0,1]}
        data = a.md3([{'vertices':[v,v,v],'triangles':[[0,1,2]],'shader':'qce/halo/test'}])
        self.assertEqual(data[:4],b'IDP3')
        header = struct.unpack_from('<9i',data,72)
        self.assertEqual(header,(0,1,0,1,0,108,164,164,len(data)))
        surf = struct.unpack_from('<10i',data,164+68)
        self.assertEqual(surf[:5],(0,1,1,3,1))
        self.assertEqual(164+surf[-1],len(data))
        self.assertEqual(struct.unpack_from('<3hH',data,164+surf[-2]),(64,128,192,0))
        vertices = [v]*1200
        tris = [list(range(i,i+3)) for i in range(0,1200,3)]
        split = a.split_surface(vertices,tris,'qce/halo/test')
        self.assertEqual(len(split),2)
        self.assertEqual(sum(len(s['triangles']) for s in split),400)
        self.assertTrue(all(len(s['vertices'])<1000 for s in split))
    def test_cubemap_face_padding_and_order(self):
        cache = FixtureCache()
        struct.pack_into('<II',cache.data,96,1,200)
        struct.pack_into('<6H',cache.data,204,4,4,1,2,14,131)
        struct.pack_into('<II',cache.data,224,1000|0x80000000,768)
        colors = (0xf800,0x07e0,0x001f,0xffff,0,0xffe0)
        for i,c in enumerate(colors):struct.pack_into('<HHI',cache.data,1000+i*128,c,0,0)
        with tempfile.TemporaryDirectory() as tmp:
            assets = a.Assets(cache,Path(tmp));assets.texture({'path':'cube','id':1,'offset':0})
            self.assertEqual(len(assets.files),6)
            for source,face in enumerate((0,2,1,3,4,5)):
                p = next(p for p in assets.files if p.endswith(f'face{face}.tga'))
                image = (Path(tmp)/p).read_bytes()
                rgb = a.rgb565(colors[source]);self.assertEqual(image[18:22],bytes((rgb[2],rgb[1],rgb[0],255)))
            struct.pack_into('<I',cache.data,228,768-128)
            with self.assertRaises(a.halo.CacheError):a.Assets(cache,Path(tmp)).texture({'path':'truncated','id':2,'offset':0})
    def test_sound_chain_join_and_cycle_rejection(self):
        cache = FixtureCache()
        struct.pack_into('<II',cache.data,152,1,200)
        struct.pack_into('<H',cache.data,244,1) # One variation, two linked chunks.
        struct.pack_into('<II',cache.data,260,2,300)
        for i in range(2):
            p = 300+i*124
            struct.pack_into('<HH',cache.data,p+40,1,1 if i==0 else 65535)
            struct.pack_into('<III',cache.data,p+64,36,0,1000+i*36)
            struct.pack_into('<hBB',cache.data,1000+i*36,1000*i,0,0)
        with tempfile.TemporaryDirectory() as tmp:
            assets = a.Assets(cache,Path(tmp));assets.sound({'path':'chain','id':1,'offset':0})
            self.assertEqual(assets.records[0]['frames'],128)
            self.assertEqual(len(assets.records[0]['segments']),2)
            with wave.open(str(Path(tmp)/assets.records[0]['outputs'][0])) as stream:
                self.assertEqual(struct.unpack('<128h',stream.readframes(128)),(0,)*64+(1000,)*64)
            struct.pack_into('<H',cache.data,300+124+42,0)
            with self.assertRaises(a.halo.CacheError):a.Assets(cache,Path(tmp)).sound({'path':'cycle','id':2,'offset':0})
    def test_deterministic_package_excludes_stale_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            assets = a.Assets(None,out)
            assets.write('textures/qce/test.tga',b'fixture',{})
            (out/'stale.map').write_bytes(b'not included')
            assets.package(out/'one.pk3');assets.package(out/'two.pk3')
            self.assertEqual((out/'one.pk3').read_bytes(),(out/'two.pk3').read_bytes())
            with zipfile.ZipFile(out/'one.pk3') as archive:
                self.assertEqual(archive.namelist(),['textures/qce/test.tga'])
                self.assertEqual(archive.testzip(),None)

if __name__=='__main__':unittest.main()
