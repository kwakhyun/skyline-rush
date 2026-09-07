#include "Gameplay/HOORunnerPawn.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AHOORunnerPawn::PlayNextMusicTrack()
{
    if(bMusicStopping || !MusicAudio || MusicTracks.IsEmpty()) return;
    // A single component owns the playlist, including across retries and menu transitions.
    for(int32 Attempt=0;Attempt<MusicTracks.Num();++Attempt)
    {
        MusicTrackIndex=(MusicTrackIndex+1)%MusicTracks.Num();
        USoundBase* Track=MusicTracks[MusicTrackIndex];
        if(!Track) continue;
        float StartTime=0.f;
#if !UE_BUILD_SHIPPING
        // Exercise real decoder completion in QA without waiting for two full songs.
        float TailSeconds=0.f;
        if(FParse::Param(FCommandLine::Get(),TEXT("RunnerQA")) &&
            FParse::Value(FCommandLine::Get(),TEXT("RunnerMusicQATailSeconds="),TailSeconds) && TailSeconds>0.f)
            StartTime=FMath::Max(0.f,Track->GetDuration()-FMath::Clamp(TailSeconds,1.f,30.f));
#endif
        MusicAudio->SetSound(Track);
        MusicAudio->Play(StartTime);
        MusicAudio->SetPaused(bMusicPaused);
        UE_LOG(LogTemp,Display,TEXT("RUNNER_MUSIC_STARTED track=%d asset=%s start=%.2f duration=%.2f"),
            MusicTrackIndex+1,*GetNameSafe(Track),StartTime,Track->GetDuration());
        return;
    }
    UE_LOG(LogTemp,Warning,TEXT("RUNNER_MUSIC_UNAVAILABLE: import the two background music assets."));
}

void AHOORunnerPawn::UpdateMusicSettings()
{
    if(!MusicAudio || bMusicStopping) return;
    // Leave headroom for the pickup, collision and booster cues.
    const float Volume=.45f*FMath::Clamp(VolumePercent,0,100)/100.f;
    if(!FMath::IsNearlyEqual(MusicVolume,Volume))
    {
        MusicVolume=Volume;
        MusicAudio->SetVolumeMultiplier(Volume);
        UE_LOG(LogTemp,Display,TEXT("RUNNER_MUSIC_VOLUME %.4f"),Volume);
    }
    const bool Paused=Run.Phase==EHOORunnerPhase::Paused;
    if(bMusicPaused!=Paused)
    {
        bMusicPaused=Paused;
        MusicAudio->SetPaused(Paused);
        UE_LOG(LogTemp,Display,TEXT("RUNNER_MUSIC_PAUSED %d"),Paused?1:0);
    }
}
