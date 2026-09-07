#pragma once
#include "CoreMinimal.h"
#include "UI/HOORunnerWidgetBase.h"
#include "HOORunnerStatusWidget.generated.h"
class UTextBlock;class UProgressBar;class UFont;class UBorder;
UCLASS()
class SKYLINERUSH_API UHOORunnerStatusWidget:public UHOORunnerWidgetBase
{
 GENERATED_BODY()
protected:
 virtual void NativeOnInitialized() override;
 virtual void NativeTick(const FGeometry& Geometry,float Dt) override;
private:
 friend class AHOORunnerVisualQA;
 UPROPERTY() TObjectPtr<UFont> KoreanFont;
 UPROPERTY() TObjectPtr<UTextBlock> ScoreText;
 UPROPERTY() TObjectPtr<UTextBlock> SpeedText;
 UPROPERTY() TObjectPtr<UTextBlock> RouteText;
 UPROPERTY() TObjectPtr<UTextBlock> TurnText;
 UPROPERTY() TObjectPtr<UTextBlock> PowerText;
 UPROPERTY() TObjectPtr<UTextBlock> ComboText;
 UPROPERTY() TObjectPtr<UTextBlock> ToastText;
 UPROPERTY() TObjectPtr<UProgressBar> FeverBar;
 UPROPERTY() TObjectPtr<UProgressBar> RouteBar;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> Hearts;
 UPROPERTY() TObjectPtr<UBorder> ToastPanel;
 float Refresh=0,ToastTime=0,ScorePulse=0;
 int32 PreviousScore=-1,PreviousHits=0,PreviousFever=0,PreviousBoost=0,PreviousTheme=-1;
 FString PreviousRunId;
};
