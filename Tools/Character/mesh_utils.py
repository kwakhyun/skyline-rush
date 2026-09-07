"""Shared skinning and FBX export used by the current student authoring recipe."""
from __future__ import annotations
import bpy
from pathlib import Path
FBX_OUTPUT=Path("StudentHero.fbx")

def add_armature_skin(
    obj: bpy.types.Object,
    armature: bpy.types.Object,
    weights: dict[str, list[tuple[int, float]]],
) -> None:
    for bone_name, entries in weights.items():
        group = obj.vertex_groups.new(name=bone_name)
        for vertex_index, weight in entries:
            group.add([vertex_index], weight, "REPLACE")
    modifier = obj.modifiers.new(name="Student_Armature", type="ARMATURE")
    modifier.object = armature
    obj.parent = armature

def create_cube_accessory(
    name: str,
    location: tuple[float, float, float],
    scale: tuple[float, float, float],
    rotation: tuple[float, float, float],
    material: bpy.types.Material,
    armature: bpy.types.Object,
    bone_name: str,
    bevel_width: float = 0.012,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(material)
    add_armature_skin(
        obj,
        armature,
        {bone_name: [(vertex.index, 1.0) for vertex in obj.data.vertices]},
    )
    bevel = obj.modifiers.new(name="Hard Surface Bevel", type="BEVEL")
    bevel.width = bevel_width
    bevel.segments = 4
    return obj

def export_fbx(armature: bpy.types.Object, mesh_objects: list[bpy.types.Object]) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    for obj in mesh_objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.export_scene.fbx(
        filepath=str(FBX_OUTPUT),
        use_selection=True,
        object_types={"ARMATURE", "MESH"},
        apply_scale_options="FBX_SCALE_ALL",
        use_space_transform=True,
        bake_space_transform=False,
        add_leaf_bones=False,
        use_armature_deform_only=False,
        mesh_smooth_type="FACE",
        use_tspace=True,
        bake_anim=False,
        path_mode="COPY",
        embed_textures=True,
        axis_forward="-Y",
        axis_up="Z",
    )
