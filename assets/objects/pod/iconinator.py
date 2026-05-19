from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

# ============================================================================
# SETTINGS - edit these first
# ============================================================================

BLENDER_EXECUTABLE = r"C:\Program Files\Blender Foundation\Blender 4.4\blender.exe"

CAMERA_POSITION = (3.75, -3.75, 2.75)
CAMERA_ROTATION_DEGREES = (58.0, 0.0, 45.0)
ORTHOGRAPHIC_ZOOM_SCALE = 5.0

RENDER_RESOLUTION = (512, 512)
RENDER_ENGINE = "BLENDER_EEVEE_NEXT"

LIGHT_TYPE = "AREA"
LIGHT_STRENGTH = 3500.0
LIGHT_POSITION = (4.5, -4.0, 6.0)

BACKGROUND_TRANSPARENT = True

OUTPUT_NAMING_PREFIX = "icon"
AUTO_NUMBERING_BEHAVIOR = True
OUTPUT_FORMAT = "PNG"

FRAMING_PADDING = 1.15
FRAME_TO_FIT_OBJECT = True
OBJECT_AUTO_CENTERING = True
OBJECT_AUTO_SCALING = True
AUTO_SCALE_TARGET_RADIUS = 1.0

ENABLE_SHADED_MATERIALS = True
USE_TEXTURES_WHEN_AVAILABLE = True

SUPPORTED_EXTENSIONS = {".obj", ".fbx", ".glb", ".gltf"}


def main() -> int:
    if len(sys.argv) < 2:
        print("Drag a model file onto iconinator.py")
        return 1

    model_path = Path(sys.argv[1]).expanduser().resolve()
    if model_path.suffix.lower() not in SUPPORTED_EXTENSIONS:
        print(f"Unsupported model format: {model_path.suffix}")
        return 1

    if not model_path.exists():
        print(f"Model file not found: {model_path}")
        return 1

    blender_executable = Path(BLENDER_EXECUTABLE).expanduser()
    if not blender_executable.exists():
        print(f"Blender executable not found: {blender_executable}")
        return 1

    settings = {
        "camera_position": CAMERA_POSITION,
        "camera_rotation_degrees": CAMERA_ROTATION_DEGREES,
        "orthographic_zoom_scale": ORTHOGRAPHIC_ZOOM_SCALE,
        "render_resolution": RENDER_RESOLUTION,
        "render_engine": RENDER_ENGINE,
        "light_type": LIGHT_TYPE,
        "light_strength": LIGHT_STRENGTH,
        "light_position": LIGHT_POSITION,
        "background_transparent": BACKGROUND_TRANSPARENT,
        "output_naming_prefix": OUTPUT_NAMING_PREFIX,
        "auto_numbering_behavior": AUTO_NUMBERING_BEHAVIOR,
        "output_format": OUTPUT_FORMAT,
        "framing_padding": FRAMING_PADDING,
        "frame_to_fit_object": FRAME_TO_FIT_OBJECT,
        "object_auto_centering": OBJECT_AUTO_CENTERING,
        "object_auto_scaling": OBJECT_AUTO_SCALING,
        "auto_scale_target_radius": AUTO_SCALE_TARGET_RADIUS,
        "enable_shaded_materials": ENABLE_SHADED_MATERIALS,
        "use_textures_when_available": USE_TEXTURES_WHEN_AVAILABLE,
    }

    blender_script = build_blender_script(settings)

    temp_script_path = None
    try:
        with tempfile.NamedTemporaryFile("w", suffix="_iconinator_blender.py", delete=False, encoding="utf-8") as temp_file:
            temp_file.write(blender_script)
            temp_script_path = temp_file.name

        command = [
            str(blender_executable),
            "--background",
            "--python",
            temp_script_path,
            "--",
            str(model_path),
            json.dumps(settings),
        ]

        completed = subprocess.run(command, check=False)
        if completed.returncode != 0:
            print(f"Blender exited with code {completed.returncode}")
            return completed.returncode

        return 0
    finally:
        if temp_script_path:
            try:
                os.remove(temp_script_path)
            except OSError:
                pass


def build_blender_script(settings: dict) -> str:
    settings_json = json.dumps(settings)
    return f'''from __future__ import annotations

import json
import math
import os
import sys
from pathlib import Path

import bpy
import mathutils


SETTINGS = json.loads({settings_json!r})


def argv_after_double_dash() -> list[str]:
    argv = sys.argv
    if "--" in argv:
        return argv[argv.index("--") + 1 :]
    return argv[1:]


def clear_scene() -> None:
    bpy.ops.wm.read_factory_settings(use_empty=True)


def import_model(model_path: Path) -> None:
    ext = model_path.suffix.lower()
    if ext == ".obj":
        if hasattr(bpy.ops.wm, "obj_import"):
            bpy.ops.wm.obj_import(filepath=str(model_path), forward_axis="Y", up_axis="Z")
        else:
            bpy.ops.import_scene.obj(filepath=str(model_path), axis_forward="Y", axis_up="Z")
    elif ext == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(model_path))
    elif ext in {{".glb", ".gltf"}}:
        bpy.ops.import_scene.gltf(filepath=str(model_path))
    else:
        raise RuntimeError(f"Unsupported format: {{ext}}")


def imported_mesh_objects() -> list[bpy.types.Object]:
    return [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]


def ensure_view_layer_active(objects: list[bpy.types.Object]) -> None:
    for obj in bpy.context.scene.objects:
        obj.select_set(False)
    for obj in objects:
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]


def bounds_for_objects(objects: list[bpy.types.Object]) -> tuple[mathutils.Vector, mathutils.Vector]:
    points = []
    for obj in objects:
        for corner in obj.bound_box:
            points.append(obj.matrix_world @ mathutils.Vector(corner))

    if not points:
        raise RuntimeError("No mesh bounds found")

    min_corner = mathutils.Vector((min(v.x for v in points), min(v.y for v in points), min(v.z for v in points)))
    max_corner = mathutils.Vector((max(v.x for v in points), max(v.y for v in points), max(v.z for v in points)))
    return min_corner, max_corner


def center_and_scale_objects(objects: list[bpy.types.Object]) -> tuple[mathutils.Vector, float]:
    min_corner, max_corner = bounds_for_objects(objects)
    center = (min_corner + max_corner) * 0.5
    extents = max_corner - min_corner
    radius = max(extents.x, extents.y, extents.z) * 0.5

    if SETTINGS["object_auto_centering"]:
        for obj in objects:
            obj.location -= center

    if SETTINGS["object_auto_scaling"]:
        target_radius = float(SETTINGS["auto_scale_target_radius"])
        if radius > 0.0:
            scale_factor = target_radius / radius
            for obj in objects:
                obj.scale = obj.scale * scale_factor
            bpy.context.view_layer.update()

    return center, radius


def create_materials() -> None:
    if not SETTINGS["enable_shaded_materials"]:
        return

    def set_socket_value(node: bpy.types.Node, socket_name: str, value: float) -> None:
        socket = node.inputs.get(socket_name)
        if socket is not None:
            socket.default_value = value

    for obj in bpy.context.scene.objects:
        if obj.type != "MESH":
            continue
        for slot in obj.material_slots:
            material = slot.material
            if material is None:
                continue
            material.use_nodes = True
            nodes = material.node_tree.nodes
            links = material.node_tree.links
            principled = nodes.get("Principled BSDF")
            if principled is None:
                continue
            set_socket_value(principled, "Roughness", 0.65)
            set_socket_value(principled, "Specular", 0.25)
            set_socket_value(principled, "Specular IOR Level", 0.35)

            if SETTINGS["use_textures_when_available"]:
                image_nodes = [node for node in nodes if node.type == "TEX_IMAGE" and getattr(node, "image", None)]
                for image_node in image_nodes:
                    if "Vector" in image_node.inputs:
                        texcoord = nodes.new("ShaderNodeTexCoord")
                        mapping = nodes.new("ShaderNodeMapping")
                        links.new(texcoord.outputs["UV"], mapping.inputs["Vector"])
                        links.new(mapping.outputs["Vector"], image_node.inputs["Vector"])
                    break


def create_environment(scene: bpy.types.Scene) -> bpy.types.Object:
    camera_data = bpy.data.cameras.new("IconCamera")
    camera = bpy.data.objects.new("IconCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.location = SETTINGS["camera_position"]
    camera.rotation_euler = tuple(math.radians(v) for v in SETTINGS["camera_rotation_degrees"])
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = SETTINGS["orthographic_zoom_scale"]

    light_data = bpy.data.lights.new(name="IconLight", type=SETTINGS["light_type"])
    light_data.energy = SETTINGS["light_strength"]
    light = bpy.data.objects.new(name="IconLight", object_data=light_data)
    scene.collection.objects.link(light)
    light.location = SETTINGS["light_position"]

    if SETTINGS["light_type"] == "AREA":
        light.data.shape = "DISK"
        light.data.size = 5.0

    bpy.context.scene.render.engine = SETTINGS["render_engine"]
    bpy.context.scene.render.film_transparent = bool(SETTINGS["background_transparent"])
    bpy.context.scene.render.image_settings.file_format = SETTINGS["output_format"]
    bpy.context.scene.render.resolution_x = int(SETTINGS["render_resolution"][0])
    bpy.context.scene.render.resolution_y = int(SETTINGS["render_resolution"][1])
    bpy.context.scene.render.resolution_percentage = 100

    return camera


def frame_camera_to_objects(camera: bpy.types.Object, objects: list[bpy.types.Object]) -> None:
    if not SETTINGS["frame_to_fit_object"]:
        return

    min_corner, max_corner = bounds_for_objects(objects)
    center = (min_corner + max_corner) * 0.5
    extents = max_corner - min_corner
    longest = max(extents.x, extents.y, extents.z)
    if longest <= 0.0:
        longest = 1.0

    padded = longest * float(SETTINGS["framing_padding"])
    camera.location = tuple(SETTINGS["camera_position"])
    camera.data.ortho_scale = max(float(SETTINGS["orthographic_zoom_scale"]), padded)
    direction = mathutils.Vector(camera.location) - center
    if direction.length > 0.0:
        camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def unique_output_path(model_path: Path) -> Path:
    directory = model_path.parent
    prefix = str(SETTINGS["output_naming_prefix"]) or "icon"
    extension = ".png" if SETTINGS["output_format"].upper() == "PNG" else f".{str(SETTINGS['output_format']).lower()}"

    if not SETTINGS["auto_numbering_behavior"]:
        candidate = directory / f"{prefix}{extension}"
        if not candidate.exists():
            return candidate

    index = 0
    while True:
        suffix = "" if index == 0 else str(index)
        candidate = directory / f"{prefix}{suffix}{extension}"
        if not candidate.exists():
            return candidate
        index += 1


def main() -> int:
    args = argv_after_double_dash()
    if not args:
        raise RuntimeError("Missing model path")

    model_path = Path(args[0]).expanduser().resolve()

    clear_scene()
    import_model(model_path)

    objects = imported_mesh_objects()
    if not objects:
        raise RuntimeError("No mesh objects found after import")

    ensure_view_layer_active(objects)
    center_and_scale_objects(objects)
    bpy.context.view_layer.update()

    create_materials()
    camera = create_environment(bpy.context.scene)
    frame_camera_to_objects(camera, objects)

    output_path = unique_output_path(model_path)
    bpy.context.scene.render.filepath = str(output_path)

    bpy.ops.render.render(write_still=True)
    print(f"Generated: {{output_path}}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
'''


if __name__ == "__main__":
    raise SystemExit(main())