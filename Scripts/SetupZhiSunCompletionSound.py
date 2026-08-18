import math
import os
import struct
import wave

import unreal


SOUND_PATH = "/Game/Assembly/ZhiSun/Audio/S_ZhiSunWoodClick"
WAV_PATH = os.path.join(unreal.Paths.project_saved_dir(), "GeneratedAudio", "ZhiSunWoodClick.wav")


def write_wood_click_wav():
    os.makedirs(os.path.dirname(WAV_PATH), exist_ok=True)
    sample_rate = 44100
    duration_seconds = 0.24
    sample_count = int(sample_rate * duration_seconds)
    seed = 1013904223
    filtered_noise = 0.0
    samples = []

    for index in range(sample_count):
        time_seconds = index / sample_rate
        seed = (1664525 * seed + 1013904223) & 0xFFFFFFFF
        noise = ((seed / 0xFFFFFFFF) * 2.0 - 1.0)
        filtered_noise = 0.82 * filtered_noise + 0.18 * noise
        bright_grain = noise - filtered_noise
        attack = min(1.0, time_seconds / 0.004)
        body = math.sin(2.0 * math.pi * 132.0 * time_seconds) * math.exp(-17.0 * time_seconds)
        resonance = math.sin(2.0 * math.pi * 214.0 * time_seconds) * math.exp(-25.0 * time_seconds)
        wood_edge = math.sin(2.0 * math.pi * 360.0 * time_seconds) * math.exp(-58.0 * time_seconds)
        grain = (0.35 * filtered_noise + 0.65 * bright_grain) * math.exp(-52.0 * time_seconds)
        sample = attack * max(-1.0, min(1.0, 0.40 * body + 0.17 * resonance + 0.10 * wood_edge + 0.24 * grain))
        samples.append(int(sample * 32767.0))

    with wave.open(WAV_PATH, "wb") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(struct.pack("<{}h".format(len(samples)), *samples))


def get_or_create_sound():
    write_wood_click_wav()
    task = unreal.AssetImportTask()
    task.filename = WAV_PATH
    task.destination_path = "/Game/Assembly/ZhiSun/Audio"
    task.destination_name = "S_ZhiSunWoodClick"
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    sound = unreal.load_asset(SOUND_PATH)
    if not sound:
        raise RuntimeError("Could not import the wood click sound.")
    return sound


sound = get_or_create_sound()
unreal.log("ZhiSun completion sound ready: {}".format(sound.get_path_name()))
