#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/HOORunnerTypes.h"
#include "HOORunnerScenery.generated.h"
class UStaticMeshComponent;class UInstancedStaticMeshComponent;class UMaterialInstanceDynamic;class UCameraComponent;
class UMaterialInterface;class UExponentialHeightFogComponent;class USkyAtmosphereComponent;
UCLASS()
class SKYLINERUSH_API AHOORunnerScenery:public AActor
{
 GENERATED_BODY()
public:
 AHOORunnerScenery();
 void Present(const FHOORunnerState& Run,UCameraComponent* Camera,int32 Quality,bool ReducedMotion,float Dt);
private:
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Sky;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Towers;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Islands;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Particles;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BurstParticles;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BurstMaterial;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ParticleMaterial;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ScreenMaterial;
 UPROPERTY() TObjectPtr<UMaterialInterface> DaySky;
 UPROPERTY() TObjectPtr<UMaterialInterface> VoidSky;
 UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
 UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
 UPROPERTY() TObjectPtr<AActor> Ocean;
 int32 PreviousBiome=-1;
 int64 LastChunk=-999;
 bool Initialized=false;
 float Time=0,BurstTime=0,PreviousHeight=0;
 int32 PreviousCoins=0,PreviousHits=0;
 FVector BurstOrigin=FVector::ZeroVector;
 FQuat BurstRotation=FQuat::Identity;
};
