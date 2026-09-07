#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/HOORunnerTypes.h"
#include "Gameplay/HOORunnerGameMode.h"
#include "HOORunnerPawn.generated.h"

class UCameraComponent;
class AHOORunnerScenery;
class AHOORunnerWorlds;
class UPointLightComponent;
class USkeletalMeshComponent;
class UInstancedStaticMeshComponent;
class UAnimSequence;
class USoundBase;
class UAudioComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class SKYLINERUSH_API AHOORunnerPawn : public APawn
{
    GENERATED_BODY()
public:
    AHOORunnerPawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UFUNCTION(BlueprintCallable, Category="Runner") void StartRun();
    UFUNCTION(BlueprintCallable, Category="Runner") void RestartRun();
    UFUNCTION(BlueprintCallable, Category="Runner") void MoveLeft();
    UFUNCTION(BlueprintCallable, Category="Runner") void MoveRight();
    UFUNCTION(BlueprintCallable, Category="Runner") void JumpAction();
    UFUNCTION(BlueprintCallable, Category="Runner") void SlideAction();
    UFUNCTION(BlueprintCallable, Category="Runner") void PauseRun();
    UFUNCTION(BlueprintPure, Category="Runner") FString GetRunSnapshot() const;
    UFUNCTION(BlueprintCallable, Category="Runner") void ReturnToMenu();
    UFUNCTION(BlueprintCallable, Category="Runner") void QuitGame();
    UFUNCTION(BlueprintCallable, Category="Runner") void CycleVolume();
    UFUNCTION(BlueprintCallable, Category="Runner") void CycleQuality();
    UFUNCTION(BlueprintCallable, Category="Runner") void ToggleReducedMotion();
    UFUNCTION(BlueprintCallable, Category="Runner") void ReloadPreferences();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runner") int32 VolumePercent=75;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runner") int32 QualityLevel=2;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runner") bool bReducedMotion=false;
    UFUNCTION(BlueprintCallable,Category="Runner") void RefreshOnlineRanking();
    UFUNCTION(BlueprintCallable,Category="Runner") void SubmitBestRun();
    UFUNCTION(BlueprintCallable,Category="Runner") bool SetPlayerNickname(const FString& Name);
    UFUNCTION(BlueprintPure,Category="Runner") FString GetRankingSnapshot() const;
    const TArray<FHOORunnerScoreEntry>& GetLocalScores() const { return LocalScores; }
    const TArray<FHOORunnerRankEntry>& GetOnlineRanks() const { return OnlineRanks; }
    const FString& GetNickname() const { return PlayerNickname; }
    const FString& GetRankStatus() const { return RankStatus; }
    bool IsOnlineBusy() const { return bOnlineBusy; }
    float GetStagePulse() const { return StagePulse; }
    bool IsNewRecord() const { return Run.Score()>StartingBestScore || Run.Distance/100.0 > FMath::Max(1.f,StartingBest); }
    bool HasSubmittableRun() const { return HOORunnerRecords::BestSubmittable(LocalScores)!=nullptr; }
    const FString& GetRunId() const { return RunId; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runner") TObjectPtr<USkeletalMeshComponent> RunnerMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runner") TObjectPtr<UCameraComponent> ChaseCamera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runner") float BestDistance = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runner") int32 BestScore = 0;
    const FHOORunnerState& GetRun() const { return Run; }
    float GetCountdown() const { return Countdown; }
    float GetPickupPulse() const { return PickupPulse; }
    const TCHAR* GetDistrict() const;

private:
    friend class AHOORunnerVisualQA;
    bool bVisualQAFrozen=false;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Pools;
    UPROPERTY() TObjectPtr<UPointLightComponent> AquariumLight;
    UPROPERTY() TObjectPtr<UPointLightComponent> HeroFill;
    UPROPERTY() TObjectPtr<AHOORunnerScenery> Scenery;
    UPROPERTY() TObjectPtr<AHOORunnerWorlds> Worlds;
    UPROPERTY() TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> RunAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> JumpAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> SlideAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> FallAnimation;
    UPROPERTY() TObjectPtr<USoundBase> PickupSound;
    UPROPERTY() TObjectPtr<USoundBase> CrashSound;
    UPROPERTY() TObjectPtr<UAudioComponent> MusicAudio;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> MusicTracks;
    int32 MusicTrackIndex = -1;
    float MusicVolume = -1.f;
    bool bMusicPaused = false;
    bool bMusicStopping = false;
    UFUNCTION() void PlayNextMusicTrack();
    void UpdateMusicSettings();
    UPROPERTY() TObjectPtr<UAnimSequence> ActiveAnimation;
    FHOORunnerState Run;
    TArray<int64> Slots;
    float Countdown = 0.0f;
    float DeathTime = 0.0f;
    float PickupPulse = 0.0f;
    float VisualTime = 0.0f;
    float SlideBlend = 0.0f;
    float StagePulse = 0.f;
    float StartingBest = 0.f;
    int32 StartingBestScore = 0;
    bool bCameraInitialized = false;
    int64 LastOrbTile = -1;
    int64 LastBoosterVisual = -1;
    int64 LastProtectedVisual = -1;
    TArray<FHOORunnerScoreEntry> LocalScores;
    TArray<FHOORunnerRankEntry> OnlineRanks;
    TArray<FString> ReplayInputs;
    FString RunId;
    FString PlayerNickname;
    FString OnlinePlayerId;
    FString OnlineToken;
    FString RankStatus=TEXT("온라인 랭킹을 확인해 보세요.");
    bool bOnlineBusy=false;
    bool bRunRecorded=false;
    bool bReplayOverflow=false;
    void RecordInput(const TCHAR* Action);
    void EnsureOnlineProfile(TFunction<void(bool)> Completion);
    FString RankingApi() const;

    UInstancedStaticMeshComponent* AddPool(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material, int32 PerTile);
    void UpdateCourse(bool bReset=false);
    void PlaceTile(int32 Slot, int64 Index);
    void UpdatePresentation(float Dt);
    void SaveRecord();
    void SavePreferences();
    void ApplyPreferences();
};
