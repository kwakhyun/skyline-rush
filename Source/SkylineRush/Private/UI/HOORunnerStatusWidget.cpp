#include "UI/HOORunnerStatusWidget.h"
#include "QA/HOORunnerVisualQA.h"
#include "UI/HOORunnerStyle.h"
#include "Components/SafeZone.h"
#include "Components/SizeBox.h"
#include "Gameplay/HOORunnerPawn.h"
#include "World/HOORunnerWorlds.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/Font.h"
#include "Misc/Paths.h"
void UHOORunnerStatusWidget::NativeOnInitialized()
{
 Super::NativeOnInitialized();
 KoreanFont=UIFont;
 auto* Safe=WidgetTree->ConstructWidget<USafeZone>();WidgetTree->RootWidget=Safe;
 auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();Safe->AddChild(Root);
 const FLinearColor Pale=HOOAero::Muted,White=HOOAero::Text,Cyan=HOOAero::Cyan;
 auto Panel=[&](FVector2D Anchor,FVector2D Align,FVector2D Position,FVector2D Size,TObjectPtr<UBorder>* Out=nullptr)
 {
  UBorder* B=nullptr;auto* Stack=AeroPanel(Root,Anchor,Align,Position,Size,&B);B->SetPadding(FMargin(16,10));if(Out)*Out=B;return Stack;
 };
 auto Text=[&](UVerticalBox* Parent,const FString& Value,int32 Size,FLinearColor Color)
 {
  auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));T->SetFont(FSlateFontInfo(KoreanFont,Size));T->SetColorAndOpacity(FSlateColor(Color));T->SetShadowOffset(FVector2D(0,2));T->SetShadowColorAndOpacity(FLinearColor(0,0,0,.4f));Parent->AddChildToVerticalBox(T);return T;
 };
 auto Bar=[&](UVerticalBox* Parent,FLinearColor Color)
 {
  auto* B=WidgetTree->ConstructWidget<UProgressBar>();FProgressBarStyle Style;
  Style.BackgroundImage.DrawAs=ESlateBrushDrawType::RoundedBox;Style.BackgroundImage.OutlineSettings.CornerRadii=FVector4(5,5,5,5);Style.BackgroundImage.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;Style.BackgroundImage.TintColor=FSlateColor(FLinearColor(.08f,.14f,.21f,1));Style.BackgroundImage.ImageSize=FVector2D(8,10);
  Style.FillImage=Style.BackgroundImage;Style.FillImage.TintColor=FSlateColor(FLinearColor::White);
  B->SetWidgetStyle(Style);B->SetBarFillStyle(EProgressBarFillStyle::Scale);B->SetFillColorAndOpacity(Color);auto* Box=WidgetTree->ConstructWidget<USizeBox>();Box->SetHeightOverride(8);Box->AddChild(B);auto* Slot=Parent->AddChildToVerticalBox(Box);Slot->SetPadding(FMargin(0,8,0,6));B->SetPercent(0);return B;
 };
 auto* Score=Panel({0,0},{0,0},{26,24},{222,92});Text(Score,TEXT("점수"),12,Pale);ScoreText=Text(Score,TEXT("0"),38,White);
 auto* Life=Panel({.5,0},{.5,0},{0,24},{308,138});
 auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Life->AddChildToVerticalBox(Row)->SetHorizontalAlignment(HAlign_Center);
 for(int I=0;I<3;++I){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(TEXT("♥")));T->SetFont(FSlateFontInfo(KoreanFont,29));T->SetColorAndOpacity(FSlateColor(HOOAero::Coral));Row->AddChildToHorizontalBox(T)->SetPadding(FMargin(10,0));Hearts.Add(T);}
 RouteText=Text(Life,TEXT("햇살 도심"),12,Pale);RouteText->SetJustification(ETextJustify::Center);RouteBar=Bar(Life,Cyan);TurnText=Text(Life,TEXT("자동 주행 · 세 레인"),11,Cyan);TurnText->SetJustification(ETextJustify::Center);
 auto* Speed=Panel({1,0},{1,0},{-26,24},{182,92});Text(Speed,TEXT("속도  /  km/h"),12,Pale);SpeedText=Text(Speed,TEXT("50"),38,Cyan);
 auto* Power=Panel({0,1},{0,1},{26,-27},{260,122});PowerText=Text(Power,TEXT("FEVER  ·  크리스털을 모아요"),15,FLinearColor(1,.35f,.67f));FeverBar=Bar(Power,FLinearColor(1,.18f,.54f));ComboText=Text(Power,TEXT("콤보 0  ·  점수 1배"),14,White);
 auto* Toast=Panel({.5,0},{.5,0},{0,165},{540,70},&ToastPanel);ToastText=Text(Toast,TEXT(""),21,White);ToastText->SetJustification(ETextJustify::Center);ToastPanel->SetRenderOpacity(0);ToastPanel->SetVisibility(ESlateVisibility::Collapsed);
 auto* Special=Panel({1,1},{1,1},{-26,-27},{430,108},&SectionPanel);
 SectionText=Text(Special,TEXT(""),20,HOOAero::Gold);SectionHint=Text(Special,TEXT(""),14,White);
 SectionPanel->SetVisibility(ESlateVisibility::Collapsed);
 SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UHOORunnerStatusWidget::NativeTick(const FGeometry& Geometry,float Dt)
{
    CSV_SCOPED_TIMING_STAT(RunnerUI,UMG);
 Super::NativeTick(Geometry,Dt);
 auto* P=Cast<AHOORunnerPawn>(GetOwningPlayerPawn());if(!P)return;const auto& R=P->GetRun();
 if(PreviousRunId!=P->GetRunId())
 {
  PreviousNear=R.NearMisses;PreviousRisk=R.RiskClears;
  PreviousRunId=P->GetRunId();Refresh=0;ToastTime=0;ScorePulse=0;PreviousScore=-1;
  PreviousHits=R.Hits;PreviousFever=R.FeverActivations;PreviousBoost=R.BoostersCollected;PreviousTheme=static_cast<int32>(HOORunnerWorld::Biome(R.Distance));
  ToastText->SetText(FText::GetEmpty());ToastPanel->SetRenderOpacity(0);ScoreText->SetRenderScale(FVector2D(1));
 }
 ToastPanel->SetVisibility(ToastTime>0?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
 ToastTime=FMath::Max(0.f,ToastTime-Dt);ScorePulse=FMath::Max(0.f,ScorePulse-Dt);
 if(ToastTime>0 || ToastPanel->GetRenderOpacity()>0)ToastPanel->SetRenderOpacity(FMath::Min(1.f,ToastTime*3));if(ScorePulse>0 || ScoreText->GetRenderTransform().Scale.X!=1)ScoreText->SetRenderScale(FVector2D(1+ScorePulse*.10));
 Refresh-=Dt;if(Refresh>0)return;Refresh=.05f;
 auto Change=[](UTextBlock* T,const FString& Value){if(T->GetText().ToString()!=Value)T->SetText(FText::FromString(Value));};
 const int32 Score=R.Score();if(Score!=PreviousScore){Change(ScoreText,FText::AsNumber(Score).ToString());if(Score-PreviousScore>20)ScorePulse=.4f;PreviousScore=Score;}
 Change(SpeedText,FString::FromInt(FMath::RoundToInt(R.CurrentSpeed*.036)));
 const float Turn=HOORunner::TurnPreview(R.Distance);
 const FString NextSpace=FString::Printf(TEXT("다음 공간까지 %.0f m"),HOORunnerWorld::Remaining(R.Distance)/100);
 Change(TurnText,FMath::Abs(Turn)>5?(Turn>0?TEXT("우측 코너 · 자동 회전"):TEXT("좌측 코너 · 자동 회전")):*NextSpace);
 Change(RouteText,FString::Printf(TEXT("%s  ·  %.0f m"),P->GetDistrict(),R.Distance/100));
 RouteBar->SetPercent(HOORunnerWorld::Progress(R.Distance));
 for(int I=0;I<Hearts.Num();++I){Hearts[I]->SetColorAndOpacity(FSlateColor(I<R.Lives?HOOAero::Coral:FLinearColor(.23f,.29f,.36f)));Hearts[I]->SetRenderScale(FVector2D(I==R.Lives && R.RecoveryRemaining>2?1.2:1));}
 const float Charge=R.IsFever()?R.FeverRemaining/8.f:R.FeverCharge/24.f;FeverBar->SetPercent(Charge);
 Change(PowerText,R.IsFever()?FString::Printf(TEXT("FEVER  ·  보호 %.1f초"),R.FeverRemaining):FString::Printf(TEXT("FEVER  ·  %d / 24"),R.FeverCharge));
 ComboText->SetColorAndOpacity(FSlateColor(R.IsBoosting()?HOOAero::Cyan:HOOAero::Text));
 Change(ComboText,R.IsBoosting()?FString::Printf(TEXT("BOOST  %.1f초  ·  콤보 %d"),R.BoostRemaining,R.Chain):FString::Printf(TEXT("콤보 %d  ·  점수 %d배"),R.Chain,R.Multiplier()));
 const auto Section=HOORunner::Special(R.Distance);
 const auto Preview=Section;
 SectionPanel->SetVisibility(Preview==EHOORunnerSpecial::None?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
 if(Preview!=EHOORunnerSpecial::None)
 {
  Change(SectionText,FString::Printf(TEXT("%s%s"),Section==EHOORunnerSpecial::None?TEXT("곧 진입 · "):TEXT(""),HOORunner::SpecialName(Preview)));
  FString Hint=TEXT("청록색 중앙 · 안전하게 통과");
  const int64 First=FMath::FloorToInt64(R.Distance/600);
  for(int I=0;I<25;++I){const auto T=HOORunner::Tile(First+I,R.Seed);const double Ahead=(First+I+.5)*600-R.Distance;
   if(T.Risk!=EHOORunnerHazard::None && Ahead>0){const TCHAR* Action=T.Risk==EHOORunnerHazard::Overhead?TEXT("슬라이드"):TEXT("점프");Hint+=FString::Printf(TEXT("\n금색 %s · %s 도전 %.0f m"),T.RiskLane<0?TEXT("왼쪽"):TEXT("오른쪽"),Action,Ahead/100);break;}}
  Change(SectionHint,Hint);
 }
 FString Toast;
 if(R.Hits>PreviousHits && R.Lives>0)Toast=FString::Printf(TEXT("다시 달려요!  ·  남은 목숨 %d"),R.Lives);
 else if(R.FeverActivations>PreviousFever)Toast=TEXT("FEVER TIME  ·  지금은 무적!");
 else if(R.BoostersCollected>PreviousBoost)Toast=TEXT("BOOST!  ·  한계를 넘어!");
 else if(R.RiskClears>PreviousRisk)Toast=TEXT("도전 성공!  ·  위험 보상 획득");
 else if(R.NearMisses>PreviousNear)Toast=TEXT("아슬아슬!  ·  정밀 회피 보너스");
 else if(PreviousTheme>=0 && PreviousTheme!=static_cast<int32>(HOORunnerWorld::Biome(R.Distance)))Toast=P->GetDistrict();
 if(!Toast.IsEmpty()){Change(ToastText,Toast);ToastTime=1.5f;}
 PreviousNear=R.NearMisses;PreviousRisk=R.RiskClears;
 PreviousHits=R.Hits;PreviousFever=R.FeverActivations;PreviousBoost=R.BoostersCollected;PreviousTheme=static_cast<int32>(HOORunnerWorld::Biome(R.Distance));
}
