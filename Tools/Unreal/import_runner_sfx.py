"""Import prepared event-specific runner sounds. Existing legacy assets are preserved."""
from pathlib import Path
import json
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
ART = ROOT / 'SourceArt/Audio/RunnerSFX'
DEST = '/Game/SkylineRush/Audio/SFX'
report = json.loads((ART / 'processing.json').read_text(encoding='utf-8'))
unreal.EditorAssetLibrary.make_directory(DEST)
for row in report:
    name = row['name']
    task = unreal.AssetImportTask()
    for key, value in dict(filename=str(ART/(name+'.wav')), destination_path=DEST,
                           destination_name=name, automated=True, replace_existing=True, save=True).items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(f'{DEST}/{name}')
    assert isinstance(sound, unreal.SoundWave), name
    sound.set_editor_property('looping', False)
    sound.set_editor_property('loading_behavior', unreal.SoundWaveLoadingBehavior.FORCE_INLINE)
    sound.set_editor_property('compression_quality', 90)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sound, False)
    assert abs(sound.get_editor_property('duration') - row['seconds']) < 0.01
    assert sound.get_editor_property('num_channels') == 2
    row['asset'] = f'{DEST}/{name}'
out = ROOT / 'Saved/QA/sfx-import.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('RUNNER_SFX_IMPORT_PASSED ' + str(len(report)))
