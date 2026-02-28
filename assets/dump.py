#!/usr/bin/env python3

import struct
import sys

# ============================================================
# CONSTANTS
# ============================================================
MESH_MAGIC = 0x4D455348
CHUNK_MATS = 0x4D415453
CHUNK_SKEL = 0x534B454C
CHUNK_BUFF = 0x42554646
CHUNK_VRTS = 0x56525453
CHUNK_IDXS = 0x49445853
CHUNK_SKIN = 0x534B494E

ANIM_MAGIC = 0x414E494D
ANIM_CHUNK_INFO = 0x494E464F
ANIM_CHUNK_CHAN = 0x4348414E

def read_uint(f):
    return struct.unpack('<I', f.read(4))[0]

def read_int(f):
    return struct.unpack('<i', f.read(4))[0]

def read_float(f):
    return struct.unpack('<f', f.read(4))[0]

def read_byte(f):
    return struct.unpack('<B', f.read(1))[0]

def read_cstring(f):
    chars = []
    while True:
        c = f.read(1)
        if c == b'\0' or len(c) == 0:
            break
        chars.append(c)
    return b''.join(chars).decode('utf-8', errors='replace')

def dump_mesh(filepath):
    print(f"\n{'='*70}")
    print(f"DUMPING MESH: {filepath}")
    print(f"{'='*70}\n")
    
    import os
    file_size = os.path.getsize(filepath)
    print(f"📦 File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")
    
    with open(filepath, 'rb') as f:
        magic = read_uint(f)
        if magic != MESH_MAGIC:
            print(f"❌ Invalid magic: 0x{magic:08X}")
            return
        
        version = read_uint(f)
        print(f"✓ Version: {version}\n")
        
        chunk_count = 0
        buffer_count = 0
        total_verts = 0
        total_indices = 0
        
        while True:
            try:
                chunk_id = read_uint(f)
                chunk_len = read_uint(f)
            except:
                break
            
            chunk_count += 1
            chunk_end = f.tell() + chunk_len
            
            chunk_name = {
                CHUNK_MATS: "MATERIALS",
                CHUNK_SKEL: "SKELETON",
                CHUNK_BUFF: "BUFFER",
                CHUNK_VRTS: "VERTICES",
                CHUNK_IDXS: "INDICES",
                CHUNK_SKIN: "SKINNING"
            }.get(chunk_id, f"UNKNOWN(0x{chunk_id:08X})")
            
            print(f"[{chunk_count:02d}] {chunk_name:12s} | Size: {chunk_len:8,} bytes")
            
            if chunk_id == CHUNK_MATS:
                num_mats = read_uint(f)
                print(f"     └─ Materials: {num_mats}")
                for i in range(num_mats):
                    name = read_cstring(f)
                    # Skip diffuse, specular, shininess
                    f.read(7 * 4)
                    num_textures = read_byte(f)
                    print(f"        [{i}] {name} ({num_textures} textures)")
                    for j in range(num_textures):
                        tex = read_cstring(f)
                        print(f"            └─ {tex}")
            
            elif chunk_id == CHUNK_SKEL:
                num_bones = read_uint(f)
                print(f"     └─ Bones: {num_bones}")
                for i in range(num_bones):
                    name = read_cstring(f)
                    parent = read_int(f)
                    f.read(16 * 4)  # Skip inv bind pose
                    parent_name = f"→ {parent}" if parent >= 0 else "ROOT"
                    print(f"        [{i:02d}] {name:20s} {parent_name}")
            
            elif chunk_id == CHUNK_BUFF:
                buffer_count += 1
                mat_idx = read_uint(f)
                flags = read_uint(f)
                skinned = "SKINNED" if (flags & 1) else "STATIC"
                print(f"     └─ Buffer #{buffer_count} | Material: {mat_idx} | {skinned}")
                
                # Continue reading sub-chunks
                while f.tell() < chunk_end:
                    sub_id = read_uint(f)
                    sub_len = read_uint(f)
                    sub_end = f.tell() + sub_len
                    
                    if sub_id == CHUNK_VRTS:
                        num_verts = read_uint(f)
                        total_verts += num_verts
                        print(f"        ├─ Vertices: {num_verts:6,}")
                    elif sub_id == CHUNK_IDXS:
                        num_indices = read_uint(f)
                        total_indices += num_indices
                        print(f"        ├─ Indices:  {num_indices:6,} ({num_indices//3:,} triangles)")
                    elif sub_id == CHUNK_SKIN:
                        num_skin = read_uint(f)
                        print(f"        └─ Skinning: {num_skin:6,} vertices")
                    
                    f.seek(sub_end)
            
            f.seek(chunk_end)
        
        print(f"\n{'='*70}")
        print(f"SUMMARY:")
        print(f"  Total chunks:    {chunk_count}")
        print(f"  Total buffers:   {buffer_count}")
        print(f"  Total vertices:  {total_verts:,}")
        print(f"  Total triangles: {total_indices//3:,}")
        print(f"{'='*70}\n")

def dump_anim(filepath):
    print(f"\n{'='*70}")
    print(f"DUMPING ANIMATION: {filepath}")
    print(f"{'='*70}\n")
    
    import os
    file_size = os.path.getsize(filepath)
    print(f"📦 File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")
    
    with open(filepath, 'rb') as f:
        magic = read_uint(f)
        if magic != ANIM_MAGIC:
            print(f"❌ Invalid magic: 0x{magic:08X}")
            return
        
        version = read_uint(f)
        print(f"✓ Version: {version}\n")
        
        while True:
            try:
                chunk_id = read_uint(f)
                chunk_len = read_uint(f)
            except:
                break
            
            chunk_end = f.tell() + chunk_len
            
            if chunk_id == ANIM_CHUNK_INFO:
                name = f.read(64).decode('utf-8', errors='replace').rstrip('\0')
                duration = read_float(f)
                ticks = read_float(f)
                num_channels = read_uint(f)
                
                print(f"[INFO] Animation: {name}")
                print(f"       Duration: {duration:.2f} ticks @ {ticks:.2f} tps")
                print(f"       Channels: {num_channels}\n")
            
            elif chunk_id == ANIM_CHUNK_CHAN:
                bone_name = read_cstring(f)
                num_keys = read_uint(f)
                
                print(f"[CHAN] Bone: {bone_name:20s} | Keyframes: {num_keys}")
                
                # Read first and last keyframe for reference
                if num_keys > 0:
                    # First keyframe
                    time = read_float(f)
                    pos = (read_float(f), read_float(f), read_float(f))
                    rot = (read_float(f), read_float(f), read_float(f), read_float(f))
                    scale = (read_float(f), read_float(f), read_float(f))
                    
                    print(f"       First key @ {time:.2f}:")
                    print(f"         Pos:   ({pos[0]:7.3f}, {pos[1]:7.3f}, {pos[2]:7.3f})")
                    print(f"         Rot:   ({rot[0]:7.3f}, {rot[1]:7.3f}, {rot[2]:7.3f}, {rot[3]:7.3f})")
                    print(f"         Scale: ({scale[0]:7.3f}, {scale[1]:7.3f}, {scale[2]:7.3f})")
                    
                    if num_keys > 1:
                        # Skip to last
                        f.seek(f.tell() + (num_keys - 2) * (4 + 3*4 + 4*4 + 3*4))
                        
                        time = read_float(f)
                        pos = (read_float(f), read_float(f), read_float(f))
                        rot = (read_float(f), read_float(f), read_float(f), read_float(f))
                        scale = (read_float(f), read_float(f), read_float(f))
                        
                        print(f"       Last key @ {time:.2f}:")
                        print(f"         Pos:   ({pos[0]:7.3f}, {pos[1]:7.3f}, {pos[2]:7.3f})")
                        print(f"         Rot:   ({rot[0]:7.3f}, {rot[1]:7.3f}, {rot[2]:7.3f}, {rot[3]:7.3f})")
                        print(f"         Scale: ({scale[0]:7.3f}, {scale[1]:7.3f}, {scale[2]:7.3f})")
                print()
            
            f.seek(chunk_end)
        
        print(f"{'='*70}\n")

if __name__ == "__main__":
    
    filepath = ""
    
    if filepath.endswith('.mesh'):
        dump_mesh(filepath)
    elif filepath.endswith('.anim'):
        dump_anim(filepath)
    else:
        print("Unknown file type (must be .mesh or .anim)")