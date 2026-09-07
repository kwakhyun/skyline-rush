#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "HOORunnerWidgetBase.generated.h"
class UFont;class UTextBlock;class UVerticalBox;class UCanvasPanel;class UBorder;
UCLASS(Abstract)
class SKYLINERUSH_API UHOORunnerWidgetBase:public UUserWidget
{
 GENERATED_BODY()
protected:
 virtual void NativeOnInitialized() override;
 UVerticalBox* AeroPanel(UCanvasPanel* Root,FVector2D Anchor,FVector2D Align,FVector2D Position,FVector2D Size,UBorder** Out=nullptr);
 UTextBlock* AeroText(UVerticalBox* Parent,FText Text,int32 Size,FLinearColor Color);
 UPROPERTY() TObjectPtr<UFont> UIFont;
};
UCLASS()
class SKYLINERUSH_API UHOORunnerButton:public UButton
{
 GENERATED_BODY()
public:
 UHOORunnerButton(const FObjectInitializer& Init):Super(Init){InitIsFocusable(false);}
};
