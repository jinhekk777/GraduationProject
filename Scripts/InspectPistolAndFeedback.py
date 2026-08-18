import unreal


PISTOL_PATH = "/Game/XRFramework/Blueprints/BP_Pistol"
MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"


def log_component_properties(component):
    values = {}
    for name in [
        "grab_type", "hide_hand", "hide_hand_on_grab", "show_hand", "hand_type",
        "motion_controller_ref", "motion_controller", "hand", "controller_hand",
    ]:
        try:
            values[name] = str(component.get_editor_property(name))
        except Exception:
            pass
    unreal.log("Component {} ({}) properties: {}".format(
        component.get_name(), component.get_class().get_name(), values))


def inspect_blueprint_graphs(blueprint):
    graphs = []
    for property_name in ["ubergraph_pages", "function_graphs", "macro_graphs"]:
        try:
            graphs.extend(blueprint.get_editor_property(property_name))
        except Exception:
            pass

    for graph in graphs:
        unreal.log("Graph: {}".format(graph.get_name()))
        for node in graph.get_editor_property("nodes"):
            title = node.get_class().get_name()
            try:
                title = node.get_node_title(unreal.NodeTitleType.LIST_VIEW).to_string()
            except Exception:
                pass
            if any(term in title.lower() for term in ["hand", "grab", "hide", "show", "held"]):
                unreal.log("  Node: {} [{}]".format(title, node.get_class().get_name()))

    unreal.log("Blueprint reflected members: {}".format(
        [name for name in dir(blueprint) if "graph" in name.lower() or "node" in name.lower()]))


def inspect_pistol():
    blueprint = unreal.load_asset(PISTOL_PATH)
    if not blueprint:
        raise RuntimeError("Unable to load pistol blueprint")

    unreal.log("=== Pistol hand-visibility inspection ===")
    inspect_blueprint_graphs(blueprint)
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if component:
            log_component_properties(component)


def inspect_completion_sound():
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    if not world:
        raise RuntimeError("Unable to load XR template map")

    unreal.log("=== Completion sound inspection ===")
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() != "ZhiSun_Mortise":
            continue
        target = actor.get_component_by_class(unreal.AssemblyTargetComponent)
        if not target:
            raise RuntimeError("ZhiSun_Mortise has no AssemblyTargetComponent")
        sound = target.get_editor_property("completion_sound")
        unreal.log("Target={} sound={} valid={}".format(
            target.get_path_name(),
            sound.get_path_name() if sound else "None",
            bool(sound),
        ))
        if sound:
            values = {}
            for property_name in ["duration", "volume", "volume_multiplier", "num_channels", "sample_rate", "num_samples"]:
                try:
                    values[property_name] = str(sound.get_editor_property(property_name))
                except Exception:
                    pass
            unreal.log("Completion sound properties: {}".format(values))
        return
    raise RuntimeError("ZhiSun_Mortise not found")


inspect_pistol()
inspect_completion_sound()
