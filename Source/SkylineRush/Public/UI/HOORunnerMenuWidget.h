#pragma once
#include "CoreMinimal.h"
#include "UI/HOORunnerWidgetBase.h"
#include "HOORunnerMenuWidget.generated.h"
class AHOORunnerHUD;class AHOORunnerPawn;class UTextBlock;class UButton;class UBorder;class UVerticalBox;
UCLASS()
class SKYLINERUSH_API UHOORunnerMenuWidget:public UHOORunnerWidgetBase
{
 GENERATED_BODY()
public:
 void Present(AHOORunnerPawn* Runner,AHOORunnerHUD* OwnerHUD);
 void Navigate(int32 Direction);
 void Confirm();
protected:
 virtual void NativeOnInitialized() override;
private:
 friend class AHOORunnerVisualQA;
 UPROPERTY() TObjectPtr<UTextBlock> Badge;
 UPROPERTY() TObjectPtr<UTextBlock> Title;
 UPROPERTY() TObjectPtr<UTextBlock> Description;
 UPROPERTY() TObjectPtr<UTextBlock> Score;
 UPROPERTY() TObjectPtr<UTextBlock> Detail;
 UPROPERTY() TObjectPtr<UTextBlock> PrimaryLabel;
 UPROPERTY() TObjectPtr<UTextBlock> HomeLabel;
 UPROPERTY() TObjectPtr<UBorder> PortraitPanel;
 UPROPERTY() TArray<TObjectPtr<UButton>> Actions;
 TWeakObjectPtr<AHOORunnerHUD> HUD;
 int32 Phase=-1,Focus=0;
 int32 PresentedTicks=-1,PresentedBestScore=-1;
 float PresentedBestDistance=-1;
 FString PresentedRunId;
 bool Ready=true;
 void StyleButtons();
 UButton* AddAction(UVerticalBox* Parent,FText Label,UTextBlock** Out=nullptr);
 UFUNCTION() void Primary();
 UFUNCTION() void Records();
 UFUNCTION() void Settings();
 UFUNCTION() void Home();
 UFUNCTION() void Quit();
};
