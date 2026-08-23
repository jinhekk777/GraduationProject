import unreal


MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"
TENON_BLUEPRINT_PATH = "/Game/Assembly/YanWei/BP_YanWeiTenon"
DEFINITION_PATH = "/Game/Teaching/Definitions/DA_YanWeiTeaching"
TENON_LABEL = "YanWei_Tenon"
MORTISE_LABEL = "YanWei_Mortise"


def find_actor(label):
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    raise RuntimeError("Missing configured actor: {}".format(label))


def get_component(actor, component_class):
    component = actor.get_component_by_class(component_class)
    if not component:
        raise RuntimeError("{} has no {}.".format(actor.get_name(), component_class.__name__))
    return component


def get_blueprint_assembly_component(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if component and isinstance(component, unreal.AssemblyPartComponent):
            return component
    raise RuntimeError("{} has no AssemblyPartComponent.".format(blueprint.get_name()))


def make_text(value):
    return unreal.TextLibrary.conv_string_to_text(value)


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
tenon = find_actor(TENON_LABEL)
mortise = find_actor(MORTISE_LABEL)
part = get_component(tenon, unreal.AssemblyPartComponent)
target = part.get_editor_property("target_component")
if not target or target.get_owner() != mortise:
    raise RuntimeError("YanWei tenon target is not the configured YanWei mortise.")

target_location = target.get_world_location()
old_delta = tenon.get_actor_location() - target_location
old_distance = (old_delta.x * old_delta.x + old_delta.y * old_delta.y + old_delta.z * old_delta.z) ** 0.5
separation = max(old_distance, part.get_editor_property("approach_rearm_distance_cm") + 1.0)
entry_direction = -target.get_forward_vector()
new_location = target_location + entry_direction * separation

part.set_editor_property("entry_axis", unreal.AssemblyEntryAxis.LOCAL_X)
part.set_editor_property("entry_side", unreal.AssemblyEntrySide.NEGATIVE_LOCAL_Z)
tenon.set_actor_location(new_location, False, False)

blueprint = unreal.load_asset(TENON_BLUEPRINT_PATH)
if not blueprint:
    raise RuntimeError("Missing YanWei tenon blueprint.")
blueprint_part = get_blueprint_assembly_component(blueprint)
blueprint_part.set_editor_property("entry_axis", unreal.AssemblyEntryAxis.LOCAL_X)
blueprint_part.set_editor_property("entry_side", unreal.AssemblyEntrySide.NEGATIVE_LOCAL_Z)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
unreal.EditorAssetLibrary.save_asset(TENON_BLUEPRINT_PATH, only_if_is_dirty=False)

definition = unreal.load_asset(DEFINITION_PATH)
if not definition:
    raise RuntimeError("Missing YanWei teaching definition.")
definition.set_editor_property("observation_description", make_text("燕尾榫的榫头像展开的燕尾。先让榫头和槽口处在同一位置，再顺着槽口从侧面滑入。"))
definition.set_editor_property("observation_narration_text", make_text("燕尾榫的榫头像展开的燕尾。先让榫头和槽口处在同一位置，再顺着槽口从侧面滑入。"))
definition.set_editor_property("observation_steps", [make_text("观察：先让燕尾形榫头和槽口对齐。"), make_text("观察：保持对齐，顺着槽口从侧面滑入。")])
definition.set_editor_property("guided_steps", [make_text("先让燕尾形榫头和槽口对齐。"), make_text("保持两块木料平齐。"), make_text("顺着槽口从侧面平稳滑入。")])
unreal.EditorAssetLibrary.save_asset(DEFINITION_PATH, only_if_is_dirty=False)

unreal.EditorLevelLibrary.save_current_level()
unreal.log("Updated YanWei assembly path: old_delta={}, new_delta={}, separation={:.3f}, axis=LocalX, side=Negative".format(
    old_delta, tenon.get_actor_location() - target_location, separation))
