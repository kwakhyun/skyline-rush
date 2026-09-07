"""Import the two user-supplied WAVs; retain one-shot playback for C++ playlist sequencing."""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/SkylineRush/Audio/Music'
SOURCE = ROOT / 'SourceArt/Audio/Music'
unreal.EditorAssetLibrary.make_directory(DEST)
report = []
for index in (1, 2):
    name = f'BGM_BreezyAdventure_{index:02d}'
    source = SOURCE / f'{name}.wav'
    assert source.is_file(), source
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', DEST)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(f'{DEST}/{name}')
    assert isinstance(sound, unreal.SoundWave), f'Import failed: {name}'
    sound.set_editor_property('looping', False)
    # Muting must not kill the component or stall the alternating playlist.
    sound.set_editor_property('virtualization_mode', unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    sound.set_editor_property('loading_behavior', unreal.SoundWaveLoadingBehavior.PRIME_ON_LOAD)
    sound.set_editor_property('compression_quality', 80)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sound, False)
    duration = float(sound.get_editor_property('duration'))
    assert duration > 1, f'Invalid duration: {name}'
    report.append({'asset': f'{DEST}/{name}', 'duration_seconds': duration,
                   'channels': int(sound.get_editor_property('num_channels')),
                   'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                   'looping': False, 'virtualization': 'PlayWhenSilent'})
out = ROOT / 'Saved/QA/music-import.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('RUNNER_MUSIC_IMPORT ' + json.dumps(report))
