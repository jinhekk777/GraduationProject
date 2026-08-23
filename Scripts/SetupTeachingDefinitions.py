import unreal

MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"
DEFINITION_FOLDER = "/Game/Teaching/Definitions"


def make_text(value):
    return unreal.TextLibrary.conv_string_to_text(value)


def asset(path):
    result = unreal.load_asset(path)
    if not result:
        raise RuntimeError("Missing asset " + path)
    return result


def actor(label):
    for item in unreal.EditorLevelLibrary.get_all_level_actors():
        if item.get_actor_label() == label:
            return item
    raise RuntimeError("Missing actor " + label)


def part(item):
    result = item.get_component_by_class(unreal.AssemblyPartComponent)
    if not result:
        raise RuntimeError("Missing AssemblyPartComponent on " + item.get_actor_label())
    return result


def definition(name, values):
    path = DEFINITION_FOLDER + "/" + name
    result = unreal.load_asset(path)
    if not result:
        if not unreal.EditorAssetLibrary.does_directory_exist(DEFINITION_FOLDER):
            unreal.EditorAssetLibrary.make_directory(DEFINITION_FOLDER)
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.JointDefinition)
        result = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DEFINITION_FOLDER, unreal.JointDefinition, factory)
    for key, value in values.items():
        result.set_editor_property(key, value)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    return result


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
zhi_tenon = actor("ZhiSun_Tenon")
zhi_mortise = actor("ZhiSun_Mortise")
yanwei_tenon = actor("YanWei_Tenon")
yanwei_mortise = actor("YanWei_Mortise")

zhi = definition("DA_ZhiSunTeaching", {
    "joint_id": "ZhiSun",
    "display_name": make_text("直榫"),
    "selection_description": make_text("选择直榫，看看榫头怎样笔直插进卯眼。"),
    "observation_description": make_text("直榫由榫头和卯眼组成。先让两边的形状对齐，再从卯眼开口的一侧平稳推入。"),
    "observation_narration_text": make_text("直榫由榫头和卯眼组成。先让两边的形状对齐，再从卯眼开口的一侧平稳推入。"),
    "observation_steps": [make_text("观察：榫头的形状要与卯眼开口对齐。"), make_text("观察：保持方向不变，沿着开口平稳推入。")],
    "guided_instruction": make_text("拿起榫头，让它靠近浅金色轮廓。"),
    "guided_steps": [make_text("先让榫头正对卯眼开口。"), make_text("从开口的一侧慢慢靠近。"), make_text("轮廓重合后，保持方向继续推入。")],
    "challenge_instruction": make_text("试着独立完成直榫装配；需要时可以请求提示。"),
    "result_description": make_text("直榫挑战结果已记录。你可以独立重试或切换另一种榫卯。"),
    "observation_unlock_seconds": 8.0,
    "hint_offer_seconds": 30.0,
    "automatic_hint_seconds": 60.0,
    "completion_feedback_seconds": 1.0,
    "tenon_mesh": asset("/Game/Mesh/zhi_sun/SM_zhi_sun_Tenon"),
    "mortise_mesh": asset("/Game/Mesh/zhi_sun/SM_zhi_sun_Mortise"),
})

yanwei = definition("DA_YanWeiTeaching", {
    "joint_id": "YanWeiSun",
    "display_name": make_text("燕尾榫"),
    "selection_description": make_text("选择燕尾榫，看看燕尾形的榫头怎样把两块木料扣在一起。"),
    "observation_description": make_text("燕尾榫的榫头像展开的燕尾。先让宽窄方向对应，再从卯眼开口的一侧平稳推入。"),
    "observation_narration_text": make_text("燕尾榫的榫头像展开的燕尾。先让宽窄方向对应，再从卯眼开口的一侧平稳推入。"),
    "observation_steps": [make_text("观察：燕尾形榫头的宽窄方向要和卯眼一致。"), make_text("观察：对准开口后，从可以进入的一侧平稳推入。")],
    "guided_instruction": make_text("拿起燕尾榫，让它与浅金色轮廓重合。"),
    "guided_steps": [make_text("先确认燕尾形榫头的宽窄方向。"), make_text("让榫头正对卯眼开口。"), make_text("轮廓重合后，保持方向继续推入。")],
    "challenge_instruction": make_text("试着独立完成燕尾榫装配；需要时可以请求提示。"),
    "result_description": make_text("燕尾榫挑战结果已记录。你可以独立重试或切换另一种榫卯。"),
    "observation_unlock_seconds": 8.0,
    "hint_offer_seconds": 30.0,
    "automatic_hint_seconds": 60.0,
    "completion_feedback_seconds": 1.0,
    "tenon_mesh": asset("/Game/Mesh/yanwei_sun/SM_yanwei_sun_Tenon"),
    "mortise_mesh": asset("/Game/Mesh/yanwei_sun/SM_yanwei_sun_Mortise"),
})

manager = None
for item in unreal.EditorLevelLibrary.get_all_level_actors():
    if item.get_actor_label() == "AssemblyTeachingSession":
        manager = item
        break
if not manager:
    manager = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.AssemblySessionManager, unreal.Vector(0.0, 0.0, 0.0))
    manager.set_actor_label("AssemblyTeachingSession")

zhi_binding = unreal.JointSessionBinding()
zhi_binding.set_editor_property("definition", zhi)
zhi_binding.set_editor_property("assembly_part", part(zhi_tenon))
zhi_binding.set_editor_property("assembly_actor", zhi_tenon)
zhi_binding.set_editor_property("presentation_actors", [zhi_tenon, zhi_mortise])
yanwei_binding = unreal.JointSessionBinding()
yanwei_binding.set_editor_property("definition", yanwei)
yanwei_binding.set_editor_property("assembly_part", part(yanwei_tenon))
yanwei_binding.set_editor_property("assembly_actor", yanwei_tenon)
yanwei_binding.set_editor_property("presentation_actors", [yanwei_tenon, yanwei_mortise])
manager.set_editor_property("joint_bindings", [zhi_binding, yanwei_binding])
unreal.EditorLevelLibrary.save_current_level()
unreal.log("Teaching definitions and two session bindings configured on " + manager.get_name())
