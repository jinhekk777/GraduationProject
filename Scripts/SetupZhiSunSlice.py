import unreal


MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"
SOURCE_GRABBABLE_PATH = "/Game/XRFramework/Blueprints/BP_Grabbable_SmallCube"
PISTOL_BLUEPRINT_PATH = "/Game/XRFramework/Blueprints/BP_Pistol"
TENON_BLUEPRINT_PATH = "/Game/Assembly/ZhiSun/BP_ZhiSunTenon"
TENON_MESH_PATH = "/Game/Mesh/zhi_sun/SM_zhi_sun_Tenon"
MORTISE_MESH_PATH = "/Game/Mesh/zhi_sun/SM_zhi_sun_Mortise"
GUIDANCE_MATERIAL_PATH = "/Game/Assembly/ZhiSun/M_ZhiSunAssemblyGuide"
COMPLETION_SOUND_PATH = "/Game/Assembly/ZhiSun/Audio/S_ZhiSunWoodClick"
MORTISE_LABEL = "ZhiSun_Mortise"
TENON_LABEL = "ZhiSun_Tenon"


def require_asset(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("Required asset was not found: {}".format(path))
    return asset


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
    existing = unreal.load_asset(TENON_BLUEPRINT_PATH)
    if existing:
        blueprint = existing
    else:
        destination_folder = "/Game/Assembly/ZhiSun"
        if not unreal.EditorAssetLibrary.does_directory_exist(destination_folder):
            unreal.EditorAssetLibrary.make_directory(destination_folder)
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
        component_data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(component_handle)
        assembly_component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(component_data, blueprint)
        if not assembly_component:
            raise RuntimeError("Could not add AssemblyPartComponent: {}".format(failure_reason))

    assembly_component.set_editor_property("auto_snap_when_in_range", True)
    assembly_component.set_editor_property("guidance_length_cm", 40.0)
    assembly_component.set_editor_property("lateral_tolerance_cm", 8.0)
    assembly_component.set_editor_property("rotation_tolerance_degrees", 45.0)
    assembly_component.set_editor_property("snap_distance_cm", 10.0)

    # Reuse the XR template pistol's SNAP grip mode, while retaining the tenon's own mesh and pose.
    pistol_grab_component = get_blueprint_grab_component(require_asset(PISTOL_BLUEPRINT_PATH))
    tenon_grab_component = get_blueprint_grab_component(blueprint)
    tenon_grab_component.set_editor_property("GrabType", pistol_grab_component.get_editor_property("GrabType"))

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
    static_mesh_component = get_component(actor, unreal.StaticMeshComponent)
    static_mesh_component.set_editor_property("static_mesh", mesh)


def get_test_base_location():
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_class().get_name() == "PlayerStart":
            return actor.get_actor_location() + actor.get_actor_forward_vector() * 250.0 + unreal.Vector(0.0, 0.0, 100.0)
    return unreal.Vector(250.0, 0.0, 120.0)


require_asset(TENON_MESH_PATH)
require_asset(MORTISE_MESH_PATH)
unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)

tenon_blueprint = get_or_create_tenon_blueprint()
tenon_class = unreal.EditorAssetLibrary.load_blueprint_class(TENON_BLUEPRINT_PATH)
if not tenon_class:
    raise RuntimeError("Could not load the generated tenon blueprint class.")

base_location = get_test_base_location()
mortise_actor = find_actor(MORTISE_LABEL)
if not mortise_actor:
    mortise_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.AssemblyMortiseActor, base_location)
    mortise_actor.set_actor_label(MORTISE_LABEL)

tenon_actor = find_actor(TENON_LABEL)
if not tenon_actor:
    tenon_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(tenon_class, base_location + unreal.Vector(0.0, 0.0, 20.0))
    tenon_actor.set_actor_label(TENON_LABEL)

configure_static_mesh(mortise_actor, require_asset(MORTISE_MESH_PATH))
configure_static_mesh(tenon_actor, require_asset(TENON_MESH_PATH))

guidance_volume = mortise_actor.get_guidance_volume()
guidance_volume.set_editor_property("static_mesh", require_asset(TENON_MESH_PATH))
guidance_volume.set_material(0, require_asset(GUIDANCE_MATERIAL_PATH))

target_component = mortise_actor.get_assembly_target_component()
target_component.set_editor_property("completion_sound", require_asset(COMPLETION_SOUND_PATH))
assembly_component = get_component(tenon_actor, unreal.AssemblyPartComponent)
tenon_actor.modify()
assembly_component.modify()
assembly_component.set_editor_property("target_component", target_component)
assembly_component.set_editor_property("auto_snap_when_in_range", True)
assembly_component.set_editor_property("entry_side", unreal.AssemblyEntrySide.POSITIVE_LOCAL_Z)
assembly_component.set_editor_property("guidance_length_cm", 40.0)
assembly_component.set_editor_property("lateral_tolerance_cm", 8.0)
assembly_component.set_editor_property("rotation_tolerance_degrees", 45.0)
assembly_component.set_editor_property("snap_distance_cm", 10.0)

unreal.EditorLevelLibrary.save_current_level()
unreal.log("ZhiSun vertical slice configured: {} -> {}".format(tenon_actor.get_name(), mortise_actor.get_name()))
