import unreal


sound = unreal.load_asset("/Game/Assembly/ZhiSun/Audio/S_ZhiSunWoodClick")
if not sound:
    raise RuntimeError("ZhiSun completion sound asset is missing")

properties = {}
for property_name in [
    "volume_multiplier", "pitch_multiplier", "sound_class_object", "attenuation_settings",
    "duration", "num_channels", "sample_rate", "sound_group",
]:
    try:
        properties[property_name] = str(sound.get_editor_property(property_name))
    except Exception:
        pass

unreal.log("Live ZhiSun sound {} properties={}".format(sound.get_path_name(), properties))
try:
    audio_subsystem = unreal.get_editor_subsystem(unreal.AudioEditorSubsystem)
    unreal.log("AudioEditorSubsystem playback members={}".format(
        [name for name in dir(audio_subsystem) if "play" in name.lower() or "preview" in name.lower()]))
except Exception as error:
    unreal.log_warning("AudioEditorSubsystem inspection unavailable: {}".format(error))
