"""Create project-owned biome assets. Existing assets are never deleted or rebuilt.
Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
"""
import unreal, math, json
from pathlib import Path
ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/SkylineRush/Environment/Materials'
MESHES = '/Game/SkylineRush/Environment/Meshes'
ART = ROOT / 'SourceArt/Environment/Foliage'
ART.mkdir(parents=True, exist_ok=True)
E = unreal.EditorAssetLibrary
L = unreal.MaterialEditingLibrary
A = unreal.AssetToolsHelpers.get_asset_tools()
E.make_directory(DEST)

def material(name, code, rough=.85, glow=0, sky=False):
    if E.does_asset_exist(DEST+'/'+name):
        return unreal.load_asset(DEST+'/'+name)
    m = A.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property('used_with_instanced_static_meshes', True)
    m.set_editor_property('used_with_nanite', True)
    m.set_editor_property('two_sided', sky)
    if sky:
        m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    p = L.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate if sky else unreal.MaterialExpressionWorldPosition)
    c = L.create_material_expression(m, unreal.MaterialExpressionCustom)
    c.set_editor_property('code', code)
    c.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    i = unreal.CustomInput(); i.set_editor_property('input_name', 'P'); c.set_editor_property('inputs', [i])
    L.connect_material_expressions(p, '', c, 'P')
    L.connect_material_property(c, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR if sky else unreal.MaterialProperty.MP_BASE_COLOR)
    r = L.create_material_expression(m, unreal.MaterialExpressionConstant); r.set_editor_property('r', rough)
    L.connect_material_property(r, '', unreal.MaterialProperty.MP_ROUGHNESS)
    if glow and not sky:
        v = L.create_material_expression(m, unreal.MaterialExpressionConstant); v.set_editor_property('r', glow)
        mul = L.create_material_expression(m, unreal.MaterialExpressionMultiply)
        L.connect_material_expressions(c, '', mul, 'A'); L.connect_material_expressions(v, '', mul, 'B')
        L.connect_material_property(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    L.recompile_material(m); assert E.save_loaded_asset(m, False)
    return m

# Opaque surfaces: no foliage cards, transparency or large light-emitting area.
grain = 'float n=.90+.045*sin(P.x*.031+sin(P.y*.017)*3)+.035*sin(P.y*.19+P.z*.013);'
material('M_EarthPath', grain+'return float3(.18,.115,.055)*n;')
material('M_ForestGround', grain+'float b=.85+.15*sin(P.x*.002+P.y*.003);return float3(.052,.105,.026)*n*b;')
material('M_Bark', grain+'return float3(.085,.045,.019)*n;')
material('M_Leaves', grain+'return float3(.028,.12,.025)*n;')
material('M_PalmLeaves', grain+'return float3(.045,.19,.035)*n;')
material('M_MossStone', grain+'float2 g=frac(P.xy/float2(210,150));float seam=step(.022,g.x)*step(.025,g.y);return lerp(float3(.035,.058,.027),float3(.16,.18,.105)*n,seam);')
material('M_CityWalk', grain+'float2 g=frac(P.xy/300);float seam=step(.013,g.x)*step(.013,g.y);return float3(.19,.23,.25)*n*lerp(.65,1,seam);', .62)
material('M_AtriumFloor', grain+'float2 g=frac(P.xy/400);float seam=step(.012,g.x)*step(.012,g.y);return float3(.34,.30,.23)*n*lerp(.55,1,seam);', .4)
material('M_AtriumWall', 'return float3(.50,.46,.36);', .65)
material('M_AtriumPanel', 'float2 g=frac(float2(P.x+P.y,P.z)/float2(300,500));float a=step(.018,g.x)*step(.02,g.y);return lerp(float3(.12,.18,.19),float3(.035,.075,.085),a);', .45, 130)
material('M_WarmLight', 'return float3(.95,.63,.28);', .5, 2000)
material('M_VoidFloor', 'float2 g=frac(P.xy/400);float edge=1-step(.012,min(g.x,g.y));return lerp(float3(.024,.022,.058),float3(.11,.09,.22),edge);', .5, 260)
material('M_VoidStone', 'return float3(.045,.035,.095);', .5, 250)
material('M_VioletLight', 'return float3(.35,.09,.78);', .5, 2600)
material('M_VoidSky', 'float2 uv=P;float2 cell=floor(uv*float2(1500,750));float h=frac(sin(dot(cell,float2(12.9898,78.233)))*43758.5453);float star=step(.9985,h)*pow(saturate(1-length(frac(uv*float2(1500,750))-.5)*2),3);float cloud=pow(saturate(.5+.5*sin(uv.x*13+sin(uv.y*17)*2)),4)*.18;return (float3(.004,.005,.018)+cloud*float3(.12,.05,.24)+star*float3(.8,.9,1))*4200;', sky=True)
sky=unreal.load_asset(DEST+'/M_VoidSky')
sky.set_editor_property('is_sky',True);L.recompile_material(sky);assert E.save_loaded_asset(sky,False)

# Mesh construction uses centimetres; opaque folded fronds hold up in motion.
def obj(name, verts, faces, matname):
    path = ART/(name+'.obj')
    with path.open('w') as f:
        f.write('o '+name+'\ns off\n')
        for v in verts: f.write('v %f %f %f\n'%tuple(v))
        for v in verts: f.write('vt %f %f\n'%(v[0]/100, v[2]/100))
        for face in faces: f.write('f '+' '.join('%d/%d'%(i+1,i+1) for i in face)+'\n')
    if not E.does_asset_exist(MESHES+'/'+name):
        t = unreal.AssetImportTask(); t.filename=str(path); t.destination_path=MESHES
        t.destination_name=name; t.automated=True; t.save=True
        A.import_asset_tasks([t])
        a=unreal.load_asset(MESHES+'/'+name); assert a
        a.set_material(0,unreal.load_asset(DEST+'/'+matname)); assert E.save_loaded_asset(a,False)

def trunk():
    v=[]; f=[]
    for k in range(9):
        for j in range(10):
            a=j*math.tau/10; rad=22-k*1.15
            v.append((math.cos(a)*rad+math.sin(k*.18)*65,math.sin(a)*rad,k*100))
    for k in range(8):
        for j in range(10):
            a=k*10+j;b=k*10+(j+1)%10;f.append((a,b,b+10,a+10))
    obj('SM_PalmTrunk',v,f,'M_Bark')

def fronds(name, count, length, height, width):
    v=[]; f=[]
    for j in range(count):
        a=j*math.tau/count; n=len(v)
        for k in range(9):
            t=k/8; x=t*length; z=height+math.sin(t*math.pi)*length*.18-t*t*length*.27
            w=math.sin(math.pi*t)*width
            for y,dz in [(-w,-w*.14),(0,w*.12),(w,-w*.14)]:
                v.append((x*math.cos(a)-y*math.sin(a),x*math.sin(a)+y*math.cos(a),z+dz))
        for k in range(8):
            for b in range(2):
                i=n+k*3+b
                f.extend([(i,i+3,i+4,i+1),(i+1,i+4,i+3,i)])
    obj(name,v,f,'M_PalmLeaves')

trunk(); fronds('SM_PalmCrown',11,590,800,85); fronds('SM_Fern',9,155,65,32)

unreal.log('SKYLINE_BIOME_ASSETS_OK')
