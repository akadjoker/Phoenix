#!/usr/bin/env python3

import bpy
import struct
import mathutils

# ============================================================
# CONSTANTS (same as C++)
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

SCALE_FACTOR = 1.0  # Match your C++ scale

# ============================================================
# BINARY WRITERS
# ============================================================
def write_uint(f, value):
    f.write(struct.pack('<I', value))

def write_int(f, value):
    f.write(struct.pack('<i', value))

def write_float(f, value):
    f.write(struct.pack('<f', value))

def write_byte(f, value):
    f.write(struct.pack('<B', value))

def write_cstring(f, text):
    f.write(text.encode('utf-8'))
    f.write(b'\0')

# ============================================================
# MESH EXPORT
# ============================================================
def export_mesh(filepath, obj, armature_obj=None):
    print(f"\n{'='*60}")
    print(f"Exporting Mesh: {filepath}")
    print(f"{'='*60}")
    
    with open(filepath, 'wb') as f:
        write_uint(f, MESH_MAGIC)
        write_uint(f, 1)  # Version
        
        # Materials
        write_materials_chunk(f, obj)
        
        # Skeleton
        if armature_obj:
            write_skeleton_chunk(f, armature_obj)
        
        # Buffers (one per material slot)
        mesh = obj.data
        if len(mesh.materials) > 0:
            for mat_idx, material in enumerate(mesh.materials):
                write_buffer_chunk(f, obj, armature_obj, mat_idx)
        else:
            # No materials - export all as one buffer
            write_buffer_chunk(f, obj, armature_obj, 0)
    
    print(f"✓ Export complete!")

def write_materials_chunk(f, obj):
    # Chunk header
    write_uint(f, CHUNK_MATS)
    chunk_start = f.tell()
    write_uint(f, 0)  # Placeholder for size
    
    # Count materials
    num_mats = len(obj.data.materials) if obj.data.materials else 1
    write_uint(f, num_mats)
    
    if obj.data.materials:
        for mat in obj.data.materials:
            write_cstring(f, mat.name)
            # Diffuse
            write_float(f, mat.diffuse_color[0])
            write_float(f, mat.diffuse_color[1])
            write_float(f, mat.diffuse_color[2])
            # Specular
            write_float(f, 0.5)
            write_float(f, 0.5)
            write_float(f, 0.5)
            # Shininess
            write_float(f, 32.0)
            # No textures for now
            write_byte(f, 0)
    else:
        # Default material
        write_cstring(f, "DefaultMaterial")
        write_float(f, 0.8)
        write_float(f, 0.8)
        write_float(f, 0.8)
        write_float(f, 0.5)
        write_float(f, 0.5)
        write_float(f, 0.5)
        write_float(f, 32.0)
        write_byte(f, 0)
    
    # Update chunk size
    chunk_end = f.tell()
    chunk_size = chunk_end - chunk_start - 4
    f.seek(chunk_start)
    write_uint(f, chunk_size)
    f.seek(chunk_end)
    
    print(f"  ✓ Materials: {num_mats}")

def write_skeleton_chunk(f, armature_obj):
    write_uint(f, CHUNK_SKEL)
    chunk_start = f.tell()
    write_uint(f, 0)  # Placeholder
    
    bones = armature_obj.data.bones
    write_uint(f, len(bones))
    
    bone_index_map = {bone.name: i for i, bone in enumerate(bones)}
    
    for bone in bones:
        # Name
        write_cstring(f, bone.name)
        
        # Parent index
        parent_idx = -1
        if bone.parent:
            parent_idx = bone_index_map.get(bone.parent.name, -1)
        write_int(f, parent_idx)
        
        # Inverse bind pose (from rest pose)
        # Get bone's world matrix in rest pose
        bind_matrix = bone.matrix_local.copy()
        
        try:
            inv_bind = bind_matrix.inverted()
        except:
            inv_bind = mathutils.Matrix.Identity(4)
        
        # Write matrix in COLUMN-MAJOR order (matching C++ CopyMatrix transpose)
        for col in range(4):
            for row in range(4):
                write_float(f, inv_bind[row][col])
        
        print(f"    Bone: {bone.name}, Parent: {parent_idx}")
    
    # Update chunk size
    chunk_end = f.tell()
    chunk_size = chunk_end - chunk_start - 4
    f.seek(chunk_start)
    write_uint(f, chunk_size)
    f.seek(chunk_end)
    
    print(f"  ✓ Skeleton: {len(bones)} bones")

def write_buffer_chunk(f, obj, armature_obj, material_index):
    write_uint(f, CHUNK_BUFF)
    chunk_start = f.tell()
    write_uint(f, 0)  # Placeholder
    
    # Material index
    write_uint(f, material_index)
    
    # Flags
    has_armature = armature_obj is not None and obj.vertex_groups
    flags = 1 if has_armature else 0  # BUFFER_FLAG_SKINNED = 1
    write_uint(f, flags)
    
    # Get mesh data
    mesh = obj.data
    
    # Apply modifiers to get final mesh (but keep original)
    depsgraph = bpy.context.evaluated_depsgraph_get()
    eval_obj = obj.evaluated_get(depsgraph)
    eval_mesh = eval_obj.to_mesh()
    
    # Filter polygons by material
    polys_for_material = [p for p in eval_mesh.polygons if p.material_index == material_index]
    
    if len(polys_for_material) == 0:
        print(f"  ⚠ Buffer #{material_index}: No polygons found!")
        eval_obj.to_mesh_clear()
        # Still write empty chunks to match format
        write_vertices_chunk_filtered(f, eval_mesh, polys_for_material)
        write_indices_chunk_filtered(f, eval_mesh, polys_for_material)
        if has_armature:
            write_skin_chunk_filtered(f, obj, armature_obj, eval_mesh, polys_for_material)
    else:
        print(f"  Buffer #{material_index}: {len(polys_for_material)} polygons, Skinned={has_armature}")
        
        # Vertices chunk (filtered)
        write_vertices_chunk_filtered(f, eval_mesh, polys_for_material)
        
        # Indices chunk (filtered)
        write_indices_chunk_filtered(f, eval_mesh, polys_for_material)
        
        # Skin chunk
        if has_armature:
            write_skin_chunk_filtered(f, obj, armature_obj, eval_mesh, polys_for_material)
    
    eval_obj.to_mesh_clear()
    
    # Update chunk size
    chunk_end = f.tell()
    chunk_size = chunk_end - chunk_start - 4
    f.seek(chunk_start)
    write_uint(f, chunk_size)
    f.seek(chunk_end)

def write_vertices_chunk_filtered(f, mesh, polygons):
    write_uint(f, CHUNK_VRTS)
    chunk_start = f.tell()
    write_uint(f, 0)  # Placeholder
    
    #mesh.calc_normals_split()
    
    # Collect unique vertices used by these polygons
    vert_indices = set()
    for poly in polygons:
        for vert_idx in poly.vertices:
            vert_indices.add(vert_idx)
    
    vert_indices = sorted(vert_indices)
    
    # Create mapping: old_index -> new_index
    vert_remap = {old_idx: new_idx for new_idx, old_idx in enumerate(vert_indices)}
    
    num_verts = len(vert_indices)
    write_uint(f, num_verts)
    
    # Get UVs if available
    uv_layer = mesh.uv_layers.active.data if mesh.uv_layers.active else None
    
    for old_idx in vert_indices:
        vert = mesh.vertices[old_idx]
        
        # Position
        pos = vert.co / SCALE_FACTOR  # Inverse scale
        write_float(f, pos.x)
        write_float(f, pos.y)
        write_float(f, pos.z)
        
        # Normal
        write_float(f, vert.normal.x)
        write_float(f, vert.normal.y)
        write_float(f, vert.normal.z)
        
        # UV (use first occurrence)
        if uv_layer:
            uv = (0.0, 0.0)
            for poly in mesh.polygons:
                for loop_idx in poly.loop_indices:
                    if mesh.loops[loop_idx].vertex_index == old_idx:
                        uv = uv_layer[loop_idx].uv
                        break
            write_float(f, uv[0])
            write_float(f, uv[1])
        else:
            write_float(f, 0.0)
            write_float(f, 0.0)
    
    chunk_end = f.tell()
    chunk_size = chunk_end - chunk_start - 4
    f.seek(chunk_start)
    write_uint(f, chunk_size)
    f.seek(chunk_end)
    
    print(f"    ✓ Vertices: {num_verts}")
    
    return vert_remap

def write_indices_chunk_filtered(f, mesh, polygons):
    write_uint(f, CHUNK_IDXS)
    chunk_start = f.tell()
    write_uint(f, 0)  # Placeholder
    
    # Collect unique vertices to build remap
    vert_indices = set()
    for poly in polygons:
        for vert_idx in poly.vertices:
            vert_indices.add(vert_idx)
    
    vert_indices = sorted(vert_indices)
    vert_remap = {old_idx: new_idx for new_idx, old_idx in enumerate(vert_indices)}
    
    # Count triangles
    num_indices = sum(len(poly.vertices) for poly in polygons if len(poly.vertices) == 3) * 3
    write_uint(f, num_indices)
    
    # Write remapped indices
    for poly in polygons:
        if len(poly.vertices) == 3:
            for old_idx in poly.vertices:
                new_idx = vert_remap[old_idx]
                write_uint(f, new_idx)
    
    chunk_end = f.tell()
    chunk_size = chunk_end - chunk_start - 4
    f.seek(chunk_start)
    write_uint(f, chunk_size)
    f.seek(chunk_end)
    
    print(f"    ✓ Triangles: {num_indices // 3}")

def write_skin_chunk_filtered(f, obj, armature_obj, mesh, polygons):
    write_uint(f, CHUNK_SKIN)
    chunk_start = f.tell()
    write_uint(f, 0)  # Placeholder
    
    # Collect unique vertices
    vert_indices = set()
    for poly in polygons:
        for vert_idx in poly.vertices:
            vert_indices.add(vert_idx)
    
    vert_indices = sorted(vert_indices)
    
    num_verts = len(vert_indices)
    write_uint(f, num_verts)
    
    # Build bone index map
    bone_index_map = {bone.name: i for i, bone in enumerate(armature_obj.data.bones)}
    
    for old_idx in vert_indices:
        vert = mesh.vertices[old_idx]
        
        # Get up to 4 weights
        weights_data = []
        for group in vert.groups:
            if group.weight > 0.001:
                vg = obj.vertex_groups[group.group]
                bone_idx = bone_index_map.get(vg.name, 0)
                weights_data.append((bone_idx, group.weight))
        
        # Sort by weight descending
        weights_data.sort(key=lambda x: x[1], reverse=True)
        weights_data = weights_data[:4]  # Keep top 4
        
        # Normalize
        total = sum(w[1] for w in weights_data)
        if total > 0:
            weights_data = [(idx, w/total) for idx, w in weights_data]
        
        # Pad to 4
        while len(weights_data) < 4:
            weights_data.append((0, 0.0))
        
        # Write bone IDs
        for bone_idx, _ in weights_data:
            write_byte(f, bone_idx)
        
        # Write weights
        for _, weight in weights_data:
            write_float(f, weight)
    
    chunk_end = f.tell()
    chunk_size = chunk_end - chunk_start - 4
    f.seek(chunk_start)
    write_uint(f, chunk_size)
    f.seek(chunk_end)
    
    print(f"    ✓ Skinned: {num_verts} vertices")

# ============================================================
# ANIMATION EXPORT
# ============================================================
def export_animation(filepath, armature_obj, action_name=None):
    print(f"\n{'='*60}")
    print(f"Exporting Animation: {filepath}")
    print(f"{'='*60}")
    
    if not armature_obj or armature_obj.type != 'ARMATURE':
        print("Error: No armature selected!")
        return False
    
    if not armature_obj.animation_data or not armature_obj.animation_data.action:
        print("Error: No animation data!")
        return False
    
    action = armature_obj.animation_data.action
    
    with open(filepath, 'wb') as f:
        write_uint(f, ANIM_MAGIC)
        write_uint(f, 1)  # Version
        
        # Info chunk
        write_animation_info_chunk(f, action, armature_obj)
        
        # Channel chunks
        write_animation_channels(f, action, armature_obj)
    
    print(f"✓ Export complete!")
    return True

def write_animation_info_chunk(f, action, armature_obj):
    write_uint(f, ANIM_CHUNK_INFO)
    chunk_start = f.tell()
    write_uint(f, 0)  # Placeholder
    
    # Name (64 bytes fixed)
    name = action.name[:63].encode('utf-8')
    f.write(name)
    f.write(b'\0' * (64 - len(name)))
    
    # Duration
    frame_start = int(action.frame_range[0])
    frame_end = int(action.frame_range[1])
    fps = bpy.context.scene.render.fps
    duration = (frame_end - frame_start) / fps
    
    write_float(f, duration * fps)  # Duration in ticks
    write_float(f, fps)  # Ticks per second
    
    # Count channels (bones with keyframes)
    num_channels = 0
    for bone in armature_obj.pose.bones:
        bone_path = f'pose.bones["{bone.name}"]'
        has_keys = any(
            fc.data_path.startswith(bone_path) 
            for fc in action.fcurves
        )
        if has_keys:
            num_channels += 1
    
    write_uint(f, num_channels)
    
    chunk_end = f.tell()
    chunk_size = chunk_end - chunk_start - 4
    f.seek(chunk_start)
    write_uint(f, chunk_size)
    f.seek(chunk_end)
    
    print(f"  Animation: {action.name}")
    print(f"    Duration: {duration:.2f}s ({duration * fps:.0f} ticks)")
    print(f"    Channels: {num_channels}")

def write_animation_channels(f, action, armature_obj):
    frame_start = int(action.frame_range[0])
    frame_end = int(action.frame_range[1])
    fps = bpy.context.scene.render.fps
    
    for bone in armature_obj.pose.bones:
        bone_path = f'pose.bones["{bone.name}"]'
        
        # Check if bone has keyframes
        has_keys = any(
            fc.data_path.startswith(bone_path) 
            for fc in action.fcurves
        )
        
        if not has_keys:
            continue
        
        print(f"    Channel: {bone.name}")
        
        write_uint(f, ANIM_CHUNK_CHAN)
        chunk_start = f.tell()
        write_uint(f, 0)  # Placeholder
        
        # Bone name
        write_cstring(f, bone.name)
        
        # Collect keyframes
        keyframes = set()
        for fc in action.fcurves:
            if fc.data_path.startswith(bone_path):
                for kf in fc.keyframe_points:
                    keyframes.add(int(kf.co[0]))
        
        keyframes = sorted(keyframes)
        write_uint(f, len(keyframes))
        
        # Write keyframe data
        for frame in keyframes:
            bpy.context.scene.frame_set(frame)
            
            # Time (in ticks)
            time = (frame - frame_start) / fps * fps
            write_float(f, time)
            
            # CRITICAL: Export in GLOBAL/ABSOLUTE space
            # This is what Assimp exports!
            
            # Get bone's current pose matrix
            pose_matrix = bone.matrix.copy()
            
            # Decompose
            loc, rot, scale = pose_matrix.decompose()
            
            # Position (inverse scale)
            write_float(f, loc.x / SCALE_FACTOR)
            write_float(f, loc.y / SCALE_FACTOR)
            write_float(f, loc.z / SCALE_FACTOR)
            
            # Rotation (x, y, z, w)
            write_float(f, rot.x)
            write_float(f, rot.y)
            write_float(f, rot.z)
            write_float(f, rot.w)
            
            # Scale
            write_float(f, scale.x)
            write_float(f, scale.y)
            write_float(f, scale.z)
        
        chunk_end = f.tell()
        chunk_size = chunk_end - chunk_start - 4
        f.seek(chunk_start)
        write_uint(f, chunk_size)
        f.seek(chunk_end)
        
        print(f"      Keyframes: {len(keyframes)}")

# ============================================================
# UI / MAIN
# ============================================================
def main():
    # Get selected objects
    selected = bpy.context.selected_objects
    
    if not selected:
        print("Error: No objects selected!")
        return
    
    # Find mesh and armature
    mesh_obj = None
    armature_obj = None
    
    for obj in selected:
        if obj.type == 'MESH':
            mesh_obj = obj
        elif obj.type == 'ARMATURE':
            armature_obj = obj
    
    if not mesh_obj:
        print("Error: No mesh selected!")
        return
    
    # Export mesh
    mesh_path = "/home/djoker/projects/cpp/Phoenix/bin/assets/blender_export.mesh"
    export_mesh(mesh_path, mesh_obj, armature_obj)
    
    # Export animation (if armature has animation)
    if armature_obj and armature_obj.animation_data and armature_obj.animation_data.action:
        anim_path = "/home/djoker/projects/cpp/Phoenix/bin/assets/blender_export.anim"
        export_animation(anim_path, armature_obj)
    
    print("\n✅ Export complete!\n")

if __name__ == "__main__":
    main()