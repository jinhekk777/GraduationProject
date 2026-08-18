import unreal


MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"
LABELS = ["ZhiSun_Mortise", "ZhiSun_Tenon", "YanWei_Mortise", "YanWei_Tenon"]


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if actor.get_actor_label() not in LABELS:
        continue
    values = {"actor": str(actor.get_actor_transform())}
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if mesh:
        values["mesh"] = str(mesh.get_world_transform())
        static_mesh = mesh.get_editor_property("static_mesh")
        values["asset"] = static_mesh.get_path_name() if static_mesh else "None"
    part = actor.get_component_by_class(unreal.AssemblyPartComponent)
    if part:
        target = part.get_editor_property("target_component")
        values["part"] = {
            "entry_axis": str(part.get_editor_property("entry_axis")),
            "entry_side": str(part.get_editor_property("entry_side")),
            "target": str(target.get_world_transform()) if target else "None",
        }
    if isinstance(actor, unreal.AssemblyMortiseActor):
        values["target"] = str(actor.get_assembly_target_component().get_world_transform())
    unreal.log("Assembly instance {}: {}".format(actor.get_actor_label(), values))
