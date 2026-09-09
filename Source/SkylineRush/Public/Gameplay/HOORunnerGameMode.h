#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SaveGame.h"
#include "HOORunnerGameMode.generated.h"
class UHOORunnerStatusWidget;class UHOORunnerMenuWidget;

USTRUCT(BlueprintType)
struct FHOORunnerScoreEntry
{
    GENERATED_BODY()
    UPROPERTY(SaveGame,BlueprintReadOnly) FString RunId;
    UPROPERTY(SaveGame,BlueprintReadOnly) FString Nickname;
    UPROPERTY(SaveGame,BlueprintReadOnly) FString Date;
    UPROPERTY(SaveGame,BlueprintReadOnly) int32 Score=0;
    UPROPERTY(SaveGame) int32 DistancePoints=0;
    UPROPERTY(SaveGame) int32 PickupPoints=0;
    UPROPERTY(SaveGame) int32 RiskPoints=0;
    UPROPERTY(SaveGame) int32 StylePoints=0;
    UPROPERTY(SaveGame,BlueprintReadOnly) float Distance=0;
    UPROPERTY(SaveGame,BlueprintReadOnly) float Seconds=0;
    UPROPERTY(SaveGame,BlueprintReadOnly) int32 Seed=409;
    UPROPERTY(SaveGame,BlueprintReadOnly) int32 Ticks=0;
    UPROPERTY(SaveGame) FString ReplayJson;
};
USTRUCT(BlueprintType)
struct FHOORunnerRankEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Rank=0;
    UPROPERTY(BlueprintReadOnly) FString Nickname;
    UPROPERTY(BlueprintReadOnly) int32 Score=0;
    UPROPERTY(BlueprintReadOnly) float Distance=0;
};

namespace HOORunnerRecords
{
    // Local records remain available even when an individual replay cannot be uploaded.
    inline const FHOORunnerScoreEntry* BestSubmittable(const TArray<FHOORunnerScoreEntry>& Runs)
    {
        const FHOORunnerScoreEntry* Best=nullptr;
        for(const auto& Entry:Runs)
        {
            if(Entry.RunId.IsEmpty() || Entry.ReplayJson.IsEmpty() || Entry.Ticks<1 || Entry.Ticks>144000 ||
                Entry.Seed<1 || Entry.Seed>1000000 || Entry.Score<0 || !FMath::IsFinite(Entry.Distance) || Entry.Distance<0) continue;
            if(!Best || Entry.Score>Best->Score || (Entry.Score==Best->Score && Entry.Distance>Best->Distance)) Best=&Entry;
        }
        return Best;
    }
}

UCLASS()
class SKYLINERUSH_API UHOORunnerRecord : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame,BlueprintReadOnly,Category="Runner") int32 Version = 6;
    UPROPERTY(SaveGame,BlueprintReadOnly,Category="Runner") TArray<FHOORunnerScoreEntry> Runs;
    UPROPERTY(SaveGame,BlueprintReadOnly,Category="Runner") float Distance = 0;
    UPROPERTY(SaveGame,BlueprintReadOnly,Category="Runner") int32 Score = 0;
};
UCLASS()
class SKYLINERUSH_API UHOORunnerPreferences : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame,BlueprintReadOnly,Category="Runner") int32 Volume = 75;
    UPROPERTY(SaveGame,BlueprintReadOnly,Category="Runner") int32 Quality = 2;
    UPROPERTY(SaveGame,BlueprintReadOnly,Category="Runner") bool bReducedMotion = false;
    UPROPERTY(SaveGame) FString Nickname;
    UPROPERTY(SaveGame) FString PlayerId;
    UPROPERTY(SaveGame) FString PlayerToken;
};

enum class ERunnerMenuAction : uint8 { Primary, NewCourse, Home, Settings, Quit, Volume, Quality, Motion, Close, Records, Online, Local, Refresh, Submit, Nickname };
struct FRunnerMenuButton { FBox2D Bounds; ERunnerMenuAction Action; bool bEnabled=true; };

UCLASS()
class SKYLINERUSH_API AHOORunnerHUD : public AHUD
{
    GENERATED_BODY()
public:
    AHOORunnerHUD();
    virtual void DrawHUD() override;
    UFUNCTION(BlueprintCallable,Category="Runner") void ActivateMenu(float X,float Y);
    UFUNCTION(BlueprintCallable,Category="Runner") void ToggleSettings();
    UFUNCTION(BlueprintPure,Category="Runner") bool IsSettingsOpen() const { return bSettingsOpen; }
    UFUNCTION(BlueprintCallable,Category="Runner") void ToggleRecords();
    UFUNCTION(BlueprintPure,Category="Runner") bool IsRecordsOpen() const { return bRecordsOpen; }
    bool IsMenuOpen() const;
    bool IsEditingNickname() const { return bEditingNickname; }
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void Navigate(int32 Direction);
    void ConfirmSelection();
    void Back();
    void Execute(ERunnerMenuAction Action);
private:
    friend class AHOORunnerVisualQA;
    UPROPERTY() TObjectPtr<class UFont> TextFont;
    UPROPERTY() TObjectPtr<UHOORunnerStatusWidget> StatusWidget;
    UPROPERTY() TObjectPtr<UHOORunnerMenuWidget> MenuWidget;
    TArray<FRunnerMenuButton> Buttons;
    int32 FocusedButton=0;
    int32 LastPhase=-1;
    bool bSettingsOpen=false;
    bool bRecordsOpen=false;
    bool bOnlineTab=false;
    bool bEditingNickname=false;
    bool bResumeAfterRecords=false;
    TSharedPtr<class SWidget> NicknameOverlay;
    void EditNickname();
    void CloseNickname();
    UPROPERTY() TObjectPtr<class UTexture2D> HeroPortrait;
    UPROPERTY() TObjectPtr<class UTexture2D> TitleIllustration;
    UPROPERTY() TObjectPtr<class UTexture2D> MenuIllustration;
    bool bResumeAfterSettings=false;
    bool bLastMenuInput=false;
};

UCLASS()
class SKYLINERUSH_API AHOORunnerController : public APlayerController
{
    GENERATED_BODY()
public:
    AHOORunnerController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    void OnApplicationActivationChanged(bool bActive);
    // The same dispatch path is used by bound keyboard/gamepad input and PIE QA.
    UFUNCTION(BlueprintCallable,Category="Runner") void DispatchRunnerInput(FName Command);
private:
    bool bStickHeld=false;
    FDelegateHandle ApplicationActivationHandle;
    void Left(); void Right(); void Up(); void Down(); void Confirm();
    void Restart(); void Pause(); void Cancel(); void Settings(); void Records(); void ClickMenu();
    void Stick(float Value); void MenuStick(float Value);
    bool bMenuStickHeld=false;
};
UCLASS()
class SKYLINERUSH_API AHOORunnerGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AHOORunnerGameMode();
};
