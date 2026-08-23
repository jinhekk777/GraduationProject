import unreal

MAP_PATH = "/Game/XRFramework/Levels/L_XRTemplate"
WIDGET_FOLDER = "/Game/Teaching/UI"
WIDGET_PATH = WIDGET_FOLDER + "/WBP_AssemblyTeachingHUD"


def make_text(value):
    return unreal.TextLibrary.conv_string_to_text(value)


def add_widget(tree, root, widget, x, y, width, height):
    slot = root.add_child(widget)
    slot.set_position(unreal.Vector2D(x, y))
    slot.set_size(unreal.Vector2D(width, height))


def add_text(tree, root, name, value, x, y, width, height):
    widget = tree.construct_widget(unreal.TextBlock, name)
    widget.set_text(make_text(value))
    add_widget(tree, root, widget, x, y, width, height)


def add_button(tree, root, name, value, x, y):
    widget = tree.construct_widget(unreal.Button, name)
    label = tree.construct_widget(unreal.TextBlock, name + "Caption")
    label.set_text(make_text(value))
    widget.set_content(label)
    add_widget(tree, root, widget, x, y, 150, 40)


widget = unreal.load_asset(WIDGET_PATH)
if widget is None:
    if not unreal.EditorAssetLibrary.does_directory_exist(WIDGET_FOLDER):
        unreal.EditorAssetLibrary.make_directory(WIDGET_FOLDER)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.AssemblyTeachingWidget)
    widget = unreal.AssetToolsHelpers.get_asset_tools().create_asset("WBP_AssemblyTeachingHUD", WIDGET_FOLDER, unreal.WidgetBlueprint, factory)
    if not widget:
        raise RuntimeError("Unable to create teaching widget")
    tree = widget.get_editor_property("widget_tree")
    root = tree.construct_widget(unreal.CanvasPanel, "Root")
    tree.set_editor_property("root_widget", root)
    add_text(tree, root, "PhaseText", "教学阶段", 32, 32, 540, 34)
    add_text(tree, root, "JointNameText", "榫卯名称", 32, 70, 540, 34)
    add_text(tree, root, "InstructionText", "教学说明", 32, 110, 640, 132)
    add_text(tree, root, "StatsText", "", 32, 246, 640, 32)
    add_text(tree, root, "InputHintText", "", 32, 282, 640, 32)
    add_button(tree, root, "AdvanceButton", "继续 / A", 32, 324)
    add_button(tree, root, "SelectNextButton", "切换 / B", 190, 324)
    add_button(tree, root, "RetryButton", "独立重试", 348, 324)
    add_button(tree, root, "HintButton", "提示 / Y", 32, 372)
    add_button(tree, root, "AlignmentAssistButton", "对准 / X", 190, 372)
    add_button(tree, root, "ExitChallengeButton", "退出挑战", 348, 372)
    unreal.BlueprintEditorLibrary.compile_blueprint(widget)
    unreal.EditorAssetLibrary.save_asset(WIDGET_PATH, only_if_is_dirty=False)

unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
manager = None
for item in unreal.EditorLevelLibrary.get_all_level_actors():
    if item.get_actor_label() == "AssemblyTeachingSession":
        manager = item
        break
if not manager:
    raise RuntimeError("Teaching session manager is missing")
widget_class = unreal.EditorAssetLibrary.load_blueprint_class(WIDGET_PATH)
if not widget_class:
    raise RuntimeError("Unable to load teaching widget class")
manager.set_editor_property("teaching_widget_class", widget_class)
manager.set_editor_property("create_teaching_widget", True)
unreal.EditorAssetLibrary.save_asset(WIDGET_PATH, only_if_is_dirty=False)
unreal.EditorLevelLibrary.save_current_level()
unreal.log("Teaching placeholder widget configured on " + manager.get_name())
