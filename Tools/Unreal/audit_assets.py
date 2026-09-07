"""Audit active C++ runtime loads, dynamic loads, map and transitive package references.
Run through UnrealEditor-Cmd -run=pythonscript -script=<absolute path>.
Produces Saved/QA/asset-dependencies.json. Missing assets fail the commandlet.
"""
import unreal,json,re
from pathlib import Path
R=Path(unreal.Paths.project_dir()).resolve();OUT=R/'Saved/QA';OUT.mkdir(parents=True,exist_ok=True)
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.search_all_assets(True)
options=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=False,include_soft_management_references=True,include_hard_management_references=True)
assets={str(a.package_name):a for a in reg.get_assets_by_path('/Game',True)}
roots={'/Game/SkylineRush/Maps/L_SkylineRush'}
for p in (R/'Source/SkylineRush').rglob('*'):
 if p.suffix not in ['.cpp','.h']:continue
 roots.update(x.split('.')[0] for x in re.findall(r'"(/Game/[^"\s]+)"',p.read_text(encoding='utf-8')) if not x.endswith('/'))
roots.update('/Game/SkylineRush/Characters/Animations/'+n for n in ['STUDENT_MM_Idle','STUDENT_MF_Unarmed_Jog_Fwd_Stable','STUDENT_MM_Jump','STUDENT_MM_Land','STUDENT_MM_Death_Front_01'])
roots.update('/Game/SkylineRush/Environment/Materials/'+n for n in ['M_CoastalPanorama','M_GlowParticle'])
seen=set();edges={};external=set();pending=list(roots)
while pending:
 p=pending.pop()
 if p in seen:continue
 seen.add(p)
 deps=[str(x) for x in reg.get_dependencies(p,options)];edges[p]=deps
 for dep in deps:
  if dep.startswith('/Game/'):
   if dep not in seen:pending.append(dep)
  else:external.add(dep)
result={'roots':sorted(roots),'keep':sorted(seen),'unused':sorted(assets.keys()-seen),'missing':sorted(seen-assets.keys()),'external':sorted(external),'dependencies':edges}
(OUT/'asset-dependencies.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
unreal.log('SKYLINE_ASSET_AUDIT '+json.dumps({k:len(result[k]) for k in ['roots','keep','unused','missing']}))
assert not result['missing'],result['missing']
assert all(p.startswith('/Game/SkylineRush/') for p in seen), 'Legacy content dependency remains'

