#include "Gameplay/HOORunnerPawn.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AHOORunnerPawn::PlayRunnerSFX(USoundBase* Sound,float Gain,float Pitch,const TCHAR* Event)
{
    if(!Sound || VolumePercent<=0) return;
    auto* Limit=Sound==PickupSound?PickupConcurrency.Get():AccentConcurrency.Get();
    UGameplayStatics::PlaySound2D(this,Sound,Gain*FMath::Clamp(VolumePercent,0,100)/100.f,Pitch,0.f,Limit,this);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("RunnerQA")))
        UE_LOG(LogTemp,Display,TEXT("RUNNER_SFX event=%s asset=%s pitch=%.3f voices=%d"),
            Event,*GetNameSafe(Sound),Pitch,Limit->Concurrency.MaxCount);
#endif
}
