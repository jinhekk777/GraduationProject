import unreal


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


expected_joint_ids = {"ZhiSun", "YanWeiSun"}
banned_player_copy = ("本地", "轴", "Transform", "方向约束", "互锁")
map_path = "/Game/XRFramework/Levels/L_XRTemplate"
require(unreal.EditorLoadingAndSavingUtils.load_map(map_path), "Could not load L_XRTemplate")
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
managers = [actor for actor in actor_subsystem.get_all_level_actors()
            if actor.get_class().get_name() == "AssemblySessionManager"]
require(len(managers) == 1, "Expected exactly one AssemblySessionManager in L_XRTemplate")

manager = managers[0]
bindings = manager.get_editor_property("joint_bindings")
require(len(bindings) == 2, "Expected two configured teaching joint bindings")
configured_ids = set()
for binding in bindings:
    definition = binding.get_editor_property("definition")
    part = binding.get_editor_property("assembly_part")
    assembly_actor = binding.get_editor_property("assembly_actor")
    presentation_actors = binding.get_editor_property("presentation_actors")
    require(definition is not None, "A teaching joint binding has no JointDefinition")
    require(part is not None or assembly_actor is not None,
            "A teaching joint binding has neither an AssemblyPartComponent nor an AssemblyActor")
    require(len(presentation_actors) >= 2, "Each teaching joint requires visible tenon and mortise actors")
    require(abs(definition.get_editor_property("observation_post_narration_delay_seconds") - 2.0) < 0.01,
            "Observation post-narration unlock delay must remain 2 seconds")
    narration_sound = definition.get_editor_property("observation_narration_sound")
    require(narration_sound is not None,
            "Each teaching joint requires an observation narration sound")
    require(narration_sound.get_class().get_name() == "SoundWave",
            "Observation narration must be a SoundWave")
    require(abs(definition.get_editor_property("hint_offer_seconds") - 30.0) < 0.01,
            "Hint offer time must remain 30 seconds")
    require(abs(definition.get_editor_property("automatic_hint_seconds") - 60.0) < 0.01,
            "Automatic hint time must remain 60 seconds")
    require(abs(definition.get_editor_property("completion_feedback_seconds") - 1.0) < 0.01,
            "Completion feedback time must remain 1 second")
    for field_name in ("selection_description", "observation_description",
                       "observation_narration_text", "guided_instruction",
                       "challenge_instruction"):
        player_copy = str(definition.get_editor_property(field_name))
        require(not any(term in player_copy for term in banned_player_copy),
                "Player-facing copy contains technical wording: " + field_name)
    configured_ids.add(str(definition.get_editor_property("joint_id")))

require(configured_ids == expected_joint_ids, "Teaching bindings must be ZhiSun and YanWeiSun")
require(manager.get_editor_property("teaching_widget_class") is not None,
        "AssemblySessionManager has no placeholder teaching widget class")

menu_context = unreal.load_asset("/Game/XRFramework/Input/IMC_Menu")
pressed_action = unreal.load_asset("/Game/XRFramework/Input/Actions/IA_Menu_Interact_Right_Pressed")
released_action = unreal.load_asset("/Game/XRFramework/Input/Actions/IA_Menu_Interact_Right_Released")
require(menu_context and pressed_action and released_action,
        "XR menu press/release input assets must exist")
release_triggers = released_action.get_editor_property("triggers")
require(any(trigger.get_class().get_name() == "InputTriggerReleased" for trigger in release_triggers),
        "Right menu release action must use InputTriggerReleased")
pressed_keys = set()
released_keys = set()
for mapping in menu_context.get_editor_property("default_key_mappings").get_editor_property("mappings"):
    action = mapping.get_editor_property("action")
    key_name = str(mapping.get_editor_property("key").get_editor_property("key_name"))
    if action == pressed_action:
        pressed_keys.add(key_name)
    elif action == released_action:
        released_keys.add(key_name)
unreal.log("Right menu pressed keys: " + ", ".join(sorted(pressed_keys)))
unreal.log("Right menu released keys: " + ", ".join(sorted(released_keys)))
require(pressed_keys and released_keys == pressed_keys,
        "Right menu release action must mirror every pressed-action controller key")
unreal.log("Dual-joint teaching configuration validation passed for " + ", ".join(sorted(configured_ids)))
