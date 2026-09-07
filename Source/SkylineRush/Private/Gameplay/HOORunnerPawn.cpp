#include "Gameplay/HOORunnerPawn.h"
#include "QA/HOORunnerVisualQA.h"
#include "Gameplay/HOORunnerGameMode.h"
#include "World/HOORunnerScenery.h"
#include "World/HOORunnerWorlds.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    enum EPool { Deck, Inset, Edge, Rail, Support, Barrier, BarrierStripe, Beam, BeamPost,
        GapWarning, Orb, Sidewalk, Arch, ArchGlow, BoosterFrame, BoosterBolt, LaunchTile, CornerBoard, CornerArrow, CityFloor, InteriorFloor, JungleFloor, DimensionFloor, PoolCount };
    const TCHAR* RecordSlot() { return FParse::Param(FCommandLine::Get(),TEXT("RunnerQA")) ? TEXT("SkylineRush_QA_Record_v5") : TEXT("SkylineRush_Record_v5"); }
    const TCHAR* PreferencesSlot() { return FParse::Param(FCommandLine::Get(),TEXT("RunnerQA")) ? TEXT("SkylineRush_QA_Preferences_v1") : TEXT("SkylineRush_Preferences_v1"); }
    UStaticMesh* Mesh(const TCHAR* Path) { return LoadObject<UStaticMesh>(nullptr, Path); }
}

AHOORunnerPawn::AHOORunnerPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    auto* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RunnerRoot"));
    SetRootComponent(Root);
    RunnerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RunnerMesh"));
    RunnerMesh->SetupAttachment(Root);
    RunnerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RunnerMesh->SetRelativeRotation(FRotator(0,-90,0));
    RunnerMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    RunnerMesh->bEnableUpdateRateOptimizations = false;
    RunnerMesh->SetBoundsScale(1.6f);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Hero(TEXT("/Game/SkylineRush/Characters/Meshes/SK_StudentHero_IllustrationV2"));
    if (Hero.Succeeded()) RunnerMesh->SetSkeletalMesh(Hero.Object);
    ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
    ChaseCamera->SetupAttachment(Root);
    ChaseCamera->SetAbsolute(true,true,true);
    ChaseCamera->FieldOfView = 76.0f;
    AquariumLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("AquariumFill"));
    AquariumLight->SetupAttachment(Root);AquariumLight->SetAbsolute(true,true,true);
    AquariumLight->SetMobility(EComponentMobility::Movable);AquariumLight->SetCastShadows(false);
    AquariumLight->SetIntensityUnits(ELightUnits::Lumens);AquariumLight->SetIntensity(12000000);
    AquariumLight->SetAttenuationRadius(6500);AquariumLight->SetLightColor(FLinearColor(.65f,.88f,1.f));
    AquariumLight->SetVisibility(false);
    HeroFill=CreateDefaultSubobject<UPointLightComponent>(TEXT("UnderwaterHeroFill"));
    HeroFill->SetupAttachment(Root);HeroFill->SetAbsolute(true,true,true);
    HeroFill->SetMobility(EComponentMobility::Movable);HeroFill->SetCastShadows(false);
    HeroFill->SetIntensityUnits(ELightUnits::Lumens);HeroFill->SetIntensity(6000000);
    HeroFill->SetAttenuationRadius(1800);HeroFill->SetLightColor(FLinearColor(.82f,.92f,1.f));
    HeroFill->SetIndirectLightingIntensity(0);HeroFill->SetVolumetricScatteringIntensity(0);
    HeroFill->bAffectTranslucentLighting=false;
    HeroFill->SetLightingChannels(false,true,false);HeroFill->SetVisibility(false);
    RunnerMesh->SetLightingChannels(true,true,false);
    const TCHAR* AnimBase = TEXT("/Game/SkylineRush/Characters/Animations/");
    auto Animation = [AnimBase](const TCHAR* Name) { return LoadObject<UAnimSequence>(nullptr, *(FString(AnimBase)+Name)); };
    IdleAnimation=Animation(TEXT("STUDENT_MM_Idle"));
    RunAnimation=Animation(TEXT("STUDENT_MF_Unarmed_Jog_Fwd_Stable"));
    JumpAnimation=Animation(TEXT("STUDENT_MM_Jump"));
    SlideAnimation=Animation(TEXT("STUDENT_MM_Land"));
    FallAnimation=Animation(TEXT("STUDENT_MM_Death_Front_01"));
    PickupSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SkylineRush/Audio/SFX/SFX_CrystalPickup"));
    CrashSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SkylineRush/Audio/SFX/SFX_ObstacleHit"));
    BoostSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SkylineRush/Audio/SFX/SFX_BoostActivate"));
    FeverSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SkylineRush/Audio/SFX/SFX_FeverActivate"));
    PickupConcurrency=CreateDefaultSubobject<USoundConcurrency>(TEXT("RunnerPickupConcurrency"));
    AccentConcurrency=CreateDefaultSubobject<USoundConcurrency>(TEXT("RunnerAccentConcurrency"));
    for(auto* Limit:{PickupConcurrency.Get(),AccentConcurrency.Get()})
    {
        Limit->Concurrency.bLimitToOwner=true;
        Limit->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::StopOldest;
        Limit->Concurrency.VoiceStealReleaseTime=.03f;
    }
    PickupConcurrency->Concurrency.MaxCount=2;
    AccentConcurrency->Concurrency.MaxCount=3;
    MusicAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("RunnerMusic"));
    MusicAudio->SetupAttachment(Root);
    MusicAudio->bAutoActivate=false;
    MusicAudio->bAutoDestroy=false;
    MusicAudio->bAllowSpatialization=false;
    MusicAudio->bIsUISound=true;
    MusicTracks.Add(LoadObject<USoundBase>(nullptr,TEXT("/Game/SkylineRush/Audio/Music/BGM_BreezyAdventure_01")));
    MusicTracks.Add(LoadObject<USoundBase>(nullptr,TEXT("/Game/SkylineRush/Audio/Music/BGM_BreezyAdventure_02")));
    UStaticMesh* Cube=Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    AddPool(TEXT("Deck"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Ink")),3);
    AddPool(TEXT("LaneInset"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_EarthPath")),3);
    AddPool(TEXT("Edge"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Cyan")),2);
    AddPool(TEXT("Rail"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Ivory")),2);
    AddPool(TEXT("Support"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Ink")),1);
    AddPool(TEXT("Barrier"),Mesh(TEXT("/Game/SkylineRush/Environment/Meshes/SM_AeroBarrier")),LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_AeroHazard")),3);
    AddPool(TEXT("BarrierStripe"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Ivory")),3);
    AddPool(TEXT("Overhead"),Mesh(TEXT("/Game/SkylineRush/Environment/Meshes/SM_AeroBarrier")),LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_AeroHazard")),3);
    AddPool(TEXT("BeamPosts"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Coral")),6);
    AddPool(TEXT("GapWarning"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Coral")),6);
    AddPool(TEXT("Shard"),Mesh(TEXT("/Game/SkylineRush/Environment/Meshes/SM_AeroCrystal")),LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Gold")),1);
    AddPool(TEXT("SideGardenDeck"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_AeroPaving")),2);
    AddPool(TEXT("RouteArch"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Ink")),3);
    AddPool(TEXT("RouteArchGlow"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Cyan")),1);
    AddPool(TEXT("BoosterFrame"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Cyan")),8);
    AddPool(TEXT("BoosterBolt"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Ivory")),3);
    AddPool(TEXT("LaunchTiles"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Gold")),3);
    AddPool(TEXT("CornerBoards"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Ink")),1);
    AddPool(TEXT("CornerArrows"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/MI_Cyan")),4);
    AddPool(TEXT("CityFloor"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_CityWalk")),3);
    AddPool(TEXT("InteriorFloor"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_AtriumFloor")),3);
    AddPool(TEXT("JungleFloor"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_MossStone")),3);
    AddPool(TEXT("DimensionFloor"),Cube,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_VoidFloor")),3);

}

UInstancedStaticMeshComponent* AHOORunnerPawn::AddPool(const TCHAR* Name,UStaticMesh* InMesh,UMaterialInterface* Mat,int32 PerTile)
{
    auto* Pool=CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
    Pool->SetupAttachment(GetRootComponent());
    Pool->SetAbsolute(true,true,true);
    Pool->SetMobility(EComponentMobility::Movable);
    Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Pool->SetStaticMesh(InMesh);
    if (Mat) Pool->SetMaterial(0,Mat);
    Pool->ComponentTags.Add(FName(*FString::FromInt(PerTile)));
    Pool->SetCanEverAffectNavigation(false);
    Pool->bCastDynamicShadow= true;
    Pools.Add(Pool);
    return Pool;
}

void AHOORunnerPawn::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Saved=Cast<UHOORunnerRecord>(UGameplayStatics::LoadGameFromSlot(RecordSlot(),0)))
    {
        if(Saved->Version==5 && FMath::IsFinite(Saved->Distance)) BestDistance=FMath::Max(0.0f,Saved->Distance);
        BestScore=FMath::Max(0,Saved->Score);
        LocalScores=Saved->Runs;
        if(LocalScores.Num()>10) LocalScores.SetNum(10);
    }
    else
    {
        const TCHAR* LegacySlot=FParse::Param(FCommandLine::Get(),TEXT("RunnerQA"))?TEXT("SkylineRush_QA_Record_v4"):TEXT("SkylineRush_Record_v4");
        auto* Legacy=Cast<UHOORunnerRecord>(UGameplayStatics::LoadGameFromSlot(LegacySlot,0));
        if(!Legacy) Legacy=Cast<UHOORunnerRecord>(UGameplayStatics::LoadGameFromSlot(FParse::Param(FCommandLine::Get(),TEXT("RunnerQA"))?TEXT("SkylineRush_QA_Record_v3"):TEXT("SkylineRush_Record_v3"),0));
        if(Legacy)
        {
            if(FMath::IsFinite(Legacy->Distance)) BestDistance=FMath::Max(0.f,Legacy->Distance);
            BestScore=FMath::Max(0,Legacy->Score);
        }
    }
    ReloadPreferences();
    Run.Reset(409);
    StartingBest=BestDistance;
    StartingBestScore=BestScore;
    Slots.Init(-1,HOORunner::VisibleTiles);
    for(const auto& Pool:Pools)
    {
        const int32 Count=FCString::Atoi(*Pool->ComponentTags[0].ToString())*HOORunner::VisibleTiles;
        Pool->PreAllocateInstancesMemory(Count);
        for(int32 I=0;I<Count;++I) Pool->AddInstance(FTransform(FQuat::Identity,FVector(0,0,-100000),FVector::ZeroVector));
    }
    Pools[Orb]->SetCastShadow(false);
    Pools[BoosterFrame]->SetCastShadow(false);
    Pools[BoosterBolt]->SetCastShadow(false);
    UpdateCourse(true);
    UpdatePresentation(0.0f);
    Scenery=GetWorld()->SpawnActor<AHOORunnerScenery>();
    Worlds=GetWorld()->SpawnActor<AHOORunnerWorlds>();
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("RunnerQA")) && FParse::Param(FCommandLine::Get(),TEXT("RunnerVisualQA")))
    {
        auto* Probe=GetWorld()->SpawnActor<AHOORunnerVisualQA>();Probe->Initialize(this);AddTickPrerequisiteActor(Probe);
    }
#endif
    MusicAudio->OnAudioFinished.AddDynamic(this,&AHOORunnerPawn::PlayNextMusicTrack);
    UpdateMusicSettings();
    PlayNextMusicTrack();
    UE_LOG(LogTemp,Display,TEXT("RUNNER_READY mesh=%s pools=%d tiles=%d"),
        *GetNameSafe(RunnerMesh->GetSkeletalMeshAsset()),Pools.Num(),Slots.Num());
}

void AHOORunnerPawn::StartRun()
{
    if(Run.Phase==EHOORunnerPhase::Paused) { PauseRun(); return; }
    if(Run.Phase==EHOORunnerPhase::Running) return;
    StartingBest=BestDistance;
    StartingBestScore=BestScore;
    Run.Reset(Run.Seed);
    RunId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    bRunRecorded=false;bReplayOverflow=false;ReplayInputs.Reset();
    Run.Start();
    Countdown=1.5f; DeathTime=0; PickupPulse=0; SlideBlend=0; StagePulse=0; LastOrbTile=-1; LastBoosterVisual=-1; LastProtectedVisual=-1; bCameraInitialized=false;
    UpdateCourse(true);
}

void AHOORunnerPawn::RestartRun()
{
    SaveRecord();
    Run.Reset(Run.Seed+1);
    StartRun();
}
void AHOORunnerPawn::MoveLeft() { if(Countdown<=0) { RecordInput(TEXT("l"));Run.Move(-1); } }
void AHOORunnerPawn::MoveRight() { if(Countdown<=0) { RecordInput(TEXT("r"));Run.Move(1); } }
void AHOORunnerPawn::JumpAction()
{
    if(Run.Phase==EHOORunnerPhase::Ready || Run.Phase==EHOORunnerPhase::Crashed) { StartRun(); return; }
    if(Countdown<=0) { RecordInput(TEXT("j"));Run.Jump(); }
}
void AHOORunnerPawn::SlideAction() { if(Countdown<=0) { RecordInput(TEXT("s"));Run.Slide(); } }
void AHOORunnerPawn::PauseRun()
{
    Run.TogglePause();
    UpdateMusicSettings();
    if(Run.Phase==EHOORunnerPhase::Running) Countdown=FMath::Max(Countdown,1.f);
}

const TCHAR* AHOORunnerPawn::GetDistrict() const
{
    return HOORunnerWorld::Name(HOORunnerWorld::Biome(Run.Distance));
}

void AHOORunnerPawn::Tick(float Dt)
{
    Super::Tick(Dt);
    UpdateMusicSettings();
    const auto Previous=Run.Phase;
    const int32 PreviousCoins=Run.Coins;
    const int32 PreviousHits=Run.Hits;
    const int32 PreviousBoosters=Run.BoostersCollected;
    const int32 PreviousFevers=Run.FeverActivations;
    const int32 PreviousLevel=HOORunner::Difficulty(Run.Distance);
    if(Run.Phase!=EHOORunnerPhase::Paused)
    {
        VisualTime+=Dt;
        StagePulse=FMath::Max(0.f,StagePulse-Dt);
        if(Countdown>0) Countdown=FMath::Max(0.0f,Countdown-Dt);
        else if(!bVisualQAFrozen) Run.Advance(Dt);
    }
    if(HOORunner::Difficulty(Run.Distance)>PreviousLevel) StagePulse=2.5f;
    if(Run.Hits>PreviousHits && Run.Lives>0) PlayRunnerSFX(CrashSound,.50f,1.f,TEXT("hit"));
    if(Run.Coins>PreviousCoins)
    {
        PickupPulse=1.0f;
        if(Run.BoostersCollected==PreviousBoosters && Run.FeverActivations==PreviousFevers)
            PlayRunnerSFX(PickupSound,.34f,FMath::Min(1.12f,1.f+.025f*(Run.Multiplier()-1)),TEXT("pickup"));
    }
    if(Run.BoostersCollected>PreviousBoosters)
    {
        PlayRunnerSFX(BoostSound,.45f,1.f,TEXT("boost"));
        UE_LOG(LogTemp,Display,TEXT("RUNNER_BOOST distance=%.2f count=%d"),Run.Distance/100.,Run.BoostersCollected);
    }
    if(Run.FeverActivations>PreviousFevers)
    {
        PlayRunnerSFX(FeverSound,.50f,1.f,TEXT("fever"));
        UE_LOG(LogTemp,Display,TEXT("RUNNER_FEVER distance=%.2f count=%d"),Run.Distance/100.,Run.FeverActivations);
    }
    PickupPulse=FMath::Max(0.0f,PickupPulse-Dt*2.5f);
    if(Run.Phase==EHOORunnerPhase::Crashed)
    {
        DeathTime+=Dt;
        if(Previous!=EHOORunnerPhase::Crashed)
        {
            SaveRecord();
            PlayRunnerSFX(CrashSound,.50f,1.f,TEXT("crash"));
            UE_LOG(LogTemp,Display,TEXT("RUNNER_CRASH distance=%.2fm hazard=%s"),Run.Distance/100.0,HOORunner::HazardName(Run.Failure));
        }
    }
    UpdateCourse();
    UpdatePresentation(Dt);
    if(Scenery) Scenery->Present(Run,ChaseCamera,QualityLevel,bReducedMotion,Dt);
    if(Worlds) Worlds->Present(Run.Distance);
}

void AHOORunnerPawn::SaveRecord()
{
    if(Run.Distance<=0) return;
    BestDistance=FMath::Max(BestDistance,static_cast<float>(Run.Distance/100.0));
    BestScore=FMath::Max(BestScore,Run.Score());
    auto* Record=Cast<UHOORunnerRecord>(UGameplayStatics::CreateSaveGameObject(UHOORunnerRecord::StaticClass()));
    if(!bRunRecorded)
    {
        FHOORunnerScoreEntry Entry;
        Entry.RunId=RunId;Entry.Nickname=PlayerNickname;Entry.Date=FDateTime::UtcNow().ToIso8601();
        Entry.Score=Run.Score();Entry.Distance=Run.Distance/100.;Entry.Seconds=Run.Elapsed;
        Entry.Seed=Run.Seed;Entry.Ticks=Run.SimulationTicks;
        if(!bReplayOverflow) Entry.ReplayJson=TEXT("[")+FString::Join(ReplayInputs,TEXT(","))+TEXT("]");
        LocalScores.Add(Entry);
        LocalScores.Sort([](const auto& A,const auto& B) { if(A.Score!=B.Score) return A.Score>B.Score;if(A.Distance!=B.Distance) return A.Distance>B.Distance;return A.Date<B.Date; });
        if(LocalScores.Num()>10) LocalScores.SetNum(10);
        bRunRecorded=true;
    }
    Record->Distance=BestDistance;Record->Score=BestScore;Record->Runs=LocalScores;
    if(!UGameplayStatics::SaveGameToSlot(Record,RecordSlot(),0))
        UE_LOG(LogTemp,Warning,TEXT("RUNNER_RECORD_SAVE_FAILED"));
}

void AHOORunnerPawn::UpdateCourse(bool bReset)
{
    const int64 Start=FMath::Max<int64>(0,FMath::FloorToInt64(Run.Distance/HOORunner::TileLength)-8);
    for(int32 I=0;I<HOORunner::VisibleTiles;++I)
    {
        const int64 Index=Start+I;
        const int32 Slot=static_cast<int32>(Index%HOORunner::VisibleTiles);
        if(bReset || Slots[Slot]!=Index) { Slots[Slot]=Index;PlaceTile(Slot,Index); }
    }
    if(Run.LastBoosterTile!=LastBoosterVisual)
    {
        LastBoosterVisual=Run.LastBoosterTile;
        if(LastBoosterVisual>=0)
        {
            const int32 Slot=static_cast<int32>(LastBoosterVisual%HOORunner::VisibleTiles);
            if(Slots[Slot]==LastBoosterVisual) PlaceTile(Slot,LastBoosterVisual);
        }
    }
    if(Run.ProtectedThroughTile!=LastProtectedVisual)
    {
        for(int32 Slot=0;Slot<Slots.Num();++Slot)
            if(Slots[Slot]>LastProtectedVisual && Slots[Slot]<=Run.ProtectedThroughTile) PlaceTile(Slot,Slots[Slot]);
        LastProtectedVisual=Run.ProtectedThroughTile;
    }
    if(Run.LastCollectedTile!=LastOrbTile)
    {
        for(int32 Slot=0;Slot<Slots.Num();++Slot)
            if(Slots[Slot]>LastOrbTile && Slots[Slot]<=Run.LastCollectedTile && Slots[Slot]%2==1)
                Pools[Orb]->UpdateInstanceTransform(Slot,FTransform(FQuat::Identity,FVector(0,0,-100000),FVector::ZeroVector),true,false,true);
        Pools[Orb]->MarkRenderInstancesDirty();
        LastOrbTile=Run.LastCollectedTile;
    }
}

void AHOORunnerPawn::PlaceTile(int32 Slot,int64 Index)
{
    const double S=(Index+.5)*HOORunner::TileLength;
    const FVector P=HOORunner::Center(S);
    const FRotator R=HOORunner::Heading(S);
    const FQuat Q=R.Quaternion();
    const double Bend=FMath::Acos(FMath::Clamp(FVector::DotProduct(HOORunner::Frame(S-300).GetUnitAxis(EAxis::X),HOORunner::Frame(S+300).GetUnitAxis(EAxis::X)),-1.,1.));
    const double Length=(HOORunner::Center(S+300)-HOORunner::Center(S-300)).Length()+2.0+970*FMath::Sin(Bend*.5);
    const auto T=HOORunner::Tile(Index,Run.Seed);
    const auto Biome=HOORunnerWorld::Biome(S);
    const bool City=Biome==EHOORunnerBiome::City;
    auto Set=[&](int32 Pool,int32 Local,const FVector& Offset,const FVector& Size,bool Visible=true,FRotator Rotation=FRotator::ZeroRotator)
    {
        const int32 PerTile=FCString::Atoi(*Pools[Pool]->ComponentTags[0].ToString());
        const FTransform Transform(Q*Rotation.Quaternion(),P+Q.RotateVector(Offset),Visible ? Size/100.0 : FVector::ZeroVector);
        Pools[Pool]->UpdateInstanceTransform(Slot*PerTile+Local,Transform,true,false,true);
    };
    for(int32 Lane=-1;Lane<=1;++Lane)
    {
        const int32 L=Lane+1;
        const auto Hazard=Index<=Run.ProtectedThroughTile && !T.bFlightGap?EHOORunnerHazard::None:T.Lanes[L];
        const bool Solid=Hazard!=EHOORunnerHazard::Gap;
        Set(Deck,L,FVector(0,Lane*300,-34),FVector(Length,300,68),Solid);
        const int Floors[]={Inset,CityFloor,InteriorFloor,JungleFloor,DimensionFloor};
        for(int B=0;B<5;++B)Set(Floors[B],L,FVector(0,Lane*300,.5),FVector(Length-2,298,1),Solid && B==static_cast<int>(Biome));
        Set(Barrier,L,FVector(0,Lane*300,57.5),FVector(150,240,115),Hazard==EHOORunnerHazard::Barrier);
        Set(BarrierStripe,L,FVector(-77,Lane*300,67),FVector(4,216,22),Hazard==EHOORunnerHazard::Barrier);
        Set(Beam,L,FVector(0,Lane*300,192.5),FVector(150,282,125),Hazard==EHOORunnerHazard::Overhead);
        for(int32 Side=0;Side<2;++Side)
        {
            Set(BeamPost,L*2+Side,FVector(0,Lane*300+(Side?135:-135),83),FVector(100,12,166),Hazard==EHOORunnerHazard::Overhead);
            Set(GapWarning,L*2+Side,FVector(Side?283:-283,Lane*300,2),FVector(26,295,4),Hazard==EHOORunnerHazard::Gap);
        }
    }
    const bool Narrow=HOORunner::Difficulty(S)>=5;
    for(int32 Side=0;Side<2;++Side)
    {
        const float Sign=Side?1.0f:-1.0f;
        Set(Edge,Side,FVector(0,Sign*455,5),FVector(Length,10,10));
        Set(Rail,Side,FVector(0,Sign*482,55),FVector(Length,10,16),Index%12<7 && !T.bFlightGap && (City || Biome==EHOORunnerBiome::Interior));
        Set(Sidewalk,Side,FVector(0,Sign*795,-65),FVector(Length,470,95),City && !Narrow && Index%12<9);
    }
    Set(Support,0,FVector(0,0,-680),FVector(120,660,1250),City && Index%4==0);
    const bool Gate=Index%24==0 && City && !T.bFlightGap;
    Set(Arch,0,FVector(0,-485,295),FVector(70,50,590),Gate);
    Set(Arch,1,FVector(0,485,295),FVector(70,50,590),Gate);
    Set(Arch,2,FVector(0,0,590),FVector(70,1020,40),Gate);
    Set(ArchGlow,0,FVector(-37,0,580),FVector(8,930,12),Gate);
    Set(Orb,0,FVector(0,T.SafeLane*300,95),FVector(55,55,90),
        Index%2==1 && Index>Run.LastCollectedTile,FRotator(0,18,0));
    const bool BoosterVisible=T.bBooster && Index>Run.LastBoosterTile;
    for(int32 I=0;I<8;++I)
    {
        const float Angle=I*PI/4.f;
        Set(BoosterFrame,I,FVector(0,T.BoosterLane*300+76*FMath::Cos(Angle),125+76*FMath::Sin(Angle)),
            FVector(16,61,12),BoosterVisible,FRotator(0,0,FMath::RadiansToDegrees(Angle)+90));
    }
    Set(BoosterBolt,0,FVector(-12,T.BoosterLane*300-9,150),FVector(24,20,64),BoosterVisible,FRotator(0,0,-27));
    Set(BoosterBolt,1,FVector(-12,T.BoosterLane*300,125),FVector(24,43,18),BoosterVisible);
    Set(BoosterBolt,2,FVector(-12,T.BoosterLane*300+9,100),FVector(24,20,64),BoosterVisible,FRotator(0,0,-27));
    for(int32 Lane=0;Lane<3;++Lane)
        Set(LaunchTile,Lane,FVector(-40,(Lane-1)*300,5),FVector(500,250,10),T.bLaunchPad);
    const float Turn=HOORunner::TurnPreview(S),Sign=Turn>0?1.f:-1.f;
    const bool HasTurn=FMath::Abs(Turn)>8 && Index%4==0;
    Set(CornerBoard,0,FVector(0,-Sign*690,205),FVector(22,172,102),HasTurn);
    for(int32 I=0;I<4;++I)
        Set(CornerArrow,I,FVector(-13,-Sign*690+(I/2?34:-34)-Sign*10,(I%2?190:220)),FVector(4,45,9),HasTurn,FRotator(0,0,Sign*(I%2?-45:45)));
    for(const auto& Pool:Pools) Pool->MarkRenderInstancesDirty();
}

void AHOORunnerPawn::UpdatePresentation(float Dt)
{
    const FTransform Track=HOORunner::Frame(Run.Distance);
    const FRotator Direction=Track.Rotator();
    const FVector Right=Track.GetUnitAxis(EAxis::Y),Up=Track.GetUnitAxis(EAxis::Z),Forward=Track.GetUnitAxis(EAxis::X);
    FVector Position=Track.GetLocation()+Right*Run.Lateral+Up*Run.Height;
    if(Run.Phase==EHOORunnerPhase::Crashed && Run.Failure==EHOORunnerHazard::Gap)
        Position.Z-=FMath::Min(1400.0f,DeathTime*DeathTime*1100.0f);
    SetActorLocationAndRotation(Position,Direction,false,nullptr,ETeleportType::TeleportPhysics);
    RunnerMesh->SetVisibility(!(Run.RecoveryRemaining>0 && FMath::Fmod(VisualTime,.20f)<.065f));
    UAnimSequence* Desired=IdleAnimation;
    bool Loop=true;
    float Rate=1;
    if(Run.Phase==EHOORunnerPhase::Running || Run.Phase==EHOORunnerPhase::Paused)
    {
        if(Countdown<=0)
        {
            Desired=RunAnimation;
            Rate=FMath::GetMappedRangeValueClamped(FVector2D(1400,3800),FVector2D(1.35,2.1),Run.CurrentSpeed);
            if(Run.Height>1) {Desired=JumpAnimation;Loop=false;Rate=1.45f;}
            if(Run.IsSliding()) {Desired=SlideAnimation;Loop=false;Rate=0.f;}
        }
    }
    if(Run.Phase==EHOORunnerPhase::Crashed) {Desired=FallAnimation;Loop=false;}
    if(Desired && Desired!=ActiveAnimation)
    {
        RunnerMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        RunnerMesh->PlayAnimation(Desired,Loop);
        ActiveAnimation=Desired;
    }
    RunnerMesh->SetPlayRate(Run.Phase==EHOORunnerPhase::Paused || bVisualQAFrozen?0:Rate);
    const float Lean=FMath::Clamp((Run.TargetLane*300-Run.Lateral)*.045f+(bReducedMotion?0:HOORunner::TurnPreview(Run.Distance)*-.12f),-10.0f,10.0f);
    if(Run.Phase!=EHOORunnerPhase::Paused)
        SlideBlend=FMath::FInterpTo(SlideBlend,Run.IsSliding()?1.f:0.f,Dt,25.f);
    // Reuse the existing landing crouch with the feet kept on the track.
    // Apply banking in actor space before the imported mesh's -90 degree facing.
    const FQuat Bank(FVector::ForwardVector,FMath::DegreesToRadians(Lean*(1-SlideBlend)));
    if(Run.IsSliding()) RunnerMesh->SetPosition(.10f,false);
    RunnerMesh->SetRelativeRotation(Bank*FRotator(0,-90,0).Quaternion());
    RunnerMesh->SetRelativeLocation(FVector::ZeroVector);
    RunnerMesh->SetMorphTarget(TEXT("Fcl_EYE_Close"),FMath::Fmod(VisualTime,4.5f)<.12f?1.0f:0.0f);
    const bool PowerActive=(Run.IsFever() || Run.IsBoosting() || Run.IsFlying()) && (Run.Phase==EHOORunnerPhase::Running || Run.Phase==EHOORunnerPhase::Paused);
    const float SpeedAlpha=FMath::Clamp((Run.CurrentSpeed-1400)/2400,0.0f,1.0f);
    const FVector CameraTarget=Position-Forward*(650+SpeedAlpha*90)+Up*340-Right*(Run.Lateral*.65f);
    const FVector LookAt=Position+Forward*900+Up*95-Right*(Run.Lateral*.8f);
    const FVector CameraPosition=bCameraInitialized?FMath::VInterpTo(ChaseCamera->GetComponentLocation(),CameraTarget,Dt,12):CameraTarget;
    ChaseCamera->SetWorldLocation(CameraPosition);
    const FVector CameraUp=bReducedMotion && HOORunner::Theme(Run.Distance)==EHOORunnerTheme::Loop?FVector::UpVector:Up;
    const FQuat CameraRotation=FRotationMatrix::MakeFromXZ((LookAt-CameraPosition).GetSafeNormal(),CameraUp).ToQuat();
    ChaseCamera->SetWorldRotation(bCameraInitialized?FQuat::Slerp(ChaseCamera->GetComponentQuat(),CameraRotation,FMath::Clamp(Dt*12,0.f,1.f)):CameraRotation);
    ChaseCamera->SetFieldOfView(FMath::FInterpTo(ChaseCamera->FieldOfView,76+(bReducedMotion?0:SpeedAlpha*14+Run.PowerSpeedAlpha*5+(Run.IsFlying()?5:0)),Dt,3.0f));
    auto& PP=ChaseCamera->PostProcessSettings;
    const auto Biome=HOORunnerWorld::Biome(Run.Distance);
    const bool Water=Biome==EHOORunnerBiome::City || Biome==EHOORunnerBiome::Interior || Biome==EHOORunnerBiome::Dimension;
    AquariumLight->SetVisibility(Water);
    AquariumLight->SetLightColor(Biome==EHOORunnerBiome::Interior?FLinearColor(1.f,.87f,.65f):FLinearColor(.62f,.76f,1.f));
    AquariumLight->SetWorldLocation(Position+Forward*650+Up*700);
    HeroFill->SetVisibility(Water);
    HeroFill->SetWorldLocation(Position-Forward*460+Right*300+Up*450);
    PP.bOverride_SceneColorTint=true;
    PP.SceneColorTint=Run.IsFever()?FLinearColor(1.f,.9f,.97f):FLinearColor::White;
    PP.bOverride_BloomIntensity=true;PP.BloomIntensity=PowerActive && Run.IsFever()?.32f:PowerActive && Run.IsBoosting()?.26f:.18f;
    PP.bOverride_VignetteIntensity=true;PP.VignetteIntensity=Water?.18f:Run.IsBoosting()?.24f:.12f;
    PP.bOverride_SceneFringeIntensity=true;PP.SceneFringeIntensity=0;
    PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=bReducedMotion || !PowerActive?0:Run.IsBoosting()?.06f:0;
    bCameraInitialized=true;
}

FString AHOORunnerPawn::GetRunSnapshot() const
{
    const TCHAR* Phase=Run.Phase==EHOORunnerPhase::Ready?TEXT("ready"):Run.Phase==EHOORunnerPhase::Running?TEXT("running"):Run.Phase==EHOORunnerPhase::Paused?TEXT("paused"):TEXT("crashed");
    FString Rows;
    const int64 Current=FMath::FloorToInt64(Run.Distance/HOORunner::TileLength);
    for(int64 I=Current;I<Current+24;++I)
    {
        const auto T=HOORunner::Tile(I,Run.Seed);
        if(T.Lanes[0]==EHOORunnerHazard::None && T.Lanes[1]==EHOORunnerHazard::None && T.Lanes[2]==EHOORunnerHazard::None) continue;
        if(!Rows.IsEmpty()) Rows+=TEXT(",");
        Rows+=FString::Printf(TEXT("{\"tile\":%lld,\"distance_cm\":%.1f,\"safe_lane\":%d,\"hazards\":[%d,%d,%d]}"),
            I,(I+.5)*HOORunner::TileLength,T.SafeLane,(int)T.Lanes[0],(int)T.Lanes[1],(int)T.Lanes[2]);
    }
    FString Boosters;
    for(int64 I=Current;I<Current+80;++I)
    {
        const auto T=HOORunner::Tile(I,Run.Seed);
        if(!T.bBooster || I<=Run.LastBoosterTile) continue;
        if(!Boosters.IsEmpty()) Boosters+=TEXT(",");
        Boosters+=FString::Printf(TEXT("{\"tile\":%lld,\"distance_cm\":%.1f,\"lane\":%d}"),I,(I+.5)*600,T.BoosterLane);
    }
    int32 Instances=0;for(const auto& Pool:Pools) Instances+=Pool->GetInstanceCount();
    return FString::Printf(TEXT("{\"lives\":%d,\"hits\":%d,\"recovery_s\":%.3f,\"theme\":%d,\"flying\":%s,\"launches\":%d,\"track_z\":%.2f,\"up_z\":%.3f,\"fever_s\":%.3f,\"boost_s\":%.3f,\"fever_charge\":%d,\"fevers\":%d,\"boosters_collected\":%d,\"protected_tile\":%lld,\"last_booster_tile\":%lld,\"boosters\":[%s],\"elapsed_s\":%.3f,\"chain\":%d,\"multiplier\":%d,\"volume\":%d,\"quality\":%d,\"reduced_motion\":%s,\"phase\":\"%s\",\"distance_m\":%.3f,\"speed_kmh\":%.2f,\"level\":%d,\"lane\":%d,\"lateral\":%.2f,\"height\":%.2f,\"sliding\":%s,\"countdown\":%.2f,\"shards\":%d,\"score\":%d,\"seed\":%d,\"best_m\":%.2f,\"pools\":%d,\"instances\":%d,\"failure\":\"%s\",\"rows\":[%s]}"),
        Run.Lives,Run.Hits,Run.RecoveryRemaining,static_cast<int32>(HOORunner::Theme(Run.Distance)),Run.IsFlying()?TEXT("true"):TEXT("false"),Run.Launches,HOORunner::Center(Run.Distance).Z,HOORunner::Frame(Run.Distance).GetUnitAxis(EAxis::Z).Z,Run.FeverRemaining,Run.BoostRemaining,Run.FeverCharge,Run.FeverActivations,Run.BoostersCollected,Run.ProtectedThroughTile,Run.LastBoosterTile,*Boosters,Run.Elapsed,Run.Chain,Run.Multiplier(),VolumePercent,QualityLevel,bReducedMotion?TEXT("true"):TEXT("false"),Phase,Run.Distance/100.0,Run.CurrentSpeed*.036,HOORunner::Difficulty(Run.Distance),Run.TargetLane,Run.Lateral,Run.Height,Run.IsSliding()?TEXT("true"):TEXT("false"),
        Countdown,Run.Coins,Run.Score(),Run.Seed,BestDistance,Pools.Num(),Instances,HOORunner::HazardName(Run.Failure),*Rows);
}

void AHOORunnerPawn::EndPlay(const EEndPlayReason::Type Reason)
{
    // Stop also emits OnAudioFinished; unbind first so teardown cannot start another song.
    bMusicStopping=true;
    MusicAudio->OnAudioFinished.RemoveDynamic(this,&AHOORunnerPawn::PlayNextMusicTrack);
    MusicAudio->Stop();
    SaveRecord();
    Super::EndPlay(Reason);
}
void AHOORunnerPawn::ReturnToMenu()
{
    SaveRecord();
    Run.Reset(Run.Seed);
    Countdown=0; DeathTime=0; SlideBlend=0; StagePulse=0; LastOrbTile=-1; LastBoosterVisual=-1; LastProtectedVisual=-1;
    bCameraInitialized=false;
    UpdateCourse(true);
}
void AHOORunnerPawn::QuitGame()
{
    SaveRecord();
    UKismetSystemLibrary::QuitGame(this,Cast<APlayerController>(GetController()),EQuitPreference::Quit,false);
}
void AHOORunnerPawn::ReloadPreferences()
{
    if(auto* Saved=Cast<UHOORunnerPreferences>(UGameplayStatics::LoadGameFromSlot(PreferencesSlot(),0)))
    {
        VolumePercent=FMath::Clamp(Saved->Volume,0,100);
        QualityLevel=FMath::Clamp(Saved->Quality,0,2);
        bReducedMotion=Saved->bReducedMotion;
        PlayerNickname=Saved->Nickname;OnlinePlayerId=Saved->PlayerId;OnlineToken=Saved->PlayerToken;
    }
    if(PlayerNickname.IsEmpty()) PlayerNickname=TEXT("러너")+FGuid::NewGuid().ToString().Left(4);
    ApplyPreferences();
}
void AHOORunnerPawn::ApplyPreferences()
{
    if(auto* Settings=UGameUserSettings::GetGameUserSettings())
    {
        Settings->SetOverallScalabilityLevel(QualityLevel);
        Settings->SetFrameRateLimit(60);
        // Apply without writing editor/global preferences during a QA run.
        Settings->ApplyNonResolutionSettings();
    }
}
void AHOORunnerPawn::SavePreferences()
{
    auto* Saved=Cast<UHOORunnerPreferences>(UGameplayStatics::CreateSaveGameObject(UHOORunnerPreferences::StaticClass()));
    Saved->Volume=VolumePercent; Saved->Quality=QualityLevel; Saved->bReducedMotion=bReducedMotion;
    Saved->Nickname=PlayerNickname;Saved->PlayerId=OnlinePlayerId;Saved->PlayerToken=OnlineToken;
    if(!UGameplayStatics::SaveGameToSlot(Saved,PreferencesSlot(),0))
        UE_LOG(LogTemp,Warning,TEXT("RUNNER_PREFERENCES_SAVE_FAILED"));
}
void AHOORunnerPawn::CycleVolume() { VolumePercent=(VolumePercent+25)%125; SavePreferences(); }
void AHOORunnerPawn::CycleQuality() { QualityLevel=(QualityLevel+1)%3; ApplyPreferences(); SavePreferences(); }
void AHOORunnerPawn::ToggleReducedMotion() { bReducedMotion=!bReducedMotion; SavePreferences(); }
