#!/usr/bin/env python3
"""Bounds and value checks for the numeric Xbox vehicle physics importer."""
import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('vehicle_physics', ROOT / 'scripts/extract-halo-vehicle-physics.py')
physics = importlib.util.module_from_spec(spec)
spec.loader.exec_module(physics)

class Cache:
    def __init__(self):
        self.data = bytearray(256)
        self.index = [{'class': 'phys', 'path': r'vehicles\warthog\warthog', 'offset': 0}]
        values = [0.0] * 23
        values[2] = 5000
        values[9] = .15
        values[20:23] = [600, 1700, 2000]
        struct.pack_into('<23f', self.data, 0, *values)
        struct.pack_into('<II', self.data, 116, 1, 128)
        struct.pack_into('<hh', self.data, 160, 0, 15)
        struct.pack_into('<f', self.data, 172, 5000)
        struct.pack_into('<3f', self.data, 184, .67, .38, .19)
        struct.pack_into('<h', self.data, 220, 1)
        struct.pack_into('<3f', self.data, 224, .75, .45, .26)
    def check(self, offset, size):
        if offset < 0 or offset + size > len(self.data):
            raise physics.h.CacheError('Pointer outside cache')
    def pointer(self, offset, size):
        self.check(offset, size)
        return offset

class VehiclePhysicsTests(unittest.TestCase):
    def test_units(self):
        _, points = physics.extract(Cache())
        self.assertAlmostEqual(points[0][0][0], 53.6, places=4)
        self.assertAlmostEqual(points[0][2], 20.8, places=4)
        self.assertEqual(points[0][1], 1)
    def test_missing_tag(self):
        c = Cache(); c.index = []
        with self.assertRaises(physics.h.CacheError): physics.extract(c)
    def test_pointer_bounds(self):
        c = Cache(); struct.pack_into('<I', c.data, 120, 240)
        with self.assertRaises(physics.h.CacheError): physics.extract(c)
    def test_bad_count(self):
        c = Cache(); struct.pack_into('<I', c.data, 116, 33)
        with self.assertRaises(physics.h.CacheError): physics.extract(c)
    def test_nonfinite_point(self):
        c = Cache(); struct.pack_into('<f', c.data, 184, float('nan'))
        with self.assertRaises(physics.h.CacheError): physics.extract(c)
    def test_negative_friction(self):
        c = Cache(); struct.pack_into('<f', c.data, 224, -1)
        with self.assertRaises(physics.h.CacheError): physics.extract(c)
    def test_zero_inertia(self):
        c = Cache(); struct.pack_into('<f', c.data, 80, 0)
        with self.assertRaises(physics.h.CacheError): physics.extract(c)

if __name__ == '__main__': unittest.main()
