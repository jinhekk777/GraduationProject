import unreal


MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"
LABELS = ["ZhiSun_Mortise", "ZhiSun_Tenon", "YanWei_Mortise", "YanWei_Tenon"]


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actor_subsystem.get_all_level_actors():
    part = actor.get_component_by_class(unreal.AssemblyPartComponent)
    if actor.get_actor_label() not in LABELS and not part and not isinstance(actor, unreal.AssemblyMortiseActor):
        continue
    values = {"name": actor.get_name(), "label": actor.get_actor_label(), "class": actor.get_class().get_name(), "actor": str(actor.get_actor_transform())}
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if mesh:
        values["mesh"] = str(mesh.get_world_transform())
        static_mesh = mesh.get_editor_property("static_mesh")
        values["asset"] = static_mesh.get_path_name() if static_mesh else "None"
    if part:
        target = part.get_editor_property("target_component")
        values["part"] = {
            "entry_axis": str(part.get_editor_property("entry_axis")),
            "entry_side": str(part.get_editor_property("entry_side")),
            "target": str(target.get_world_transform()) if target else "None",
            "target_x": str(target.get_forward_vector()) if target else "None",
            "target_y": str(target.get_right_vector()) if target else "None",
            "target_z": str(target.get_up_vector()) if target else "None",
            "delta_from_target": str(actor.get_actor_location() - target.get_world_location()) if target else "None",
        }
    if isinstance(actor, unreal.AssemblyMortiseActor):
        values["target"] = str(actor.get_assembly_target_component().get_world_transform())
    unreal.log("Assembly instance {}: {}".format(actor.get_actor_label(), values))
