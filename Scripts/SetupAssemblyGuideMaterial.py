import unreal


MATERIAL_PATH = "/Game/Assembly/ZhiSun/M_ZhiSunAssemblyGuide"


def create_or_update_material():
    material = unreal.load_asset(MATERIAL_PATH)
    if not material:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = asset_tools.create_asset(
            "M_ZhiSunAssemblyGuide",
            "/Game/Assembly/ZhiSun",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
        if not material:
            raise RuntimeError("Could not create the assembly guide material.")

    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", True)
    try:
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception:
        pass

    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -500, 0
    )
    color.set_editor_property("parameter_name", "GuideColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.74, 0.56, 0.25, 1.0))

    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -500, 150
    )
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.28)

    # A full transparent volume must remain visible face-on; do not mask it with Fresnel.
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH, only_if_is_dirty=False)
    return material


material = create_or_update_material()
unreal.log("Assembly guide material ready: {}".format(material.get_path_name()))
