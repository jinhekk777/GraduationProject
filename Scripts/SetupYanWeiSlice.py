import unreal


MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"
SOURCE_GRABBABLE_PATH = "/Game/XRFramework/Blueprints/BP_Grabbable_SmallCube"
PISTOL_BLUEPRINT_PATH = "/Game/XRFramework/Blueprints/BP_Pistol"
TENON_BLUEPRINT_PATH = "/Game/Assembly/YanWei/BP_YanWeiTenon"
TENON_MESH_PATH = "/Game/Mesh/yanwei_sun/SM_yanwei_sun_Tenon"
MORTISE_MESH_PATH = "/Game/Mesh/yanwei_sun/SM_yanwei_sun_Mortise"
GUIDANCE_SOURCE_PATH = "/Game/Assembly/ZhiSun/M_ZhiSunAssemblyGuide"
GUIDANCE_MATERIAL_PATH = "/Game/Assembly/YanWei/M_YanWeiAssemblyGuide"
SOUND_SOURCE_PATH = "/Game/Assembly/ZhiSun/Audio/S_ZhiSunWoodClick"
COMPLETION_SOUND_PATH = "/Game/Assembly/YanWei/Audio/S_YanWeiWoodClick"
MORTISE_LABEL = "YanWei_Mortise"
TENON_LABEL = "YanWei_Tenon"


def require_asset(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("Required asset was not found: {}".format(path))
    return asset


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def duplicate_if_missing(source_path, destination_path):
    existing = unreal.load_asset(destination_path)
    if existing:
        return existing
    ensure_directory(destination_path.rsplit("/", 1)[0])
    duplicated = unreal.EditorAssetLibrary.duplicate_asset(source_path, destination_path)
    if not duplicated:
        raise RuntimeError("Unable to duplicate {} to {}".format(source_path, destination_path))
    return duplicated


def find_actor(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


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


def get_or_create_tenon_blueprint():
    blueprint = unreal.load_asset(TENON_BLUEPRINT_PATH)
    if not blueprint:
        ensure_directory("/Game/Assembly/YanWei")
        blueprint = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_GRABBABLE_PATH, TENON_BLUEPRINT_PATH)
        if not blueprint:
            raise RuntimeError("Could not duplicate the XR template grabbable blueprint.")

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
    assembly_component = None
    for handle in handles:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if component and isinstance(component, unreal.AssemblyPartComponent):
            assembly_component = component
            break

    if not assembly_component:
        params = unreal.AddNewSubobjectParams()
        params.parent_handle = handles[0]
        params.new_class = unreal.AssemblyPartComponent
        params.blueprint_context = blueprint
        component_handle, failure_reason = subsystem.add_new_subobject(params)
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(component_handle)
        assembly_component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if not assembly_component:
            raise RuntimeError("Could not add AssemblyPartComponent: {}".format(failure_reason))

    assembly_component.set_editor_property("auto_snap_when_in_range", True)
    assembly_component.set_editor_property("entry_axis", unreal.AssemblyEntryAxis.LOCAL_Y)
    assembly_component.set_editor_property("entry_side", unreal.AssemblyEntrySide.NEGATIVE_LOCAL_Z)
    assembly_component.set_editor_property("guidance_length_cm", 45.0)
    assembly_component.set_editor_property("approach_rearm_distance_cm", 18.0)
    assembly_component.set_editor_property("lateral_tolerance_cm", 4.0)
    assembly_component.set_editor_property("rotation_tolerance_degrees", 25.0)
    assembly_component.set_editor_property("snap_distance_cm", 5.0)
    assembly_component.set_editor_property("allow_reassembly_after_completion", True)
    assembly_component.set_editor_property("hide_xr_hand_when_held", True)

    source_grab = get_blueprint_grab_component(require_asset(PISTOL_BLUEPRINT_PATH))
    tenon_grab = get_blueprint_grab_component(blueprint)
    tenon_grab.set_editor_property("GrabType", source_grab.get_editor_property("GrabType"))

    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if component and isinstance(component, unreal.StaticMeshComponent):
            component.set_editor_property("static_mesh", require_asset(TENON_MESH_PATH))
            break

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_asset(TENON_BLUEPRINT_PATH, only_if_is_dirty=False)
    return blueprint


def configure_static_mesh(actor, mesh):
    get_component(actor, unreal.StaticMeshComponent).set_editor_property("static_mesh", mesh)


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
tenon_mesh = require_asset(TENON_MESH_PATH)
mortise_mesh = require_asset(MORTISE_MESH_PATH)
guide_material = duplicate_if_missing(GUIDANCE_SOURCE_PATH, GUIDANCE_MATERIAL_PATH)
completion_sound = duplicate_if_missing(SOUND_SOURCE_PATH, COMPLETION_SOUND_PATH)
get_or_create_tenon_blueprint()
tenon_class = unreal.EditorAssetLibrary.load_blueprint_class(TENON_BLUEPRINT_PATH)
if not tenon_class:
    raise RuntimeError("Could not load the generated YanWei tenon blueprint class.")

zhi_sun_mortise = find_actor("ZhiSun_Mortise")
base_location = zhi_sun_mortise.get_actor_location() + unreal.Vector(110.0, 0.0, 0.0) if zhi_sun_mortise else unreal.Vector(250.0, 110.0, 120.0)
mortise_actor = find_actor(MORTISE_LABEL)
if not mortise_actor:
    mortise_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.AssemblyMortiseActor, base_location)
    mortise_actor.set_actor_label(MORTISE_LABEL)
tenon_actor = find_actor(TENON_LABEL)
if not tenon_actor:
    tenon_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(tenon_class, base_location + unreal.Vector(0.0, -35.0, 0.0))
    tenon_actor.set_actor_label(TENON_LABEL)

tenon_actor.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
configure_static_mesh(mortise_actor, mortise_mesh)
configure_static_mesh(tenon_actor, tenon_mesh)
guidance_volume = mortise_actor.get_guidance_volume()
guidance_volume.set_editor_property("static_mesh", tenon_mesh)
guidance_volume.set_material(0, guide_material)
target_component = mortise_actor.get_assembly_target_component()
target_component.set_editor_property("completion_sound", completion_sound)
target_component.set_editor_property("completion_sound_volume", 0.8)
assembly_component = get_component(tenon_actor, unreal.AssemblyPartComponent)
assembly_component.set_editor_property("target_component", target_component)
assembly_component.set_editor_property("entry_axis", unreal.AssemblyEntryAxis.LOCAL_Y)
assembly_component.set_editor_property("entry_side", unreal.AssemblyEntrySide.NEGATIVE_LOCAL_Z)
assembly_component.set_editor_property("auto_snap_when_in_range", True)
assembly_component.set_editor_property("guidance_length_cm", 45.0)
assembly_component.set_editor_property("approach_rearm_distance_cm", 18.0)
assembly_component.set_editor_property("lateral_tolerance_cm", 4.0)
assembly_component.set_editor_property("rotation_tolerance_degrees", 25.0)
assembly_component.set_editor_property("snap_distance_cm", 5.0)
unreal.EditorLevelLibrary.save_current_level()
unreal.log("YanWei slice configured: {} -> {}; final={}, start={}".format(
	tenon_actor.get_name(), mortise_actor.get_name(), target_component.get_world_transform(), tenon_actor.get_actor_transform()))
