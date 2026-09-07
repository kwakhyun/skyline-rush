"""Refine the existing licensed student source without changing skeleton or facial morphs.
Authored pleated skirt, collar, buttons and backpack construction; tapered hair tips.
"""
import bpy,bmesh,math,json,sys,hashlib
from pathlib import Path
from mathutils import Vector,kdtree
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'SourceArt/Characters/StudentHero/IllustrationV2';OUT.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(Path(__file__).parent));import mesh_utils as pipeline
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'SourceArt/Characters/StudentHero/Working/HOO_StudentHero_v3.blend'))
bpy.context.preferences.filepaths.save_version=0
rig=bpy.data.objects['SK_HOO_StudentHero_Rig'];body=bpy.data.objects['SK_HOO_StudentHero_Body'];hair=bpy.data.objects['SK_HOO_StudentHero_Hair'];face=bpy.data.objects['SK_HOO_StudentHero_Face']
original_morphs=[k.name for k in face.data.shape_keys.key_blocks];original_bones=[b.name for b in rig.data.bones]
white=bpy.data.materials['M_STUDENT_BlouseWhite'];navy=bpy.data.materials['M_STUDENT_SkirtNavy'];bagmat=bpy.data.materials['M_STUDENT_Bag']
skirt_index=next(i for i,m in enumerate(body.data.materials) if m==navy)
oldids={v for p in body.data.polygons if p.material_index==skirt_index for v in p.vertices}
tree=kdtree.KDTree(len(oldids));weights={}
for i,vi in enumerate(oldids):
    tree.insert(body.data.vertices[vi].co,i);weights[i]=[(body.vertex_groups[g.group].name,g.weight) for g in body.data.vertices[vi].groups]
tree.balance()
bm=bmesh.new();bm.from_mesh(body.data);bmesh.ops.delete(bm,geom=[p for p in bm.faces if p.material_index==skirt_index],context='FACES');bm.to_mesh(body.data);bm.free()
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);bpy.context.view_layer.objects.active=body
sub=body.modifiers.new('Soft cloth and anatomy silhouette','SUBSURF');sub.levels=1;sub.render_levels=1;bpy.ops.object.modifier_apply(modifier=sub.name)

def weighted_mesh(name,vs,fs,material,bone='J_Bip_C_UpperChest',transfer=False):
    data=bpy.data.meshes.new(name);data.from_pydata(vs,[],fs);data.update();ob=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(ob);data.materials.append(material)
    for p in data.polygons:p.use_smooth=True
    uv=data.uv_layers.new(name='UVMap')
    for p in data.polygons:
        for li in p.loop_indices:
            co=data.vertices[data.loops[li].vertex_index].co;uv.data[li].uv=(.5+co.x,.5+co.z*.25)
    if transfer:
        for v in data.vertices:
            _,near,_=tree.find(v.co)
            for bn,w in weights[near]:
                group=ob.vertex_groups.get(bn) or ob.vertex_groups.new(name=bn);group.add([v.index],w,'REPLACE')
    else:ob.vertex_groups.new(name=bone).add(list(range(len(vs))),1,'REPLACE')
    mod=ob.modifiers.new('Armature','ARMATURE');mod.object=rig
    return ob

n=96;rings=9;vs=[];fs=[]
for j in range(rings):
    t=j/(rings-1);z=1.105-.315*t
    for i in range(n):
        angle=i*math.tau/n;pleat=(0,.008,.011,-.002)[i%4]*t
        rx=.121+.113*t+pleat;ry=.103+.082*t+pleat
        vs.append((math.cos(angle)*rx,math.sin(angle)*ry+.012,z+.0015*math.sin(angle*3)*t))
for j in range(rings-1):
    for i in range(n):fs.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
skirt=weighted_mesh('Tailored 24-pleat skirt',vs,fs,navy,transfer=True)
solid=skirt.modifiers.new('Stitched hem thickness','SOLIDIFY');solid.thickness=.0018
bpy.context.view_layer.objects.active=skirt;bpy.ops.object.modifier_apply(modifier=solid.name)
additions=[skirt]
for side in (-1,1):
    coords=[(.051,-.005,1.389),(.012,-.059,1.374),(.031,-.131,1.306),(.098,-.101,1.350),(.090,-.050,1.382)]
    ob=weighted_mesh('Folded shirt collar',[(side*x,y,z) for x,y,z in coords],[(0,1,4),(1,3,4),(1,2,3)],white)
    so=ob.modifiers.new('Collar edge','SOLIDIFY');so.thickness=.002
    bpy.context.view_layer.objects.active=ob;bpy.ops.object.modifier_apply(modifier=so.name);additions.append(ob)
# Shirt button placket follows the chest profile, with actual pearl buttons.
profile=[(1.326,-.127),(1.28,-.163),(1.235,-.168),(1.19,-.148),(1.14,-.12),(1.106,-.109)]
vs=[(x,y-.003,z) for z,y in profile for x in (-.007,.007)]
additions.append(weighted_mesh('Shirt placket',vs,[(i*2,i*2+1,i*2+3,i*2+2) for i in range(len(profile)-1)],white))
for z,y in profile[1:-1]:
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,radius=.0036,location=(0,y-.007,z));ob=bpy.context.object;ob.name='Pearl button';ob.scale.y=.4
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);ob.data.materials.append(white);ob.vertex_groups.new(name='J_Bip_C_UpperChest').add(list(range(len(ob.data.vertices))),1,'REPLACE');additions.append(ob)
# Preserve the body's material slot ordering by joining all cloth into that mesh.
bpy.ops.object.select_all(action='DESELECT')
for ob in [body]+additions:ob.select_set(True)
bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
# Reduce the flat cut-off ends of each original hair strand without replacing skin weights.
bm=bmesh.new();bm.from_mesh(hair.data);remaining=set(bm.verts);islands=[]
while remaining:
    seed=remaining.pop();stack=[seed];island=[seed]
    while stack:
        v=stack.pop()
        for e in v.link_edges:
            other=e.other_vert(v)
            if other in remaining:remaining.remove(other);stack.append(other);island.append(other)
    islands.append(island)
for strand in islands:
    lo=min(v.co.z for v in strand);hi=max(v.co.z for v in strand)
    if hi-lo<.06:continue
    tip=[v for v in strand if v.co.z<lo+(hi-lo)*.12]
    center=sum((v.co for v in tip),Vector())/len(tip)
    for v in strand:
        t=max(0,1-(v.co.z-lo)/max(.03,(hi-lo)*.16));factor=1-.80*t*t
        v.co.x=center.x+(v.co.x-center.x)*factor;v.co.y=center.y+(v.co.y-center.y)*factor
bm.to_mesh(hair.data);bm.free()
# Backpack straps and front pocket use the existing bag material and armature.
bag=bpy.data.objects['STUDENT_Backpack'];bags=[]
for side in (-1,1):
    path=[(side*.065,.18,1.34),(side*.11,.07,1.39),(side*.125,-.042,1.36),(side*.135,-.108,1.28),(side*.12,-.116,1.18),(side*.10,.12,1.07)]
    vv=[(x+dx,y,z) for x,y,z in path for dx in (-.011,.011)]
    bags.append(weighted_mesh('Backpack shoulder strap',vv,[(i*2,i*2+1,i*2+3,i*2+2) for i in range(len(path)-1)],bagmat))
for name,loc,extent in [('Front zip pocket',(0,.252,1.12),(.088,.014,.066)),('Top carry loop',(0,.192,1.344),(.036,.013,.012))]:
    bags.append(pipeline.create_cube_accessory(name,loc,extent,(0,0,0),bagmat,rig,'J_Bip_C_UpperChest',bevel_width=.008))
bpy.ops.object.select_all(action='DESELECT')
for ob in [bag]+bags:ob.select_set(True)
bpy.context.view_layer.objects.active=bag;bpy.ops.object.join()
meshes=[o for o in bpy.data.objects if o.type=='MESH' and (o.vertex_groups or o.data.shape_keys)]
assert original_morphs==[k.name for k in face.data.shape_keys.key_blocks]
assert original_bones==[b.name for b in rig.data.bones]
# Main source is neutral; preserve every original facial shape key and bone.
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'StudentIllustration_Source.blend'))
pipeline.FBX_OUTPUT=OUT/'SK_StudentHero_IllustrationV2.fbx';pipeline.export_fbx(rig,meshes)
report=dict(source='Project-owned StudentHero v3 blend; existing VRoid provenance applies. New geometry authored in this script.',bones=len(original_bones),facial_shape_keys=len(original_morphs)-1,tailored_pleats=24,collar_panels=2,hair_strands_refined=len(islands),meshes=[dict(name=o.name,vertices=len(o.data.vertices)) for o in meshes])
report['files']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in OUT.iterdir() if p.suffix in ('.blend','.fbx')}
(OUT/'manifest.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('HOO_STUDENT_ILLUSTRATION_SOURCE_OK '+json.dumps(report),flush=True)
