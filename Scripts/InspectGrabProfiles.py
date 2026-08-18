import unreal


def inspect_component_variables():
    component_blueprint = unreal.load_asset("/Game/XRFramework/Blueprints/BP_GrabComponent")
    if not component_blueprint:
        raise RuntimeError("Unable to load BP_GrabComponent")

    component_cdo = unreal.get_default_object(component_blueprint.generated_class())
    unreal.log("Grab component reflected members: {}".format(dir(component_cdo)))


def inspect_grab_component(blueprint_path):
    blueprint = unreal.load_asset(blueprint_path)
    if not blueprint:
        raise RuntimeError("Unable to load {}".format(blueprint_path))

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if component and component.get_class().get_name() == "BP_GrabComponent_C":
            values = {}
            for property_name in [
                "grab_type", "grab_mode", "hand_type", "hide_hand", "hide_hand_on_grab",
                "show_hand", "attachment_type", "use_hand_rotation", "rotation_offset",
                "GrabType", "Hand", "PrimaryGrabComponent", "PrimaryGrabRelativeRotation"
            ]:
                try:
                    values[property_name] = str(component.get_editor_property(property_name))
                except Exception:
                    pass
            unreal.log("Grab profile {}: {}".format(blueprint_path, values))


inspect_component_variables()
inspect_grab_component("/Game/XRFramework/Blueprints/BP_Grabbable_SmallCube")
inspect_grab_component("/Game/XRFramework/Blueprints/BP_Pistol")
