# SKYLINE RUSH asset provenance

Updated 2026-09-07. This document covers retained assets only.

- Student: project-owned VRoid Studio 2.14.0 base and bundled clothing presets, refined in Blender 4.5.12. VRM metadata permits individual/corporate commercial use and modification, requires creator credit, and prohibits standalone model redistribution. Preserve this attribution and metadata when shipping. No third-party preset was downloaded.
- Editable student: SourceArt/Characters/StudentHero/IllustrationV2/StudentIllustration_Source.blend. The current recipe starts from Working/HOO_StudentHero_v3.blend; both sources, VRoid source, VRM metadata and required FBX/material import sources are retained.
- Student recipe: Tools/Character/build_student_illustration.py; original tailored collar, pleated skirt and backpack geometry. Shared bone names and 57 facial morph targets remain intact.
- Epic mannequin skeleton, Control Rig, preview meshes and five retargeted animations remain because the active character references them. They are Epic Unreal template content, subject to the applicable Unreal Engine EULA. Some engine-provided animation import paths point to Epic build machines; source FBX files were never present locally.
- Tree geometry and base/ORM atlas: original mathematical geometry and seeded texture synthesis in Blender. SourceArt/Environment/CityIllustrationV2/CityIllustration_Source.blend contains the shared editable scene; only the two used tree variants and their LOD exports are retained.
- Paving albedo: original OpenAI image generation, 2026-08-12; normal and roughness were derived from that project-owned image. Retained in SourceArt/Environment/Paving.
- Coastal panorama: original AI-generated image, not a downloaded photograph. Exact prompt and provenance are in SourceArt/Environment/Coastal/PROVENANCE.json. Coastal, course and foliage meshes are original procedural geometry.
- Portrait: original image generation; provenance retained next to SourceArt/UI/Dialogue/T_Dialogue_StudentHero_Portrait_v1.png.
- Pickup and damage sounds: project-authored procedural WAV files in SourceArt/Audio, reused from the earlier game.
- Active pickup, hit, boost and fever effects now use four user-supplied generated WAVs. Original files are preserved under SourceArt/Audio/RunnerSFX/Originals; prepared short versions and processing hashes are documented in SourceArt/Audio/RunnerSFX/PROVENANCE.md. Legacy procedural sounds remain available locally but are no longer triggered by the runner.
- Background music: two user-supplied Breezy Adventure WAVs, following the Suno generation discussion. Original bytes, filenames and hashes are recorded in SourceArt/Audio/Music/PROVENANCE.md. The game alternates the two tracks; the public source export excludes the raw recordings.
- New material graphs and UI are project-authored. No copied commercial game characters, UI artwork, levels or paid assets were introduced by cleanup.

Asset leaf names containing HOO, v1 or v2 can remain where they identify a live authored asset or skeleton. Unreal package dependency closure is authoritative.
