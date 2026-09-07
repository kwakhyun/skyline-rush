"""Original special-section meshes and Imagegen UV-atlas materials. Run inside Unreal."""
import unreal, math, json
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
ART=ROOT/'SourceArt/Environment/SpecialSections'
DEST='/Game/SkylineRush/Environment/SpecialSections'
E=unreal.EditorAssetLibrary; L=unreal.MaterialEditingLibrary; A=unreal.AssetToolsHelpers.get_asset_tools()
E.make_directory(DEST)
task=unreal.AssetImportTask();task.filename=str(ART/'T_SpecialSurfaces_v1.png');task.destination_path=DEST
task.destination_name='T_SpecialSurfaces_v1';task.automated=True;task.replace_existing=True;task.save=True
A.import_asset_tasks([task]);tex=unreal.load_asset(DEST+'/T_SpecialSurfaces_v1');assert tex

names=['Timber','Skyworks','Prism','Jade','Rift','Warning']
for n,name in enumerate(names):
    path=DEST+'/M_SP_'+name
    if E.does_asset_exist(path): continue
    mat=A.create_asset('M_SP_'+name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('used_with_instanced_static_meshes',True)
    uv=L.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate)
    c=L.create_material_expression(mat,unreal.MaterialExpressionCustom)
    # A two pixel inset keeps mip filtering inside each cell; mesh UVs span [0,1].
    c.set_editor_property('code',f'return (saturate(P.xy)*float2(508.,508.)+float2({n%3*512+2}.,{n//3*512+2}.))/float2(1536.,1024.);')
    c.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    ci=unreal.CustomInput();ci.set_editor_property('input_name','P');c.set_editor_property('inputs',[ci])
    L.connect_material_expressions(uv,'',c,'P')
    sample=L.create_material_expression(mat,unreal.MaterialExpressionTextureSample);sample.set_editor_property('texture',tex)
    L.connect_material_expressions(c,'',sample,'UVs');L.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    for prop,val in [(unreal.MaterialProperty.MP_ROUGHNESS,[.86,.48,.32,.82,.29,.55][n]),(unreal.MaterialProperty.MP_METALLIC,[0,.65,.08,0,.5,.5][n])]:
        v=L.create_material_expression(mat,unreal.MaterialExpressionConstant);v.set_editor_property('r',val);L.connect_material_property(v,'',prop)
    # Shader surface relief is derived from atlas luminance; no edited raster maps.
    bump=L.create_material_expression(mat,unreal.MaterialExpressionCustom)
    bump.set_editor_property('code','float h=dot(C,float3(.299,.587,.114)); float3 sx=ddx(W), sy=ddy(W); float3 n=normalize(N); float3 a=cross(sy,n), b=cross(n,sx); float det=dot(sx,a); return normalize(abs(det)*n-sign(det)*.35*(ddx(h)*a+ddy(h)*b));')
    bump.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs=[]
    for key in ['C','W','N']:
        i=unreal.CustomInput();i.set_editor_property('input_name',key);inputs.append(i)
    bump.set_editor_property('inputs',inputs)
    wp=L.create_material_expression(mat,unreal.MaterialExpressionWorldPosition)
    normal=L.create_material_expression(mat,unreal.MaterialExpressionVertexNormalWS)
    L.connect_material_expressions(sample,'RGB',bump,'C');L.connect_material_expressions(wp,'',bump,'W');L.connect_material_expressions(normal,'',bump,'N')
    mat.set_editor_property('tangent_space_normal',False);L.connect_material_property(bump,'',unreal.MaterialProperty.MP_NORMAL)
    if n==4:
        glow=L.create_material_expression(mat,unreal.MaterialExpressionCustom)
        glow.set_editor_property('code','return C*float3(.25,1,1.5)*180;')
        glow.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        i=unreal.CustomInput();i.set_editor_property('input_name','C');glow.set_editor_property('inputs',[i])
        L.connect_material_expressions(sample,'RGB',glow,'C');L.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    L.recompile_material(mat);assert E.save_loaded_asset(mat,False)

class Mesh:
    def __init__(self): self.v=[];self.uv=[];self.f=[]
    def quad(self,points):
        n=len(self.v);self.v.extend(points);self.uv.extend([(0,0),(1,0),(1,1),(0,1)]);self.f.append([n+j+1 for j in range(4)])
    def box(self,c,s):
        x,y,z=c;a,b,d=[v*.5 for v in s]
        p=[(x+i*a,y+j*b,z+k*d) for i,j,k in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        for face in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(3,7,6,2),(0,4,7,3),(1,2,6,5)]: self.quad([p[i] for i in face])
    def curve(self,points,radius):
        pts=[points[0]]+points+[points[-1]];samples=[]
        for k in range(1,len(pts)-2):
            for j in range(8):
                t=j/8;samples.append(tuple(.5*((2*pts[k][a])+(-pts[k-1][a]+pts[k+1][a])*t+(2*pts[k-1][a]-5*pts[k][a]+4*pts[k+1][a]-pts[k+2][a])*t*t+(-pts[k-1][a]+3*pts[k][a]-3*pts[k+1][a]+pts[k+2][a])*t*t*t) for a in range(3)))
        samples.append(points[-1]);n=len(self.v);sides=20
        for i,p in enumerate(samples):
            a=samples[max(0,i-1)];b=samples[min(len(samples)-1,i+1)]
            dy=b[1]-a[1];dz=b[2]-a[2];d=math.hypot(dy,dz);u=(0,-dz/d,dy/d)
            rad=radius*(1-.22*i/(len(samples)-1))
            for j in range(sides+1):
                angle=j*math.tau/sides;r=rad*(1+.055*math.sin(j*2.7+i*.23))
                self.v.append((p[0]+r*math.sin(angle),p[1]+r*u[1]*math.cos(angle),p[2]+r*u[2]*math.cos(angle)))
                self.uv.append((j/sides,i/(len(samples)-1)))
        for i in range(len(samples)-1):
            for j in range(sides):
                a=n+i*(sides+1)+j+1;self.f.append([a,a+1,a+sides+2,a+sides+1])
    def tube(self,points,radius,sides=16):
        # Each span uses a stable perpendicular frame; closed end caps avoid holes.
        for start,end in zip(points,points[1:]):
            axis=[end[i]-start[i] for i in range(3)];ln=math.sqrt(sum(x*x for x in axis));w=[x/ln for x in axis]
            ref=(0,0,1) if abs(w[2])<.9 else (1,0,0)
            def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
            u=cross(w,ref);norm=math.sqrt(sum(x*x for x in u));u=[x/norm for x in u];v=cross(w,u)
            rings=[[(p[i]+radius*(u[i]*math.cos(j*math.tau/sides)+v[i]*math.sin(j*math.tau/sides))) for i in range(3)] for p in [start,end] for j in range(sides)]
            for j in range(sides): self.quad([rings[j],rings[(j+1)%sides],rings[(j+1)%sides+sides],rings[j+sides]])
            # Polygon caps with explicit OBJ UVs.
            for ring in [rings[:sides][::-1],rings[sides:]]:
                n=len(self.v);self.v.extend(ring);self.uv.extend([(0.5+.49*math.cos(j*math.tau/sides),.5+.49*math.sin(j*math.tau/sides)) for j in range(sides)]);self.f.append([n+j+1 for j in range(sides)])
    def save(self,name,material):
        path=ART/(name+'.obj')
        with path.open('w') as f:
            f.write('o '+name+'\ns off\n')
            for v in self.v:f.write('v %f %f %f\n'%tuple(v))
            for uv in self.uv:f.write('vt %f %f\n'%tuple(uv))
            for face in self.f:f.write('f '+' '.join(f'{i}/{i}' for i in face)+'\n')
        t=unreal.AssetImportTask();t.filename=str(path);t.destination_path=DEST;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
        A.import_asset_tasks([t]);mesh=unreal.load_asset(DEST+'/'+name);assert mesh
        mesh.set_material(0,unreal.load_asset(DEST+'/M_SP_'+material));assert E.save_loaded_asset(mesh,False)
        return {'mesh':name,'vertices':len(self.v),'faces':len(self.f)}

stats=[]
for kind in names[:5]:
    m=Mesh()
    if kind=='Timber':
        for sign in [-1,1]:
            m.curve([(0,sign*990,-60),(40,sign*1060,430),(-30,sign*900,950),(0,sign*620,1330),(0,0,1500)],145)
            for d in [-1,1]:m.curve([(0,sign*1000,210),(d*220,sign*1250,20),(d*400,sign*1590,-70)],90)
            m.curve([(0,sign*900,900),(180,sign*1450,1150),(260,sign*1850,1180)],85)
    elif kind=='Skyworks':
        for sign in [-1,1]:
            m.box((0,sign*880,600),(180,160,1200));m.box((0,sign*880,50),(440,440,100))
            for z in [200,550,900]:m.tube([(-220,sign*1020,z),(220,sign*1020,z+330)],30,8)
        m.box((0,0,1250),(200,1940,190));m.box((0,0,1450),(130,2400,100))
        m.tube([(-170,-900,1360),(170,0,1580),(-170,900,1360)],50,8)
    elif kind=='Prism':
        for sign in [-1,1]:
            m.box((0,sign*860,530),(260,240,1060));m.box((0,sign*860,70),(380,360,140))
            for y in [-75,0,75]:m.tube([(-140,sign*860+y,160),(-140,sign*860+y,950)],18,12)
        m.box((0,0,1160),(300,1960,240));m.box((0,0,1350),(420,2220,110))
        for y in [-510,0,510]:m.box((-180,y,1160),(50,120,180))
    elif kind=='Jade':
        for sign in [-1,1]:
            for k in range(5):m.box((k%2*30,sign*(960-k*18),k*210+90),(420-k*25,360-k*20,180))
            m.box((0,sign*980,-120),(840,700,200))
        # Wedge voussoirs form a real curved arch with small mortar joints.
        for j in range(15):
            a=j*math.pi/15+.012;b=(j+1)*math.pi/15-.012
            ring=[(math.cos(t)*r,980+math.sin(t)*r*.58) for t,r in [(a,880),(b,880),(b,1090),(a,1090)]]
            p=[(x,y,z) for x in [-160,160] for y,z in ring]
            for face in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:m.quad([p[i] for i in face])
    else:
        for radius,x in [(1100,0),(1340,150)]:
            pts=[(x,math.cos(j*math.tau/12)*radius,1250+math.sin(j*math.tau/12)*radius) for j in range(13)]
            # Bottom arcs are below the deck; no opaque surface crosses the camera/route.
            pts=[(p[0],p[1],p[2]+320) for p in pts]
            # Keep the inner opening clear above 380cm; outer foundations remain off lanes.
            for a,b in zip(pts,pts[1:]):
                if min(a[2],b[2])>500 or min(abs(a[1]),abs(b[1]))>650:m.tube([a,b],75,8)
        for sign in [-1,1]:m.box((0,sign*1060,300),(240,260,600))
    stats.append(m.save('SM_SP_'+kind+'Gate',kind))
(ART/'mesh-stats.json').write_text(json.dumps(stats,indent=2))
unreal.log('SPECIAL_SECTION_ASSETS_OK '+json.dumps(stats))
