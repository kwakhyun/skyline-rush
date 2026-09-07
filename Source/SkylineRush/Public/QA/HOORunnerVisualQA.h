#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "HOORunnerVisualQA.generated.h"
CSV_DECLARE_CATEGORY_EXTERN(RunnerUI);
class AHOORunnerPawn;
/** Opt-in development-only capture harness. Never enabled by normal play or shipping. */
UCLASS(NotBlueprintable)
class SKYLINERUSH_API AHOORunnerVisualQA : public AActor
{
 GENERATED_BODY()
public:
 AHOORunnerVisualQA();
 void Initialize(AHOORunnerPawn* InRunner);
 virtual void Tick(float Dt) override;
private:
 UPROPERTY() TObjectPtr<AHOORunnerPawn> Runner;
 int32 Stage=0,StageFrames=0,SkipSamples=0,ForegroundSamples=0;
 double StartTime=0,PreviousTime=0;
 FString Out,Tag;
 bool bVideo=false,bProfile=false,bFinished=false;
 TArray<FVector4> Samples;
 TArray<int32> Draws;
 TSet<int32> ShotStages;
 void Next(int32 Value);
 void Shot(const TCHAR* Name);
 void Drive();
 void Report();
 void CheckReview(const TCHAR* Name,bool bPassed);
 TArray<TSharedPtr<class FJsonValue>> ReviewChecks;
 double ReviewDistance=0;
 TArray<int32> SpecialReplayTicks;
 TArray<FString> SpecialReplayActions;
 int32 SpecialReplayCursor=0,SpecialReplayEnd=0,SpecialReplayScore=0;
};
