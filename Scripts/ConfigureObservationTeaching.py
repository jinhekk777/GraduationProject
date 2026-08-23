import unreal


ASSET_PATHS = (
    "/Game/Teaching/Definitions/DA_ZhiSunTeaching",
    "/Game/Teaching/Definitions/DA_YanWeiTeaching",
)


for asset_path in ASSET_PATHS:
    definition = unreal.load_asset(asset_path)
    if definition is None:
        raise RuntimeError("Missing teaching definition: " + asset_path)

    definition.set_editor_property("observation_narration_text", definition.get_editor_property("observation_description"))
    definition.set_editor_property("observation_post_narration_delay_seconds", 2.0)
    unreal.EditorAssetLibrary.save_loaded_asset(definition)
    unreal.log("Configured observation teaching data for " + asset_path)
