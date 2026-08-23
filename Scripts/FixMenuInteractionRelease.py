import unreal


CONTEXT_PATH = "/Game/XRFramework/Input/IMC_Menu"
PRESSED_PATH = "/Game/XRFramework/Input/Actions/IA_Menu_Interact_Right_Pressed"
RELEASED_PATH = "/Game/XRFramework/Input/Actions/IA_Menu_Interact_Right_Released"


context = unreal.load_asset(CONTEXT_PATH)
pressed_action = unreal.load_asset(PRESSED_PATH)
released_action = unreal.load_asset(RELEASED_PATH)
if not context or not pressed_action or not released_action:
    raise RuntimeError("Missing XR menu interaction input assets.")

# The shipped right-release asset was configured with a Pressed trigger and was
# not mapped in IMC_Menu. Correct both so the trigger release reaches C++.
released_trigger = unreal.InputTriggerReleased(outer=released_action)
released_action.set_editor_property("triggers", [released_trigger])

mappings = list(context.get_editor_property("default_key_mappings").get_editor_property("mappings"))
pressed_keys = []
for mapping in mappings:
    action = mapping.get_editor_property("action")
    key = mapping.get_editor_property("key")
    if action == pressed_action:
        key_copy = unreal.Key()
        key_copy.set_editor_property("key_name", key.get_editor_property("key_name"))
        pressed_keys.append(key_copy)

context.unmap_all_keys_from_action(released_action)
for key in pressed_keys:
    context.map_key(released_action, key)

unreal.EditorAssetLibrary.save_asset(RELEASED_PATH, only_if_is_dirty=False)
unreal.EditorAssetLibrary.save_asset(CONTEXT_PATH, only_if_is_dirty=False)
unreal.log("Configured right menu interaction release for continuous widget dragging.")
