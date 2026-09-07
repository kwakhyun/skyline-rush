# Special-section art

Created for SKYLINE RUSH on 2026-09-07.

- `T_SpecialSurfaces_v1.png`: original image generated with the built-in Imagegen tool, generation mode. 1536×1024 RGB atlas. Copied unchanged into this project; no external photo or game reference supplied.
- `SM_SP_*Gate.obj`: original procedural geometry authored in `Tools/Unreal/build_special_sections.py`, centimetre units. Cedar curves, structural supports, fluted pillars, stone arch wedges and double polygon rings.
- Imported assets and materials live under `/Game/SkylineRush/Environment/SpecialSections`.
- Texture cells are selected in the Unreal material graph, with roughness/metallic controls and a shader surface-normal perturbation derived from luminance. No claim of scanned PBR accuracy is made.
- Existing character, foliage and background assets retain their original provenance.

## Final Imagegen prompt

Use case: stylized-concept. Asset type: a single production game environment material atlas, exactly 3 columns by 2 rows, 1536x1024 landscape. Six equally sized square texture cells with absolutely no gutters, borders, captions or text, occupying the entire canvas. This is ONE UV atlas to be used on 3D meshes in an original realistic adventure runner game SKYLINE RUSH. Orthographic flat surface scans, photoreal material microdetail, no perspective, no cast shadows, neutral diffuse lighting. Top left: warm ancient cedar bark with deep vertical furrows and small moss patches. Top middle: weathered warm orange painted industrial steel with precise inset panel seams, rivets and dark scraped edges. Top right: refined ivory limestone marble with restrained subtle gold mineral veins and fine pores. Bottom left: old dark jade stone with moss in cracks and subtle original geometric carved channels. Bottom middle: obsidian blue-violet alien ceramic with angular cyan mineral veins, dark polished panels, no symbols. Bottom right: satin dark titanium with diagonal yellow industrial hazard stripes and scuffs. Each square's material fills its entire cell, consistent realistic physical surface scale. No scenery, no objects, no typography, no watermark, no photographic staging. Clear material separation exactly at x=1/3 and 2/3 and y=1/2. Rich detailed believable materials, restrained color contrast suitable for game surfaces.
