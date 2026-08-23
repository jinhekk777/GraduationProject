import unreal


def make_text(value):
    return unreal.TextLibrary.conv_string_to_text(value)


def update_definition(path, values):
    definition = unreal.load_asset(path)
    if not definition:
        raise RuntimeError("Missing teaching definition " + path)
    for key, value in values.items():
        definition.set_editor_property(key, value)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)


update_definition("/Game/Teaching/Definitions/DA_ZhiSunTeaching", {
    "selection_description": make_text("选择直榫，看看榫头怎样笔直插进卯眼。"),
    "observation_description": make_text("直榫由榫头和卯眼组成。先让两边的形状对齐，再从卯眼开口的一侧平稳推入。"),
    "observation_narration_text": make_text("直榫由榫头和卯眼组成。先让两边的形状对齐，再从卯眼开口的一侧平稳推入。"),
    "observation_steps": [make_text("观察：榫头的形状要与卯眼开口对齐。"), make_text("观察：保持方向不变，沿着开口平稳推入。")],
    "guided_instruction": make_text("拿起榫头，让它靠近浅金色轮廓。"),
    "guided_steps": [make_text("先让榫头正对卯眼开口。"), make_text("从开口的一侧慢慢靠近。"), make_text("轮廓重合后，保持方向继续推入。")],
    "challenge_instruction": make_text("试着独立完成直榫装配；需要时可以请求提示。"),
})

update_definition("/Game/Teaching/Definitions/DA_YanWeiTeaching", {
    "selection_description": make_text("选择燕尾榫，看看燕尾形的榫头怎样把两块木料扣在一起。"),
    "observation_description": make_text("燕尾榫的榫头像展开的燕尾。先让榫头和槽口处在同一位置，再顺着槽口从侧面滑入。"),
    "observation_narration_text": make_text("燕尾榫的榫头像展开的燕尾。先让榫头和槽口处在同一位置，再顺着槽口从侧面滑入。"),
    "observation_steps": [make_text("观察：先让燕尾形榫头和槽口对齐。"), make_text("观察：保持对齐，顺着槽口从侧面滑入。")],
    "guided_instruction": make_text("拿起燕尾榫，让它与浅金色轮廓重合。"),
    "guided_steps": [make_text("先让燕尾形榫头和槽口对齐。"), make_text("保持两块木料平齐。"), make_text("顺着槽口从侧面平稳滑入。")],
    "challenge_instruction": make_text("试着独立完成燕尾榫装配；需要时可以请求提示。"),
})

unreal.log("Updated player-facing teaching copy for both joint definitions.")
