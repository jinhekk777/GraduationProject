import unreal


MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"
TENON_BLUEPRINT_PATH = "/Game/Assembly/YanWei/BP_YanWeiTenon"
TENON_MESH_PATH = "/Game/Mesh/yanwei_sun/SM_yanwei_sun_Tenon"
MORTISE_MESH_PATH = "/Game/Mesh/yanwei_sun/SM_yanwei_sun_Mortise"
GUIDANCE_MATERIAL_PATH = "/Game/Assembly/YanWei/M_YanWeiAssemblyGuide"
SOUND_PATH = "/Game/Assembly/YanWei/Audio/S_YanWeiWoodClick"


def find_actor(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    raise RuntimeError("Missing configured actor: {}".format(label))


def get_component(actor, component_class):
    component = actor.get_component_by_class(component_class)
    if not component:
        raise RuntimeError("{} has no {}.".format(actor.get_name(), component_class.__name__))
    return component


def get_blueprint_grab_component(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if component and component.get_class().get_name() == "BP_GrabComponent_C":
            return component
    raise RuntimeError("{} has no BP_GrabComponent component.".format(blueprint.get_name()))


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
mortise = find_actor("YanWei_Mortise")
tenon = find_actor("YanWei_Tenon")
mortise_mesh = get_component(mortise, unreal.StaticMeshComponent)
tenon_mesh = get_component(tenon, unreal.StaticMeshComponent)
part = get_component(tenon, unreal.AssemblyPartComponent)
target = part.get_editor_property("target_component")
if mortise_mesh.get_editor_property("static_mesh").get_path_name() != MORTISE_MESH_PATH + ".SM_yanwei_sun_Mortise":
    raise RuntimeError("Mortise mesh assignment is incorrect.")
if tenon_mesh.get_editor_property("static_mesh").get_path_name() != TENON_MESH_PATH + ".SM_yanwei_sun_Tenon":
    raise RuntimeError("Tenon mesh assignment is incorrect.")
if not target or target.get_owner() != mortise:
    raise RuntimeError("Tenon target does not reference the YanWei mortise actor.")
if part.get_editor_property("entry_axis") != unreal.AssemblyEntryAxis.LOCAL_X:
    raise RuntimeError("YanWei must use local X as its insertion axis.")
if part.get_editor_property("entry_side") != unreal.AssemblyEntrySide.NEGATIVE_LOCAL_Z:
    raise RuntimeError("YanWei must enter from the negative local-X side.")
if not part.get_editor_property("auto_snap_when_in_range"):
    raise RuntimeError("Automatic controlled snap is not enabled.")
tenon_scale = tenon_mesh.get_world_transform().scale3d
target_scale = target.get_world_transform().scale3d
if not (unreal.MathLibrary.nearly_equal_float_float(tenon_scale.x, target_scale.x, 0.001)
        and unreal.MathLibrary.nearly_equal_float_float(tenon_scale.y, target_scale.y, 0.001)
        and unreal.MathLibrary.nearly_equal_float_float(tenon_scale.z, target_scale.z, 0.001)):
    raise RuntimeError("Tenon and target scales do not match.")
guidance = mortise.get_guidance_volume()
if guidance.get_editor_property("static_mesh").get_path_name() != TENON_MESH_PATH + ".SM_yanwei_sun_Tenon":
    raise RuntimeError("Guidance volume does not use the YanWei tenon mesh.")
if guidance.get_material(0).get_path_name() != GUIDANCE_MATERIAL_PATH + ".M_YanWeiAssemblyGuide":
    raise RuntimeError("YanWei guidance material is not independent.")
if target.get_editor_property("completion_sound").get_path_name() != SOUND_PATH + ".S_YanWeiWoodClick":
    raise RuntimeError("YanWei completion sound is not independent.")
target_location = target.get_world_location()
start_delta = tenon.get_actor_location() - target_location
entry_direction = -target.get_forward_vector()
entry_distance = start_delta.x * entry_direction.x + start_delta.y * entry_direction.y + start_delta.z * entry_direction.z
target_right = target.get_right_vector()
target_up = target.get_up_vector()
lateral_distance = abs(start_delta.x * target_right.x + start_delta.y * target_right.y + start_delta.z * target_right.z)
vertical_distance = abs(start_delta.x * target_up.x + start_delta.y * target_up.y + start_delta.z * target_up.z)
if entry_distance <= 0.0:
    raise RuntimeError("YanWei start pose is not on the configured local-X entry side.")
if lateral_distance > 0.5 or vertical_distance > 0.5:
    raise RuntimeError("YanWei start pose is not aligned before its local-X slide-in motion.")
blueprint = unreal.load_asset(TENON_BLUEPRINT_PATH)
if not blueprint or "SNAP" not in str(get_blueprint_grab_component(blueprint).get_editor_property("GrabType")):
    raise RuntimeError("YanWei tenon does not use the XR SNAP grab mode.")
unreal.log("YanWei validation passed: tenon={}, target={}, axis={}, side={}, entry_distance={:.3f}, lateral={:.3f}, vertical={:.3f}".format(
    tenon.get_name(), target.get_name(), part.get_editor_property("entry_axis"), part.get_editor_property("entry_side"),
    entry_distance, lateral_distance, vertical_distance))
