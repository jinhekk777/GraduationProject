import unreal


MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"


def find_actor(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    raise RuntimeError("Missing configured actor: {}".format(label))


def get_component(actor, component_class):
    component = actor.get_component_by_class(component_class)
    if not component:
        raise RuntimeError("{} is missing {}.".format(actor.get_name(), component_class.__name__))
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
mortise = find_actor("ZhiSun_Mortise")
tenon = find_actor("ZhiSun_Tenon")
mortise_mesh = get_component(mortise, unreal.StaticMeshComponent)
tenon_mesh = get_component(tenon, unreal.StaticMeshComponent)
guidance_volume = mortise.get_guidance_volume()
assembly_part = get_component(tenon, unreal.AssemblyPartComponent)
target = assembly_part.get_editor_property("target_component")

if mortise_mesh.get_editor_property("static_mesh").get_path_name() != "/Game/Mesh/zhi_sun/SM_zhi_sun_Mortise.SM_zhi_sun_Mortise":
    raise RuntimeError("Mortise mesh assignment is incorrect.")
if tenon_mesh.get_editor_property("static_mesh").get_path_name() != "/Game/Mesh/zhi_sun/SM_zhi_sun_Tenon.SM_zhi_sun_Tenon":
    raise RuntimeError("Tenon mesh assignment is incorrect.")
if guidance_volume.get_editor_property("static_mesh").get_path_name() != "/Game/Mesh/zhi_sun/SM_zhi_sun_Tenon.SM_zhi_sun_Tenon":
    raise RuntimeError("Guidance volume does not match the tenon mesh.")
if not target or target.get_owner() != mortise:
    raise RuntimeError("Tenon target does not reference the configured mortise actor.")
completion_sound = target.get_editor_property("completion_sound")
if not completion_sound or completion_sound.get_path_name() != "/Game/Assembly/ZhiSun/Audio/S_ZhiSunWoodClick.S_ZhiSunWoodClick":
    raise RuntimeError("Completion sound is not configured.")
if not assembly_part.get_editor_property("auto_snap_when_in_range"):
    raise RuntimeError("Automatic controlled snap is not enabled.")
snap_distance_cm = assembly_part.get_editor_property("snap_distance_cm")
if not unreal.MathLibrary.nearly_equal_float_float(snap_distance_cm, 10.0, 0.01):
    raise RuntimeError("Snap distance is not configured for VR testing: {}".format(snap_distance_cm))

tenon_blueprint = unreal.load_asset("/Game/Assembly/ZhiSun/BP_ZhiSunTenon")
if not tenon_blueprint:
    raise RuntimeError("Tenon blueprint is missing.")
grab_type = str(get_blueprint_grab_component(tenon_blueprint).get_editor_property("GrabType"))
if "SNAP" not in grab_type:
    raise RuntimeError("Tenon grab type is not SNAP: {}".format(grab_type))

unreal.log("ZhiSun validation passed: mortise={}, tenon={}, target={}, grab_type={}".format(mortise.get_name(), tenon.get_name(), target.get_name(), grab_type))
