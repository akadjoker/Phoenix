#!/usr/bin/env python3
 
import bpy
import struct
import mathutils
import math

# ============================================================
# FORMAT CONSTANTS
# ============================================================
MESH_MAGIC = 0x4D455348  # "MESH"
CHUNK_MATS = 0x4D415453  # "MATS"
CHUNK_SKEL = 0x534B454C  # "SKEL"
CHUNK_BUFF = 0x42554646  # "BUFF"
CHUNK_VRTS = 0x56525453  # "VRTS"
CHUNK_IDXS = 0x49445853  # "IDXS"
CHUNK_SKIN = 0x534B494E  # "SKIN"

ANIM_MAGIC = 0x414E494D  # "ANIM"
ANIM_CHUNK_INFO = 0x494E464F  # "INFO"
ANIM_CHUNK_CHAN = 0x4348414E  # "CHAN"

# Conversão OpenGL (Y-up) → Blender (Z-up)
COORD_TRANSFORM = mathutils.Matrix.Rotation(math.radians(90), 4, 'X')

# ============================================================
# BINARY READING HELPERS
# ============================================================
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

# ============================================================
# MESH LOADING
# ============================================================
def load_mesh(filepath):
    print(f"\n{'='*60}")
    print(f"Loading Mesh: {filepath}")
    print(f"{'='*60}")
    
    with open(filepath, 'rb') as f:
        magic = read_uint(f)
        if magic != MESH_MAGIC:
            print(f"❌ Invalid magic: {hex(magic)}")
            return None
        
        version = read_uint(f)
        print(f"✓ Version: {version}")
        
        materials = []
        bones = []
        meshes = []
        
        while True:
            try:
                chunk_id = read_uint(f)
                chunk_len = read_uint(f)
            except:
                break
            
            chunk_end = f.tell() + chunk_len
            
            if chunk_id == CHUNK_MATS:
                materials = read_materials(f)
            elif chunk_id == CHUNK_SKEL:
                bones = read_skeleton(f)
            elif chunk_id == CHUNK_BUFF:
                mesh_data = read_buffer(f, chunk_end)
                meshes.append(mesh_data)
            
            f.seek(chunk_end)
    
    return {'materials': materials, 'bones': bones, 'meshes': meshes}

def read_materials(f):
    num_mats = read_uint(f)
    materials = []
    print(f"\n📦 Materials: {num_mats}")
    
    for i in range(num_mats):
        mat = {
            'name': read_cstring(f),
            'diffuse': (read_float(f), read_float(f), read_float(f)),
            'specular': (read_float(f), read_float(f), read_float(f)),
            'shininess': read_float(f),
            'textures': []
        }
        
        num_layers = read_byte(f)
        for j in range(num_layers):
            mat['textures'].append(read_cstring(f))
        
        materials.append(mat)
        print(f"  [{i}] {mat['name']}")
    
    return materials

def read_skeleton(f):
    num_bones = read_uint(f)
    bones = []
    print(f"\n🦴 Skeleton: {num_bones} bones")
    
    for i in range(num_bones):
        bone = {
            'name': read_cstring(f),
            'parent': read_int(f),
            'inv_bind': [read_float(f) for _ in range(16)]  # Só inverseBindPose
        }
        bones.append(bone)
        parent_info = f" (parent: {bone['parent']})" if bone['parent'] >= 0 else " (root)"
        print(f"  [{i}] {bone['name']}{parent_info}")
    
    return bones

def read_buffer(f, chunk_end):
    mesh = {
        'material_idx': read_uint(f),
        'flags': read_uint(f)
    }
    
    while f.tell() < chunk_end:
        sub_id = read_uint(f)
        sub_len = read_uint(f)
        sub_end = f.tell() + sub_len
        
        if sub_id == CHUNK_VRTS:
            mesh['vertices'] = read_vertices(f)
        elif sub_id == CHUNK_IDXS:
            mesh['indices'] = read_indices(f)
        elif sub_id == CHUNK_SKIN:
            mesh['skin'] = read_skin(f)
        
        f.seek(sub_end)
    
    return mesh

def read_vertices(f):
    num_verts = read_uint(f)
    vertices = []
    for i in range(num_verts):
        v = {
            'pos': (read_float(f), read_float(f), read_float(f)),
            'normal': (read_float(f), read_float(f), read_float(f)),
            'uv': (read_float(f), read_float(f))
        }
        vertices.append(v)
    print(f"  ▸ Vertices: {num_verts}")
    return vertices

def read_indices(f):
    num_indices = read_uint(f)
    indices = [read_uint(f) for _ in range(num_indices)]
    print(f"  ▸ Triangles: {num_indices // 3}")
    return indices

def read_skin(f):
    num_verts = read_uint(f)
    skin_data = []
    for i in range(num_verts):
        skin = {
            'bone_ids': [read_byte(f) for _ in range(4)],
            'weights': [read_float(f) for _ in range(4)]
        }
        skin_data.append(skin)
    print(f"  ▸ Skinned vertices: {num_verts}")
    return skin_data

# ============================================================
# ANIMATION LOADING
# ============================================================
def load_animation(filepath):
    print(f"\n{'='*60}")
    print(f"Loading Animation: {filepath}")
    print(f"{'='*60}")
    
    with open(filepath, 'rb') as f:
        magic = read_uint(f)
        if magic != ANIM_MAGIC:
            print(f"❌ Invalid magic: {hex(magic)}")
            return None
        
        version = read_uint(f)
        print(f"✓ Version: {version}")
        
        anim_info = None
        channels = []
        
        while True:
            try:
                chunk_id = read_uint(f)
                chunk_len = read_uint(f)
            except:
                break
            
            chunk_end = f.tell() + chunk_len
            
            if chunk_id == ANIM_CHUNK_INFO:
                anim_info = read_anim_info(f)
            elif chunk_id == ANIM_CHUNK_CHAN:
                channels.append(read_anim_channel(f))
            
            f.seek(chunk_end)
    
    return {'info': anim_info, 'channels': channels}

def read_anim_info(f):
    info = {
        'name': f.read(64).decode('utf-8', errors='replace').rstrip('\0'),
        'duration': read_float(f),
        'ticks_per_second': read_float(f),
        'num_channels': read_uint(f)
    }
    print(f"\n🎬 Animation: {info['name']}")
    print(f"  Duration: {info['duration']:.2f}s, FPS: {info['ticks_per_second']:.1f}")
    return info

def read_anim_channel(f):
    channel = {
        'bone_name': read_cstring(f),
        'keyframes': []
    }
    
    num_keys = read_uint(f)
    for i in range(num_keys):
        key = {
            'time': read_float(f),
            'position': (read_float(f), read_float(f), read_float(f)),
            'rotation': (read_float(f), read_float(f), read_float(f), read_float(f)),
            'scale': (read_float(f), read_float(f), read_float(f))
        }
        channel['keyframes'].append(key)
    
    print(f"  ▸ {channel['bone_name']}: {num_keys} keyframes")
    return channel

# ============================================================
# BLENDER CREATION - USANDO SÓ INVERSE BIND POSE
# ============================================================
import math

# Conversão OpenGL → Blender + SCALE
SCALE_FACTOR = 1.0  # cm → metros (divide por 100)
COORD_TRANSFORM = mathutils.Matrix.Rotation(math.radians(-90), 4, 'X')

def create_armature_from_animation(bones, anim_data):
    """Cria armature com escala e conversão corretas"""
    print(f"\n🦴 Creating armature from animation...")
    
    armature_data = bpy.data.armatures.new("Armature")
    armature_obj = bpy.data.objects.new("Armature", armature_data)
    bpy.context.collection.objects.link(armature_obj)
    
    bpy.context.view_layer.objects.active = armature_obj
    bpy.ops.object.mode_set(mode='EDIT')
    
    # Mapa bone_name → primeira keyframe
    first_pose = {}
    for channel in anim_data['channels']:
        if channel['keyframes']:
            first_key = min(channel['keyframes'], key=lambda k: abs(k['time']))
            first_pose[channel['bone_name']] = first_key
    
    # Calcular world transforms
    def get_bone_world_matrix(bone_idx, computed=None):
        if computed is None:
            computed = {}
        
        if bone_idx in computed:
            return computed[bone_idx]
        
        bone_data = bones[bone_idx]
        bone_name = bone_data['name']
        
        # Construir local transform da keyframe
        if bone_name in first_pose:
            key = first_pose[bone_name]
            
            # 🔥 APLICAR SCALE FACTOR às posições!
            pos = mathutils.Vector(key['position']) * SCALE_FACTOR
            
            mat_loc = mathutils.Matrix.Translation(pos)
            mat_rot = mathutils.Quaternion((
                key['rotation'][3], key['rotation'][0],
                key['rotation'][1], key['rotation'][2]
            )).to_matrix().to_4x4()
            mat_scale = mathutils.Matrix.Diagonal((*key['scale'], 1.0)).to_4x4()
            
            local_mat = mat_loc @ mat_rot @ mat_scale
        else:
            local_mat = mathutils.Matrix.Identity(4)
        
        # Acumular com parent
        if bone_data['parent'] >= 0:
            parent_world = get_bone_world_matrix(bone_data['parent'], computed)
            world_mat = parent_world @ local_mat
        else:
            # Root: aplicar conversão de coordenadas
            world_mat = COORD_TRANSFORM @ local_mat
        
        computed[bone_idx] = world_mat
        return world_mat
    
    # Criar bones
    edit_bones = []
    for i, bone_data in enumerate(bones):
        bone = armature_data.edit_bones.new(bone_data['name'])
        
        world_mat = get_bone_world_matrix(i)
        
        position = world_mat.to_translation()
        rotation = world_mat.to_3x3()
        
        bone.head = position
        # 🔥 Tail com comprimento proporcional ao scale
        bone.tail = position + (rotation @ mathutils.Vector((0, 0.1 * SCALE_FACTOR, 0)))
        
        edit_bones.append(bone)
    
    # Parents
    for i, bone_data in enumerate(bones):
        if bone_data['parent'] >= 0 and bone_data['parent'] < len(edit_bones):
            edit_bones[i].parent = edit_bones[bone_data['parent']]
            edit_bones[i].use_connect = False
    
    bpy.ops.object.mode_set(mode='OBJECT')
    print(f"  ✓ Created {len(bones)} bones")
    return armature_obj


def create_blender_meshes(mesh_data, armature_obj):
    """Cria meshes com escala correta"""
    print(f"\n🔷 Creating meshes...")
    
    # Materials
    bl_materials = []
    for mat_data in mesh_data['materials']:
        mat = bpy.data.materials.new(name=mat_data['name'])
        mat.diffuse_color = (*mat_data['diffuse'], 1.0)
        mat.use_nodes = True
        bl_materials.append(mat)
    
    # Meshes
    for i, mesh in enumerate(mesh_data['meshes']):
        mesh_name = f"Mesh_{i:03d}"
        mesh_obj = bpy.data.meshes.new(mesh_name)
        obj = bpy.data.objects.new(mesh_name, mesh_obj)
        bpy.context.collection.objects.link(obj)
        
        # 🔥 Aplicar conversão + SCALE aos vértices
        verts = []
        normals = []
        for v in mesh['vertices']:
            pos_scaled = mathutils.Vector(v['pos']) * SCALE_FACTOR
            pos = COORD_TRANSFORM @ pos_scaled
            verts.append((pos.x, pos.y, pos.z))
            
            normal = COORD_TRANSFORM.to_3x3() @ mathutils.Vector(v['normal'])
            normals.append((normal.x, normal.y, normal.z))
        
        # Geometry
        faces = [(mesh['indices'][j], mesh['indices'][j+1], mesh['indices'][j+2]) 
                 for j in range(0, len(mesh['indices']), 3)]
        
        mesh_obj.from_pydata(verts, [], faces)
        mesh_obj.normals_split_custom_set_from_vertices(normals)
        
        # UVs
        uv_layer = mesh_obj.uv_layers.new(name="UVMap")
        for poly in mesh_obj.polygons:
            for loop_idx in poly.loop_indices:
                vert_idx = mesh_obj.loops[loop_idx].vertex_index
                uv_layer.data[loop_idx].uv = mesh['vertices'][vert_idx]['uv']
        
        # Material
        if mesh['material_idx'] < len(bl_materials):
            mesh_obj.materials.append(bl_materials[mesh['material_idx']])
        
        mesh_obj.update()
        
        print(f"  ✓ Created {mesh_name}: {len(verts)} verts, {len(faces)} faces")
        
        # Skinning
        if 'skin' in mesh and armature_obj:
            print(f"    🦴 Applying skinning...")
            
            # Vertex groups
            for bone_data in mesh_data['bones']:
                obj.vertex_groups.new(name=bone_data['name'])
            
            # Weights
            for vert_idx, skin in enumerate(mesh['skin']):
                for j in range(4):
                    weight = skin['weights'][j]
                    if weight > 0.001:
                        bone_idx = skin['bone_ids'][j]
                        if bone_idx < len(mesh_data['bones']):
                            obj.vertex_groups[bone_idx].add([vert_idx], weight, 'REPLACE')
            
            # Armature modifier
            obj.parent = armature_obj
            modifier = obj.modifiers.new(name="Armature", type='ARMATURE')
            modifier.object = armature_obj
            
            print(f"    ✓ Skinned")


def apply_animation(anim_data, armature_obj):
    """Aplica animação com escala correta"""
    print(f"\n🎬 Applying animation...")
    
    info = anim_data['info']
    action = bpy.data.actions.new(name=info['name'])
    
    if not armature_obj.animation_data:
        armature_obj.animation_data_create()
    
    armature_obj.animation_data.action = action
    
    fps = info['ticks_per_second']
    total_frames = int(info['duration'] * fps / info['ticks_per_second'])
    
    bpy.context.scene.frame_start = 0
    bpy.context.scene.frame_end = total_frames
    bpy.context.scene.render.fps = int(fps)
    
    bpy.context.view_layer.objects.active = armature_obj
    bpy.ops.object.mode_set(mode='POSE')
    
    for channel in anim_data['channels']:
        bone_name = channel['bone_name']
        
        if bone_name not in armature_obj.pose.bones:
            continue
        
        pose_bone = armature_obj.pose.bones[bone_name]
        pose_bone.rotation_mode = 'QUATERNION'
        
        for key in channel['keyframes']:
            frame = int(key['time'] * fps / info['ticks_per_second'])
            
            # 🔥 APLICAR SCALE às posições das keyframes!
            pos = mathutils.Vector(key['position']) * SCALE_FACTOR
            
            pose_bone.location = pos
            pose_bone.rotation_quaternion = mathutils.Quaternion((
                key['rotation'][3], key['rotation'][0],
                key['rotation'][1], key['rotation'][2]
            ))
            pose_bone.scale = mathutils.Vector(key['scale'])
            
            pose_bone.keyframe_insert(data_path="location", frame=frame)
            pose_bone.keyframe_insert(data_path="rotation_quaternion", frame=frame)
            pose_bone.keyframe_insert(data_path="scale", frame=frame)
    
    bpy.ops.object.mode_set(mode='OBJECT')
    print(f"  ✓ Animation applied: {total_frames} frames")



def debug_bone_data(bones, anim_data):
    """Mostra informação dos bones e keyframes"""
    print(f"\n{'='*60}")
    print(f"DEBUG: Bone & Animation Data")
    print(f"{'='*60}")
    
    # Keyframes disponíveis
    first_pose = {}
    for channel in anim_data['channels']:
        if channel['keyframes']:
            first_key = min(channel['keyframes'], key=lambda k: abs(k['time']))
            first_pose[channel['bone_name']] = first_key
    
    # Info de cada bone
    for i, bone_data in enumerate(bones):
        print(f"\n[{i}] {bone_data['name']}")
        print(f"  Parent: {bone_data['parent']}")
        
        # Inverse bind pose
        inv_bind = bone_data['inv_bind']
        print(f"  InvBind[0:4]: [{inv_bind[0]:.3f}, {inv_bind[1]:.3f}, {inv_bind[2]:.3f}, {inv_bind[3]:.3f}]")
        
        # Keyframe data
        if bone_data['name'] in first_pose:
            key = first_pose[bone_data['name']]
            print(f"  Keyframe[0]:")
            print(f"    pos: {key['position']}")
            print(f"    rot: {key['rotation']}")
            print(f"    scale: {key['scale']}")
        else:
            print(f"  ⚠ NO KEYFRAME!")
    
    print(f"\n{'='*60}\n")


# ============================================================
# MAIN
# ============================================================
def main():
    import bpy
    
    # ============ LIMPAR CENA ============
    # Força object mode
    if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    
    # Deseleciona tudo primeiro
    for obj in bpy.context.selected_objects:
        obj.select_set(False)
    
    # Agora seleciona e deleta tudo
    for obj in list(bpy.data.objects):
        obj.select_set(True)
        bpy.data.objects.remove(obj, do_unlink=True)
    
    # ============ CARREGAR ============
    mesh_file = "/home/djoker/projects/cpp/Phoenix/bin/assets/anim.mesh"
    anim_file = "/home/djoker/projects/cpp/Phoenix/bin/assets/idle.anim"
    
    mesh_data = load_mesh(mesh_file)
    if not mesh_data:
        return
    
    anim_data = load_animation(anim_file)
    if not anim_data:
        return
    
    debug_bone_data(mesh_data['bones'], anim_data)
    
    # ============ CRIAR ============
    armature_obj = create_armature_from_animation(mesh_data['bones'], anim_data)
    create_blender_meshes(mesh_data, armature_obj)
    #apply_animation(anim_data, armature_obj)
    
    # ============ ENQUADRAR CAMERA ============
    # Seleciona o armature para visualizar
    for obj in bpy.data.objects:
        obj.select_set(False)
    armature_obj.select_set(True)
    bpy.context.view_layer.objects.active = armature_obj
    
    # Enquadra a vista (se estiveres em 3D view)
    for area in bpy.context.screen.areas:
        if area.type == 'VIEW_3D':
            for region in area.regions:
                if region.type == 'WINDOW':
                    override = {'area': area, 'region': region}
                    with bpy.context.temp_override(**override):
                        bpy.ops.view3d.view_all(center=False)
                    break
    
    print(f"\n✅ DONE! Vê o resultado na viewport!")

# Executar
main()
