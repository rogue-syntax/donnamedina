# Test 01: one UI capture on a plane, slow camera push-in with shallow depth of field and a light
# sweeping across the panel. 3 s at 30 fps, 1920x1080, Eevee. Written for Blender 5.2 LTS.
#
# Run from the project root (no window):
#   blender -b --factory-startup -P video/test01/pushin.py -- --stills   (frames 1, 45, 90 as PNG)
#   blender -b --factory-startup -P video/test01/pushin.py -- --video    (the whole shot as MP4)
# Input:  video/out/ui.png (an EDITOR_SHOT capture, see tests/editor_cycle)
# Output: video/out/test01/

import math
import os
import sys

import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
VIDEO = os.path.dirname(HERE)
UI_IMAGE = os.path.join(VIDEO, "out", "ui.png")
OUT_DIR = os.path.join(VIDEO, "out", "test01")

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
MODE = "video" if "--video" in args else "stills"

FPS = 30
FRAMES = 90


def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def make_panel(img):
    # plane sized to the image (2 m wide), standing up and facing -Y
    w = 2.0
    h = w * img.size[1] / img.size[0]
    bpy.ops.mesh.primitive_plane_add(size=1.0, location=(0, 0, 0), rotation=(math.radians(90), 0, 0))
    panel = bpy.context.active_object
    panel.name = "UI panel"
    panel.scale = (w, h, 1.0)

    mat = bpy.data.materials.new("UI")
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = img
    tex.interpolation = "Cubic"
    # the UI glows like a screen (emission carries the colours); a dark, glossy surface lets the
    # moving light show only as a sheen across it, like glass over a display
    bsdf.inputs["Base Color"].default_value = (0.02, 0.02, 0.025, 1.0)
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Emission Color"])
    bsdf.inputs["Emission Strength"].default_value = 1.0
    bsdf.inputs["Roughness"].default_value = 0.3
    bsdf.inputs["Specular IOR Level"].default_value = 0.5
    panel.data.materials.append(mat)
    return panel, w, h


def make_camera(panel, w, h):
    cam_data = bpy.data.cameras.new("Camera")
    cam = bpy.data.objects.new("Camera", cam_data)
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    cam_data.lens = 50

    # aim at a point left of centre (the oscillator section), so the push-in has a subject
    target = bpy.data.objects.new("Camera target", None)
    bpy.context.scene.collection.objects.link(target)
    target.location = (-0.35, 0, 0.08)
    track = cam.constraints.new("TRACK_TO")
    track.target = target
    track.track_axis = "TRACK_NEGATIVE_Z"
    track.up_axis = "UP_Y"

    # depth of field: focus on the target, wide open so the far side of the panel softens
    cam_data.dof.use_dof = True
    cam_data.dof.focus_object = target
    cam_data.dof.aperture_fstop = 1.4

    # push in from a low three-quarter angle; ease in and out
    start = Vector((1.15, -2.6, -0.35))
    end = Vector((0.45, -1.05, -0.12))
    for f, loc in ((1, start), (FRAMES, end)):
        cam.location = loc
        cam.keyframe_insert("location", frame=f)
    target.location = (-0.30, 0, 0.06)
    target.keyframe_insert("location", frame=1)
    target.location = (-0.45, 0, 0.10)
    target.keyframe_insert("location", frame=FRAMES)
    return cam


def make_lights(w, h):
    # sweep: a long, thin area light crossing the panel left to right
    data = bpy.data.lights.new("Sweep", type="AREA")
    data.shape = "RECTANGLE"
    data.size = 0.25
    data.size_y = 3.0
    data.energy = 25
    data.color = (0.8, 0.92, 1.0)
    data.diffuse_factor = 0.0  # reflection only: no wash over the panel
    sweep = bpy.data.objects.new("Sweep", data)
    bpy.context.scene.collection.objects.link(sweep)
    sweep.rotation_euler = (math.radians(90), 0, 0)  # faces the panel (+Y)
    # constant speed (new keys are linear while these are inserted; the camera keeps its ease)
    edit = bpy.context.preferences.edit
    old = edit.keyframe_new_interpolation_type
    edit.keyframe_new_interpolation_type = "LINEAR"
    for f, x in ((1, -1.6), (FRAMES, 1.6)):
        sweep.location = (x, -2.0, 0.6)
        sweep.keyframe_insert("location", frame=f)
    edit.keyframe_new_interpolation_type = old


def setup_world():
    world = bpy.data.worlds.new("World")
    bpy.context.scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs["Color"].default_value = (0.004, 0.006, 0.012, 1.0)
    bg.inputs["Strength"].default_value = 1.0


def setup_render():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.eevee.taa_render_samples = 64
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.resolution_percentage = 100
    scene.render.fps = FPS
    scene.frame_start = 1
    scene.frame_end = FRAMES
    # Standard view transform: the UI's colours come out as designed (AgX would desaturate them)
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    os.makedirs(OUT_DIR, exist_ok=True)


def render_stills():
    scene = bpy.context.scene
    scene.render.image_settings.file_format = "PNG"
    for f in (1, FRAMES // 2, FRAMES):
        scene.frame_set(f)
        scene.render.filepath = os.path.join(OUT_DIR, "still_%03d.png" % f)
        bpy.ops.render.render(write_still=True)
        print("wrote", scene.render.filepath)


def render_video():
    scene = bpy.context.scene
    s = scene.render.image_settings
    s.media_type = "VIDEO"
    s.file_format = "FFMPEG"
    scene.render.ffmpeg.format = "MPEG4"
    scene.render.ffmpeg.codec = "H264"
    scene.render.ffmpeg.constant_rate_factor = "HIGH"
    scene.render.filepath = os.path.join(OUT_DIR, "pushin_")
    bpy.ops.render.render(animation=True)
    print("wrote", OUT_DIR)


def main():
    if not os.path.exists(UI_IMAGE):
        raise SystemExit("missing %s (take an EDITOR_SHOT capture first)" % UI_IMAGE)
    clear_scene()
    img = bpy.data.images.load(UI_IMAGE)
    panel, w, h = make_panel(img)
    make_camera(panel, w, h)
    make_lights(w, h)
    setup_world()
    setup_render()
    render_video() if MODE == "video" else render_stills()


main()
