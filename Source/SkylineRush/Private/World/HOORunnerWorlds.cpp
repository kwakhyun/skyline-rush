#include "World/HOORunnerWorlds.h"
#include "Gameplay/HOORunnerTypes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

namespace
{
    constexpr double Bounds[]={0,60000,126000,166000,206000,300000};
    enum Pool { Ground, Broadleaf, Cedar, Rocks, Towers, Walls, WindowPanels, Roof,
        Columns, Lights, Palms, Fronds, Ferns, Ruins, Vines, VoidBlocks, VoidLines, Portal, SideFloor,
        TimberGate, SkyworksGate, PrismGate, JadeGate, RiftGate, SpecialTrim, Count };
}
EHOORunnerBiome HOORunnerWorld::Biome(double S)
{
    const double U=FMath::Fmod(FMath::Max(0.,S),HOORunner::ThemeCycleLength);
    for(int I=0;I<4;++I)if(U<Bounds[I+1])return static_cast<EHOORunnerBiome>(I);
    return EHOORunnerBiome::Dimension;
}
const TCHAR* HOORunnerWorld::Name(EHOORunnerBiome B)
{
    switch(B)
    {
    case EHOORunnerBiome::Forest:return TEXT("햇살 자연숲");
    case EHOORunnerBiome::City:return TEXT("스카이라인 도심");
    case EHOORunnerBiome::Interior:return TEXT("빌딩 아트리움");
    case EHOORunnerBiome::Jungle:return TEXT("정글 유적");
    default:return TEXT("4차원 넥서스");
    }
}
double HOORunnerWorld::Remaining(double S)
{
    return Bounds[static_cast<int>(Biome(S))+1]-FMath::Fmod(FMath::Max(0.,S),HOORunner::ThemeCycleLength);
}
float HOORunnerWorld::Progress(double S)
{
    const int I=static_cast<int>(Biome(S));
    return 1.f-static_cast<float>(Remaining(S)/(Bounds[I+1]-Bounds[I]));
}
AHOORunnerWorlds::AHOORunnerWorlds()
{
    auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("BiomeRoot"));SetRootComponent(Root);
    auto Add=[&](const TCHAR* N,const TCHAR* Mesh,const TCHAR* Mat,int32 Number,bool Shadow=false)
    {
        auto* P=CreateDefaultSubobject<UInstancedStaticMeshComponent>(N);P->SetupAttachment(Root);
        P->SetMobility(EComponentMobility::Movable);P->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Mesh));
        if(Mat)P->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Mat));
        P->SetCollisionEnabled(ECollisionEnabled::NoCollision);P->SetCanEverAffectNavigation(false);
        P->SetCastShadow(Shadow);Pools.Add(P);Counts.Add(Number);
    };
    const TCHAR* Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
    Add(TEXT("ForestBanks"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_ForestGround"),2);
    Add(TEXT("ForestBroadleaf"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_Illustration_StreetTree"),nullptr,2,true);
    Add(TEXT("ForestCanopy"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_Illustration_StreetTreeB"),nullptr,2);
    Add(TEXT("ForestRocks"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_CoastalIsland"),TEXT("/Game/SkylineRush/Environment/Materials/M_MossStone"),2);
    Add(TEXT("CityFacades"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_CoastalTower"),TEXT("/Game/SkylineRush/Environment/Materials/M_AeroGlass"),2,true);
    Add(TEXT("AtriumWalls"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_AtriumWall"),2,true);
    Add(TEXT("AtriumWindows"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_AtriumPanel"),2);
    Add(TEXT("AtriumCeiling"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_AtriumWall"),1,true);
    Add(TEXT("AtriumColumns"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_AtriumWall"),2,true);
    Add(TEXT("CeilingLights"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_WarmLight"),2);
    Add(TEXT("JunglePalms"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_PalmTrunk"),nullptr,2,true);
    Add(TEXT("JungleFronds"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_PalmCrown"),nullptr,2,true);
    Add(TEXT("JungleFerns"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_Fern"),nullptr,4);
    Add(TEXT("JungleRuins"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_MossStone"),3,true);
    Add(TEXT("HangingVines"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),TEXT("/Game/SkylineRush/Environment/Materials/M_Leaves"),2);
    Add(TEXT("DimensionMonoliths"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_VoidStone"),2);
    Add(TEXT("DimensionTesseracts"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_VioletLight"),24);
    Add(TEXT("WorldThresholds"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/MI_Cyan"),3);
    Add(TEXT("AtriumSideFloor"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/M_AtriumFloor"),2);
    const TCHAR* Themes[]={TEXT("Timber"),TEXT("Skyworks"),TEXT("Prism"),TEXT("Jade"),TEXT("Rift")};
    for(const auto* T:Themes)
        Add(*(FString(T)+TEXT("SpecialGate")),*(FString(TEXT("/Game/SkylineRush/Environment/SpecialSections/SM_SP_"))+T+TEXT("Gate")),nullptr,1,true);
    Add(TEXT("SpecialArchitecturalTrim"),Cube,TEXT("/Game/SkylineRush/Environment/Materials/MI_Gold"),6);
}
void AHOORunnerWorlds::Present(double Distance)
{
    if(Slots.IsEmpty())
    {
        Slots.Init(-9999,HOORunner::VisibleTiles);
        for(int I=0;I<Pools.Num();++I)
            for(int J=0;J<HOORunner::VisibleTiles*Counts[I];++J)
                Pools[I]->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
    }
    const int64 First=FMath::FloorToInt64(Distance/HOORunner::TileLength)-6;
    bool Changed=false;
    for(int I=0;I<HOORunner::VisibleTiles;++I)
    {
        const int64 T=First+I;const int Slot=static_cast<int>((T%Slots.Num()+Slots.Num())%Slots.Num());
        if(Slots[Slot]!=T){Slots[Slot]=T;Place(Slot,T);Changed=true;}
    }
    if(Changed)for(const auto& P:Pools)P->MarkRenderInstancesDirty();
}
void AHOORunnerWorlds::Place(int32 Slot,int64 Tile)
{
    const double S=(Tile+.5)*HOORunner::TileLength;
    const auto F=HOORunner::Frame(S);const FQuat Q=F.GetRotation();const FVector P=F.GetLocation();
    const auto B=HOORunnerWorld::Biome(S);
    const bool Forest=B==EHOORunnerBiome::Forest,City=B==EHOORunnerBiome::City;
    const bool Inside=B==EHOORunnerBiome::Interior,Jungle=B==EHOORunnerBiome::Jungle,Void=B==EHOORunnerBiome::Dimension;
    const int64 N=FMath::Abs(Tile);const bool Pair=N%2==0;
    auto Set=[&](int PoolIndex,int Local,FVector Offset,FVector Scale,bool Visible,FRotator Rot=FRotator::ZeroRotator)
    {
        Pools[PoolIndex]->UpdateInstanceTransform(Slot*Counts[PoolIndex]+Local,FTransform(Q*Rot.Quaternion(),P+Q.RotateVector(Offset),Visible?Scale:FVector::ZeroVector),true,false,true);
    };
    for(int Side=0;Side<2;++Side)
    {
        const float Sign=Side?1.f:-1.f;
        // Banks stop outside the playable deck; no background ever fills a gameplay gap.
        Set(Ground,Side,{0,Sign*3500,-180},{7,59,2.7},Forest || Jungle);
        const float TreeScale=2.2f+(N%4)*.14f;
        Set(Broadleaf,Side,{180*FMath::Sin(N*2.3+Side),Sign*((Jungle?3400:1750)+N%3*170),-40},FVector(Jungle?3.4f:TreeScale),(Forest && Pair) || (Jungle && N%4==0),FRotator(0,N*31,0));
        Set(Cedar,Side,{240*FMath::Sin(N*1.3+Side),Sign*(3300+N%4*200),-40},FVector(3.4f+(N%3)*.2f),Forest && N%4==0,FRotator(0,N*51,0));
        Set(Rocks,Side,{0,Sign*(800+N%3*130),-160},{3.2,2.8,2.8},(Forest || Jungle) && N%3==0,FRotator(0,N*37,0));
        // 24m facades with varied heights and alternating setbacks enclose the skyway.
        Set(Towers,Side,{0,Sign*(2400+N%3*260),-2200},{20,20,85.+N%7*12},City && N%4==0,FRotator(0,Side?0:180,0));
        Set(Walls,Side,{0,Sign*1480,620},{7,1,14},Inside);
        Set(WindowPanels,Side,{0,Sign*1418,675},{5.6,.20,8.6},Inside);
        Set(Columns,Side,{0,Sign*1120,510},{1.2,1.6,10.2},Inside && N%3==0);
        Set(Lights,Side,{0,Sign*900,1185},{5.4,.25,.10},Inside);
        Set(SideFloor,Side,{0,Sign*1010,-90},{7,10.2,1},Inside);
        Set(Palms,Side,{0,Sign*(1450+N%3*170),-30},FVector(1.65),Jungle && Pair,FRotator(0,N*17,0));
        Set(Fronds,Side,{0,Sign*(1450+N%3*170),-30},FVector(1.65),Jungle && Pair,FRotator(0,N*17,0));
        Set(Ruins,Side,{0,Sign*1100,400},{2.7,3.4,8},Jungle && N%6==0,FRotator(0,(N%3-1)*5,0));
        Set(Vines,Side,{100,Sign*1350,950},{.13,.13,8},Jungle && N%3==0,FRotator(7,0,6));
        Set(VoidBlocks,Side,{0,Sign*(2900+N%3*600),1000+N%4*700.},{7.+N%3*2,7,12},Void && N%5==0,FRotator(N*13,N*17,35));
    }
    Set(Roof,0,{0,0,1230},{7,30.6,.6},Inside);
    Set(Ruins,2,{0,0,900},{2.7,25,1.7},Jungle && N%18==0);
    for(int I=0;I<4;++I)
        Set(Ferns,I,{(I/2?200.:-200.),(I%2?1.:-1.)*(730+N%3*95),-10},FVector(Jungle?2.3:1.2),(Forest || Jungle) && Pair,FRotator(0,N*31+I*45,0));
    // Two nested wire cubes beside the route, 12 edges each. Static transforms recycle per tile.
    const FQuat Spin=FRotator(28,N*17,35).Quaternion();
    for(int Layer=0;Layer<2;++Layer)for(int Axis=0;Axis<3;++Axis)for(int Edge=0;Edge<4;++Edge)
    {
        const float Radius=Layer?610.f:1000.f;FVector O=FVector::ZeroVector,Size(12);
        O[(Axis+1)%3]=(Edge&1?1:-1)*Radius;O[(Axis+2)%3]=(Edge&2?1:-1)*Radius;Size[Axis]=Radius*2;
        const FVector Anchor(0,(N%2?1:-1)*4300.,2400+N%3*700.);
        Set(VoidLines,Layer*12+Axis*4+Edge,Anchor+Spin.RotateVector(O),Size/100,Void && N%10==0,Spin.Rotator());
    }
    const bool Threshold=HOORunnerWorld::Biome(S-600)!=B && Tile>=0;
    Set(Portal,0,{0,-650,400},{.45,.45,8},Threshold);
    Set(Portal,1,{0,650,400},{.45,.45,8},Threshold);
    Set(Portal,2,{0,0,800},{.45,13.4,.45},Threshold);
    const auto Special=HOORunner::Special(S);const int Phase=HOORunner::SpecialPhase(S);
    const bool Landmark=Phase==0 || Phase==6 || Phase==12 || Phase==18 || Phase==23;
    for(int I=0;I<5;++I)
        Set(TimberGate+I,0,{0,0,-20},FVector(1),Landmark && static_cast<int>(Special)==I+1);
    // Narrow luminous rails sit outside the three lanes. They reveal depth without
    // masking gameplay gaps or adding transparent overdraw/full-screen effects.
    for(int I=0;I<6;++I)
    {
        const int Side=I%2?1:-1;const int Layer=I/2;
        Set(SpecialTrim,I,{0,Side*(650.+Layer*65),80.+Layer*110},{6,.07,.07},Special!=EHOORunnerSpecial::None && !Forest);
    }
}
