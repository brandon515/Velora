#!/usr/bin/env python3
"""Writes the game's default assets into assets/: a unit cube (Cube.gltf and
Cube.bin), its procedural textures, and DefaultTexture.png, the renderer's
fallback texture. Everything is generated here, so the repo owns all of it.

Usage: python3 tools/make_cube_assets.py [output_dir]   (default: assets/)
Standard library only. The output is deterministic, so rerunning it gives
byte-identical files.
"""

import json
import struct
import sys
import zlib
from pathlib import Path

TEXTURE_SIZE = 256


def write_png(path, width, height, pixel):
    """pixel(x, y) -> (r, g, b, a) with 0-255 components."""
    rows = bytearray()
    for y in range(height):
        rows.append(0)  # filter type: none
        for x in range(width):
            rows.extend(pixel(x, y))

    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)  # 8-bit RGBA
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b"")
    path.write_bytes(png)


def base_color(x, y):
    # An 8x8 checkerboard of two blues with a light border on each face, so
    # orientation and UV mapping are easy to see.
    edge = 6
    if x < edge or y < edge or x >= TEXTURE_SIZE - edge or y >= TEXTURE_SIZE - edge:
        return (235, 235, 235, 255)
    cell = TEXTURE_SIZE // 8
    if ((x // cell) + (y // cell)) % 2 == 0:
        return (40, 110, 200, 255)
    return (20, 60, 130, 255)


def metallic_roughness(x, y):
    # glTF packs roughness in G and metalness in B: rough, non-metal.
    return (0, 200, 0, 255)


def missing_texture(x, y):
    # Magenta and black checkers: the usual "texture not found" look.
    cell = 8
    if ((x // cell) + (y // cell)) % 2 == 0:
        return (255, 0, 255, 255)
    return (0, 0, 0, 255)


def cube_geometry():
    """24 vertices (4 per face, so each face has its own normal and UVs) and 36 indices."""
    # (normal, u axis, v axis) for each face; corners are normal +- u +- v.
    faces = [
        ((1, 0, 0), (0, 0, -1), (0, 1, 0)),
        ((-1, 0, 0), (0, 0, 1), (0, 1, 0)),
        ((0, 1, 0), (1, 0, 0), (0, 0, -1)),
        ((0, -1, 0), (1, 0, 0), (0, 0, 1)),
        ((0, 0, 1), (1, 0, 0), (0, 1, 0)),
        ((0, 0, -1), (-1, 0, 0), (0, 1, 0)),
    ]
    positions, normals, uvs, indices = [], [], [], []
    for normal, u, v in faces:
        base = len(positions)
        for su, sv, tex in ((-1, -1, (0, 1)), (1, -1, (1, 1)), (1, 1, (1, 0)), (-1, 1, (0, 0))):
            positions.append(tuple(0.5 * (n + su * a + sv * b) for n, a, b in zip(normal, u, v)))
            normals.append(normal)
            uvs.append(tex)
        # Counter-clockwise seen from outside, glTF's front face.
        indices += [base, base + 1, base + 2, base, base + 2, base + 3]
    return positions, normals, uvs, indices


def write_cube(out_dir):
    positions, normals, uvs, indices = cube_geometry()

    index_bytes = struct.pack(f"<{len(indices)}H", *indices)
    position_bytes = b"".join(struct.pack("<3f", *p) for p in positions)
    normal_bytes = b"".join(struct.pack("<3f", *n) for n in normals)
    uv_bytes = b"".join(struct.pack("<2f", *t) for t in uvs)

    views, offset, blob = [], 0, bytearray()
    for data, target in ((index_bytes, 34963), (position_bytes, 34962), (normal_bytes, 34962), (uv_bytes, 34962)):
        while len(blob) % 4:
            blob.append(0)
        offset = len(blob)
        blob.extend(data)
        views.append({"buffer": 0, "byteOffset": offset, "byteLength": len(data), "target": target})
    (out_dir / "Cube.bin").write_bytes(bytes(blob))

    gltf = {
        "asset": {"version": "2.0", "generator": "Velora tools/make_cube_assets.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": "Cube"}],
        "meshes": [{
            "name": "Cube",
            "primitives": [{
                "attributes": {"POSITION": 1, "NORMAL": 2, "TEXCOORD_0": 3},
                "indices": 0,
                "material": 0,
            }],
        }],
        "materials": [{
            "name": "Cube",
            "pbrMetallicRoughness": {
                "baseColorTexture": {"index": 0},
                "metallicRoughnessTexture": {"index": 1},
            },
        }],
        "textures": [{"source": 0, "sampler": 0}, {"source": 1, "sampler": 0}],
        "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
        "images": [
            {"uri": "Cube_BaseColor.png", "mimeType": "image/png"},
            {"uri": "Cube_MetallicRoughness.png", "mimeType": "image/png"},
        ],
        "accessors": [
            {"bufferView": 0, "byteOffset": 0, "componentType": 5123, "count": len(indices), "type": "SCALAR",
             "min": [min(indices)], "max": [max(indices)]},
            {"bufferView": 1, "byteOffset": 0, "componentType": 5126, "count": len(positions), "type": "VEC3",
             "min": [-0.5, -0.5, -0.5], "max": [0.5, 0.5, 0.5]},
            {"bufferView": 2, "byteOffset": 0, "componentType": 5126, "count": len(normals), "type": "VEC3"},
            {"bufferView": 3, "byteOffset": 0, "componentType": 5126, "count": len(uvs), "type": "VEC2"},
        ],
        "bufferViews": views,
        "buffers": [{"uri": "Cube.bin", "byteLength": len(blob)}],
    }
    (out_dir / "Cube.gltf").write_text(json.dumps(gltf, indent=2) + "\n")


def main():
    out_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent / "assets"
    out_dir.mkdir(parents=True, exist_ok=True)
    write_cube(out_dir)
    write_png(out_dir / "Cube_BaseColor.png", TEXTURE_SIZE, TEXTURE_SIZE, base_color)
    write_png(out_dir / "Cube_MetallicRoughness.png", 16, 16, metallic_roughness)
    write_png(out_dir / "DefaultTexture.png", 64, 64, missing_texture)
    print(f"Wrote cube assets to {out_dir}")


if __name__ == "__main__":
    main()
