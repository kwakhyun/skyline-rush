#include "World/HOORunnerScenery.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraComponent.h"
#include "World/HOORunnerWorlds.h"
#include "EngineUtils.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
AHOORunnerScenery::AHOORunnerScenery()
{
 auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("SceneryRoot"));SetRootComponent(Root);
 auto Mat=[](const TCHAR* N){return LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/SkylineRush/Environment/Materials/"))+N));};
 Sky=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhotoPanorama"));Sky->SetupAttachment(Root);Sky->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));Sky->SetMaterial(0,Mat(TEXT("M_CoastalPanorama")));Sky->SetWorldScale3D(FVector(20000));Sky->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sky->SetCastShadow(false);
 auto Pool=[&](const TCHAR* Name,const TCHAR* Mesh,const TCHAR* Material){auto* P=CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);P->SetupAttachment(Root);P->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Mesh));P->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Material));P->SetCollisionEnabled(ECollisionEnabled::NoCollision);P->SetCastShadow(false);P->SetCanEverAffectNavigation(false);return P;};
 Towers=Pool(TEXT("CoastalTowers"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_CoastalTower"),TEXT("/Game/SkylineRush/Environment/Materials/M_AeroGlass"));
 Islands=Pool(TEXT("CoastalIslands"),TEXT("/Game/SkylineRush/Environment/Meshes/SM_CoastalIsland"),TEXT("/Game/SkylineRush/Environment/Materials/M_AeroCliff"));
 BurstParticles=Pool(TEXT("ActionBurst"),TEXT("/Engine/BasicShapes/Plane.Plane"),TEXT("/Game/SkylineRush/Environment/Materials/M_GlowParticle"));
 Particles=Pool(TEXT("RunnerEffectPool"),TEXT("/Engine/BasicShapes/Plane.Plane"),TEXT("/Game/SkylineRush/Environment/Materials/M_GlowParticle"));
 DaySky=Mat(TEXT("M_CoastalPanorama"));
 VoidSky=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_VoidSky"));
}
void AHOORunnerScenery::Present(const FHOORunnerState& R,UCameraComponent* Camera,int32 Quality,bool ReducedMotion,float Dt)
{
 if(!Initialized)
 {
  for(int I=0;I<96;++I)Towers->AddInstance(FTransform::Identity);
  for(int I=0;I<12;++I)Islands->AddInstance(FTransform::Identity);
  for(int I=0;I<64;++I)Particles->AddInstance(FTransform::Identity);
  for(int I=0;I<20;++I)BurstParticles->AddInstance(FTransform::Identity);
  BurstMaterial=BurstParticles->CreateDynamicMaterialInstance(0);
  ParticleMaterial=Particles->CreateDynamicMaterialInstance(0);
  if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SkylineRush/Environment/Materials/M_AeroSpeedScreen"))){ScreenMaterial=UMaterialInstanceDynamic::Create(M,this);Camera->AddOrUpdateBlendable(ScreenMaterial,1);}
  Initialized=true;
  for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("RunnerOcean"))){Ocean=*It;break;}
  for(TActorIterator<AExponentialHeightFog> It(GetWorld());It;++It){Fog=It->GetComponent();break;}
  for(TActorIterator<AActor> It(GetWorld());It;++It)if(auto* C=It->FindComponentByClass<USkyAtmosphereComponent>()){Atmosphere=C;break;}
 }
 const auto Biome=HOORunnerWorld::Biome(R.Distance);
 if(PreviousBiome!=static_cast<int32>(Biome))
 {
  PreviousBiome=static_cast<int32>(Biome);
  const bool Dimension=Biome==EHOORunnerBiome::Dimension;
  Sky->SetMaterial(0,Dimension?VoidSky:DaySky);
  Islands->SetVisibility(Biome==EHOORunnerBiome::City);
  Towers->SetVisibility(Biome==EHOORunnerBiome::City);
  if(Atmosphere)Atmosphere->SetVisibility(!Dimension);
  if(Ocean)Ocean->SetActorHiddenInGame(Biome!=EHOORunnerBiome::City);
  if(Fog)
  {
   Fog->SetVisibility(!Dimension);
   Fog->SetFogInscatteringColor(Biome==EHOORunnerBiome::Jungle?FLinearColor(.22f,.33f,.18f):Biome==EHOORunnerBiome::Forest?FLinearColor(.36f,.44f,.31f):FLinearColor(.42f,.57f,.67f));
  }
 }
 if(R.Phase!=EHOORunnerPhase::Paused)Time+=Dt;
 const FVector Origin=HOORunner::Center(R.Distance);Sky->SetWorldLocation(Origin);
 const int64 Chunk=FMath::FloorToInt64(R.Distance/12000);
 if(LastChunk!=Chunk)
 {
  LastChunk=Chunk;
  for(int I=0;I<24;++I)
  {
   const int64 K=Chunk+I-3;const auto P=HOORunner::Center(K*12000.+6000);
   const bool City=HOORunnerWorld::Biome(FMath::Max(0.,K*12000.+6000))==EHOORunnerBiome::City;
   for(int J=0;J<4;++J)
   {
    const double H=4500+(FMath::Abs(K*37+J*79)%11)*1250.;
    const auto Frame=K<1?FTransform::Identity:HOORunner::Frame(K*12000.+6000);
    FVector Pos=P+Frame.GetUnitAxis(EAxis::X)*((J%2)*4800)+Frame.GetUnitAxis(EAxis::Y)*((J<2?-1:1)*(8200+(FMath::Abs(K*13+J*19)%5)*3400));Pos.Z=-180;
    Towers->UpdateInstanceTransform(I*4+J,FTransform(FRotator(0,(K%3)*7,0),Pos,City?FVector(18+(K&3)*4,16,H/100):FVector::ZeroVector),true,false,true);
   }
  }
  for(int I=0;I<12;++I)
  {
   const int64 K=Chunk/3+I-2;auto P=HOORunner::Center(K*36000.);
   P.Y+=(I%2?1:-1)*(55000+FMath::Abs(K*137)%24000);P.Z=-1200;
   Islands->UpdateInstanceTransform(I,FTransform(FRotator(0,K*37,0),P,FVector(260+(I%3)*40,220+(I%4)*30,220+(I%5)*35)),true,false,true);
  }
  Towers->MarkRenderInstancesDirty();Islands->MarkRenderInstancesDirty();
 }
 const bool Active=R.Phase==EHOORunnerPhase::Running;
 const float Hit=FMath::Clamp((R.RecoveryRemaining-2.1f)/.4f,0.f,1.f);
 if(ScreenMaterial){ScreenMaterial->SetScalarParameterValue(TEXT("Boost"),Active && R.IsBoosting()?1:0);ScreenMaterial->SetScalarParameterValue(TEXT("Fever"),Active && R.IsFever()?1:0);ScreenMaterial->SetScalarParameterValue(TEXT("Hit"),Active?Hit:0);ScreenMaterial->SetScalarParameterValue(TEXT("Motion"),ReducedMotion?0:1);}
 const bool Powered=R.IsBoosting() || R.IsFever() || R.IsFlying() || R.RecoveryRemaining>0;
 const auto Frame=HOORunner::Frame(R.Distance);const FVector Base=Frame.GetLocation()+Frame.GetUnitAxis(EAxis::Y)*R.Lateral+Frame.GetUnitAxis(EAxis::Z)*R.Height;
 const bool Land=PreviousHeight>1 && R.Height<=1,Jump=PreviousHeight<=1 && R.Height>1;
 if(Active && (R.Coins>PreviousCoins || R.Hits>PreviousHits || Land || Jump))
 {
  BurstTime=.34f;BurstOrigin=Base+Frame.GetUnitAxis(EAxis::Z)*(R.Coins>PreviousCoins?85:8);BurstRotation=Frame.GetRotation();
  BurstMaterial->SetVectorParameterValue(TEXT("GlowColor"),R.Hits>PreviousHits?FLinearColor(1,.12f,.05f):R.Coins>PreviousCoins?FLinearColor(1,.68f,.12f):FLinearColor(.6f,.8f,1));
 }
 if(R.Phase!=EHOORunnerPhase::Paused)BurstTime=FMath::Max(0.f,BurstTime-Dt);
 if(!Active && R.Phase!=EHOORunnerPhase::Paused)BurstTime=0;
 BurstParticles->SetVisibility(BurstTime>0);
 if(BurstTime>0 && R.Phase!=EHOORunnerPhase::Paused)
 {
 for(int I=0;I<20;++I)
 {
  const float Age=1-BurstTime/.34f,A=I*2.399f;
  const FVector Pos=BurstOrigin+BurstRotation.RotateVector(FVector(FMath::Cos(A)*Age*90,FMath::Sin(A)*Age*90,Age*(I%3)*30));
  const FQuat Face=FRotationMatrix::MakeFromZ((Camera->GetComponentLocation()-Pos).GetSafeNormal()).ToQuat();
  const float Size=BurstTime>0 && I<(Quality==0?8:20)?(1-Age)*18:0;
  BurstParticles->UpdateInstanceTransform(I,FTransform(Face,Pos,FVector(Size/100)),true,false,true);
 }
 BurstParticles->MarkRenderInstancesDirty();
 }
 PreviousCoins=R.Coins;PreviousHits=R.Hits;PreviousHeight=R.Height;
 Particles->SetVisibility(Active);
 if(!Active)return;
 if(ParticleMaterial)ParticleMaterial->SetVectorParameterValue(TEXT("GlowColor"),R.IsFever()?FLinearColor(1,.04f,.37f):R.IsBoosting()?FLinearColor(.03f,.65f,1.f):FLinearColor(.03f,.5f,1));
 for(int I=0;I<64;++I)
 {
  const float U=FMath::Fmod(Time*(Powered?1.5f:2.f)+I*.618f,1.f),A=I*2.399f;
  FVector Offset(-40-U*(Powered?620:170),FMath::Sin(A)*(Powered?75:34)*(1-U),18+FMath::Cos(A)*18+U*(Powered?80:22));
  const FVector Pos=Base+Frame.GetRotation().RotateVector(Offset);
  const FQuat Face=FRotationMatrix::MakeFromZ((Camera->GetComponentLocation()-Pos).GetSafeNormal()).ToQuat();
  const bool Visible=Active && I<(Quality==0?20:Powered?64:22);
  const float Size=Visible?(Powered?30:12)*(1-U):0;
  Particles->UpdateInstanceTransform(I,FTransform(Face,Pos,FVector(Size/100)),true,false,true);
 }
 Particles->MarkRenderInstancesDirty();
}
