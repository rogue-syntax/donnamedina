# Import a film tool render (video\film\bin\film.exe) into the current Blender scene.
# Written for Blender 5.2 LTS. Never shipped; see docs/08-video.md.
#
# What it makes, in a collection named after the film folder:
#   - "UI": an empty holding one plane per layer (base, displays, art, controls, overlays), each an
#     image sequence with straight alpha, bottom layer at the back. The planes are 2 m wide (--width),
#     stand up facing -Y, and sit --gap metres apart (move or animate them to pull the layers apart).
#   - "Film data": an empty with every data.json channel as a custom property, keyed on every frame
#     (e.g. ["audio.rms"], ["note.60.on"], ["playhead.0.env1"], ["param.42.FILTER__1"]); constant
#     channels are plain properties. Drive anything from them (right-click a value > Add Driver,
#     a Single Property variable on "Film data" with path ["audio.rms"]).
#   - audio.wav as a sound strip in the sequencer at the first frame.
#   - scene fps and frame range; a straight-on orthographic "Flat camera" if the scene has no camera.
# Film frame N is scene frame --frame-start + N (default 1).
#
# Run (from the project root):
#   blender -P video/blender/import_film.py -- video/out/film/demo          (opens Blender with it)
#   blender -b -P video/blender/import_film.py -- video/out/film/demo --save video/out/demo.blend
# Options after "--": FILM_DIR [--width M] [--gap M] [--frame-start F] [--raw] [--match-resolution]
#   [--save FILE.blend] [--render-still FRAME OUT.png] (the last two are for checks from the command line)
#
# Colour: by default the textures are sRGB and the view transform is Standard. Blender then blends the
# layers' soft edges and glows in linear light, a little brighter than the plugin (NanoVG blends the
# sRGB numbers); a flat render differs by about 7/255 on average there, flat areas are exact.
# --raw reads the PNGs as plain numbers and sets the Raw view transform: the layers then blend as in
# the plugin (flat render within 1/255), but lights and other objects in the scene also work on sRGB
# numbers instead of light. A one-pass film (film.exe --layers all) is exact either way.
# From Blender's Text editor: set FILM_DIR below and Run Script.

import json
import os
import sys

import bpy

FILM_DIR = ""  # used when the script is run from the Text editor without "--" arguments

CONSTANT_SUFFIXES = (".on", ".held", ".velocity", ".pitch", "notesHeld")


def parse_args():
    a = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    o = {"dir": FILM_DIR, "width": 2.0, "gap": 0.002, "frame_start": 1, "match": False, "raw": False,
         "save": "", "still": None}
    i = 0
    while i < len(a):
        k = a[i]
        if k == "--width":
            o["width"] = float(a[i + 1]); i += 1
        elif k == "--gap":
            o["gap"] = float(a[i + 1]); i += 1
        elif k == "--frame-start":
            o["frame_start"] = int(a[i + 1]); i += 1
        elif k == "--raw":
            o["raw"] = True
        elif k == "--match-resolution":
            o["match"] = True
        elif k == "--save":
            o["save"] = a[i + 1]; i += 1
        elif k == "--render-still":
            o["still"] = (int(a[i + 1]), a[i + 2]); i += 2
        elif k.startswith("--"):
            raise SystemExit("unknown option " + k)
        else:
            o["dir"] = k
        i += 1
    if not o["dir"]:
        raise SystemExit("give the film folder: blender -P import_film.py -- video/out/film/NAME")
    o["dir"] = os.path.abspath(o["dir"])
    return o


def link(obj, coll):
    coll.objects.link(obj)
    return obj


def layer_material(name, first_png, frames, frame_start, raw):
    img = bpy.data.images.load(first_png, check_existing=False)
    img.name = name
    img.source = "SEQUENCE"
    img.alpha_mode = "STRAIGHT"
    # --raw: the sRGB numbers go through unconverted, so layers blend on them as NanoVG does
    img.colorspace_settings.name = "Non-Color" if raw else "sRGB"

    mat = bpy.data.materials.new(name)
    if mat.node_tree is None:  # before Blender 5 materials start without nodes
        mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    tex = nt.nodes.new("ShaderNodeTexImage")
    emit = nt.nodes.new("ShaderNodeEmission")
    clear = nt.nodes.new("ShaderNodeBsdfTransparent")
    mix = nt.nodes.new("ShaderNodeMixShader")
    tex.image = img
    tex.interpolation = "Linear"
    tex.extension = "CLIP"
    u = tex.image_user
    u.frame_duration = frames
    u.frame_start = frame_start
    u.frame_offset = -1  # files start at 00000
    u.use_auto_refresh = True
    # the UI shines like a screen: emission strength 1 gives its colours unchanged under the
    # Standard view transform; alpha mixes to transparent
    nt.links.new(tex.outputs["Color"], emit.inputs["Color"])
    emit.inputs["Strength"].default_value = 1.0
    nt.links.new(tex.outputs["Alpha"], mix.inputs["Fac"])
    nt.links.new(clear.outputs[0], mix.inputs[1])
    nt.links.new(emit.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], out.inputs["Surface"])
    tex.location, emit.location, clear.location = (-500, 0), (-200, -100), (-200, 100)
    mix.location, out.location = (0, 0), (200, 0)
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = "BLENDED"  # exact alpha over the layers behind
    mat.use_backface_culling = False
    return mat


def layer_plane(name, mat, w, h, y, coll, parent):
    me = bpy.data.meshes.new(name)
    # standing up in XZ, facing -Y; UV 0..1 with V up
    me.from_pydata([(-w / 2, 0, -h / 2), (w / 2, 0, -h / 2), (w / 2, 0, h / 2), (-w / 2, 0, h / 2)],
                   [], [(0, 1, 2, 3)])
    uv = me.uv_layers.new(name="UVMap")
    for li, (s, t) in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
        uv.data[li].uv = (s, t)
    me.materials.append(mat)
    ob = link(bpy.data.objects.new(name, me), coll)
    ob.location = (0, y, 0)
    ob.parent = parent
    return ob


def channelbag_for(ob, action):
    # Blender 4.4+ layered actions: the F-curves live in a channel bag for the object's slot
    try:
        from bpy_extras import anim_utils
        slot = action.slots.new(id_type="OBJECT", name=ob.name)
        ob.animation_data.action_slot = slot
        return anim_utils.action_ensure_channelbag_for_slot(action, slot).fcurves
    except (ImportError, AttributeError):
        return action.fcurves  # older Blender


def data_empty(data, coll, frame_start):
    ob = link(bpy.data.objects.new("Film data", None), coll)
    ob.empty_display_size = 0.1
    ob.location = (0, 0, -0.8)
    channels = data.get("channels", {})
    for k, v in data.get("constant", {}).items():
        ob[k] = float(v)
    for k in channels:
        ob[k] = float(channels[k][0]) if channels[k] else 0.0
    ob.animation_data_create()
    action = bpy.data.actions.new("Film data")
    ob.animation_data.action = action
    fcurves = channelbag_for(ob, action)
    for k, vals in channels.items():
        path = '["%s"]' % k.replace("\\", "\\\\").replace('"', '\\"')
        fc = fcurves.new(data_path=path)
        n = len(vals)
        fc.keyframe_points.add(n)
        co = [0.0] * (2 * n)
        co[0::2] = [frame_start + i for i in range(n)]
        co[1::2] = [float(x) for x in vals]
        fc.keyframe_points.foreach_set("co", co)
        interp = "CONSTANT" if k.endswith(CONSTANT_SUFFIXES) else "LINEAR"
        for kp in fc.keyframe_points:
            kp.interpolation = interp
        fc.update()
    return ob


def add_audio(scene, wav, frame_start):
    if not os.path.exists(wav):
        return None
    ed = scene.sequence_editor or scene.sequence_editor_create()
    strips = ed.strips if hasattr(ed, "strips") else ed.sequences  # "sequences" before Blender 5
    return strips.new_sound("Synth audio", wav, 1, frame_start)


def flat_camera(scene, coll, w, h, res_w, res_h):
    cam_data = bpy.data.cameras.new("Flat camera")
    cam_data.type = "ORTHO"
    cam_data.sensor_fit = "HORIZONTAL" if w / h >= res_w / res_h else "VERTICAL"
    cam_data.ortho_scale = w if cam_data.sensor_fit == "HORIZONTAL" else h * res_w / res_h
    cam = link(bpy.data.objects.new("Flat camera", cam_data), coll)
    cam.location = (0, -3, 0)
    cam.rotation_euler = (1.5707963267948966, 0, 0)
    scene.camera = cam
    return cam


def main():
    o = parse_args()
    with open(os.path.join(o["dir"], "data.json"), "r", encoding="utf-8") as f:
        data = json.load(f)
    frames = int(data["frames"])
    fs = o["frame_start"]
    layers = data["layers"]
    pw, ph = int(data["width"]), int(data["height"])
    w = o["width"]
    h = w * ph / pw

    scene = bpy.context.scene
    scene.render.fps = int(round(data["fps"]))
    scene.render.fps_base = scene.render.fps / float(data["fps"])
    scene.frame_start = fs
    scene.frame_end = fs + frames - 1
    # the UI's colours come out as designed (AgX would change them); --raw outputs the numbers as
    # they are (the textures already hold sRGB values)
    scene.view_settings.view_transform = "Raw" if o["raw"] else "Standard"
    scene.view_settings.look = "None"

    coll = bpy.data.collections.new(os.path.basename(o["dir"]))
    scene.collection.children.link(coll)
    rig = link(bpy.data.objects.new("UI", None), coll)
    rig.empty_display_type = "PLAIN_AXES"
    rig.empty_display_size = 0.2
    for i, name in enumerate(layers):
        first = os.path.join(o["dir"], name, "00000.png")
        if not os.path.exists(first):
            raise SystemExit("missing " + first)
        mat = layer_material("UI " + name, first, frames, fs, o["raw"])
        layer_plane("UI " + name, mat, w, h, -i * o["gap"], coll, rig)

    data_empty(data, coll, fs)
    add_audio(scene, os.path.join(o["dir"], "audio.wav"), fs)

    if o["match"]:
        scene.render.resolution_x, scene.render.resolution_y = pw, ph
        scene.render.resolution_percentage = 100
    if scene.camera is None:
        flat_camera(scene, coll, w, h, scene.render.resolution_x, scene.render.resolution_y)
    scene.frame_set(fs)
    print("imported %s: %d layers, %d frames at %g fps, %d data channels"
          % (o["dir"], len(layers), frames, data["fps"], len(data.get("channels", {}))))

    if o["save"]:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(o["save"]))
    if o["still"]:
        frame, path = o["still"]
        scene.frame_set(frame)
        scene.render.image_settings.file_format = "PNG"
        scene.render.image_settings.color_mode = "RGBA"
        scene.render.film_transparent = True
        scene.render.filepath = os.path.abspath(path)
        bpy.ops.render.render(write_still=True)
        print("wrote", scene.render.filepath)


main()
