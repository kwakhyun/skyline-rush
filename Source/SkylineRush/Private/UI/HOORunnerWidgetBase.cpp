#include "UI/HOORunnerWidgetBase.h"
#include "UI/HOORunnerStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Misc/Paths.h"
void UHOORunnerWidgetBase::NativeOnInitialized()
{
 Super::NativeOnInitialized();
 UIFont=NewObject<UFont>(this);UIFont->FontCacheType=EFontCacheType::Runtime;
 UIFont->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"),FPaths::EngineContentDir()/TEXT("Slate/Fonts/DroidSansFallback.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad));
}
UVerticalBox* UHOORunnerWidgetBase::AeroPanel(UCanvasPanel* Root,FVector2D Anchor,FVector2D Align,FVector2D Position,FVector2D Size,UBorder** Out)
{
 auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrush(HOOAero::Brush(HOOAero::Panel));B->SetPadding(FMargin(24,20));
 auto* S=Root->AddChildToCanvas(B);S->SetAnchors(FAnchors(Anchor.X,Anchor.Y));S->SetAlignment(Align);S->SetPosition(Position);S->SetSize(Size);
 auto* V=WidgetTree->ConstructWidget<UVerticalBox>();B->AddChild(V);if(Out)*Out=B;return V;
}
UTextBlock* UHOORunnerWidgetBase::AeroText(UVerticalBox* Parent,FText Text,int32 Size,FLinearColor Color)
{
 auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(Text);T->SetFont(FSlateFontInfo(UIFont,Size));T->SetColorAndOpacity(FSlateColor(Color));
 Parent->AddChildToVerticalBox(T)->SetPadding(FMargin(0,0,0,12));return T;
}