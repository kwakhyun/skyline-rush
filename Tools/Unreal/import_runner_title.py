"""Import the original dedicated title illustration as a resident sRGB UI texture."""
from pathlib import Path
import unreal
ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/SkylineRush/UI/Title'
NAME = 'T_SkylineRush_Title_v1'
unreal.EditorAssetLibrary.make_directory(DEST)
task = unreal.AssetImportTask()
task.filename = str(ROOT / 'SourceArt/UI/Title' / (NAME + '.png'))
task.destination_path = DEST
task.destination_name = NAME
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset(DEST + '/' + NAME)
assert isinstance(texture, unreal.Texture2D)
texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('srgb', True)
texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
texture.set_editor_property('never_stream', True)
assert unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
unreal.log('RUNNER_TITLE_IMPORT_OK ' + texture.get_path_name())
