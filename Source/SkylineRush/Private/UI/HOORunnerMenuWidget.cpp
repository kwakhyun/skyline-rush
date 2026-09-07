#include "UI/HOORunnerMenuWidget.h"
#include "Gameplay/HOORunnerPawn.h"
#include "Gameplay/HOORunnerGameMode.h"
#include "UI/HOORunnerStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/SafeZone.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#define LOCTEXT_NAMESPACE "SkylineRushMenu"
void UHOORunnerMenuWidget::NativeOnInitialized()
{
 Super::NativeOnInitialized();
 auto* Safe=WidgetTree->ConstructWidget<USafeZone>();WidgetTree->RootWidget=Safe;
 auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();Safe->AddChild(Root);
 auto* Main=AeroPanel(Root,{0,.5},{0,.5},{32,0},{510,714});
 Badge=AeroText(Main,LOCTEXT("Edition","FIVE WORLDS  /  SKYLINE RUSH"),12,HOOAero::Cyan);
 Title=AeroText(Main,LOCTEXT("Title","스카이라인\n러시"),42,HOOAero::Text);
 Description=AeroText(Main,LOCTEXT("Subtitle","바람을 가르는 나만의 질주"),18,HOOAero::Muted);
 Score=AeroText(Main,FText::GetEmpty(),29,HOOAero::Gold);
 Detail=AeroText(Main,FText::GetEmpty(),14,HOOAero::Muted);
 UTextBlock* L=nullptr;auto* B=AddAction(Main,LOCTEXT("Start","달리기 시작  /  Enter"),&L);PrimaryLabel=L;B->OnClicked.AddDynamic(this,&UHOORunnerMenuWidget::Primary);
 B=AddAction(Main,LOCTEXT("Records","기록과 랭킹"));B->OnClicked.AddDynamic(this,&UHOORunnerMenuWidget::Records);
 B=AddAction(Main,LOCTEXT("Settings","게임 설정"));B->OnClicked.AddDynamic(this,&UHOORunnerMenuWidget::Settings);
 B=AddAction(Main,LOCTEXT("New","새 코스로 출발"),&L);HomeLabel=L;B->OnClicked.AddDynamic(this,&UHOORunnerMenuWidget::Home);
 B=AddAction(Main,LOCTEXT("Quit","게임 종료"));B->OnClicked.AddDynamic(this,&UHOORunnerMenuWidget::Quit);
 AeroText(Main,LOCTEXT("Controls","A / D 이동 · Space 점프 · S 슬라이드"),12,HOOAero::Muted);
 UBorder* Portrait=nullptr;
 auto* Side=AeroPanel(Root,{1,.5},{1,.5},{-32,0},{278,370},&Portrait);PortraitPanel=Portrait;
 auto* Im=WidgetTree->ConstructWidget<UImage>();
 Im->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/SkylineRush/UI/T_Dialogue_StudentHero_Portrait_v1")));
 auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetHeightOverride(246);Size->AddChild(Im);Side->AddChildToVerticalBox(Size)->SetPadding(FMargin(0,0,0,18));
 AeroText(Side,LOCTEXT("Together","함께 달려요"),22,HOOAero::Cyan);
 AeroText(Side,LOCTEXT("Hearts","세 개의 하트, 끝없는 가능성"),12,HOOAero::Muted);
 StyleButtons();
}
UButton* UHOORunnerMenuWidget::AddAction(UVerticalBox* Parent,FText Label,UTextBlock** Out)
{
 auto* B=WidgetTree->ConstructWidget<UHOORunnerButton>();
 auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(Label);T->SetFont(FSlateFontInfo(UIFont,17));T->SetColorAndOpacity(FSlateColor(HOOAero::Text));
 B->AddChild(T);
 auto* Box=WidgetTree->ConstructWidget<USizeBox>();Box->SetHeightOverride(48);Box->AddChild(B);Parent->AddChildToVerticalBox(Box)->SetPadding(FMargin(0,0,0,10));
 Actions.Add(B);if(Out)*Out=T;return B;
}
void UHOORunnerMenuWidget::StyleButtons()
{
 for(int32 I=0;I<Actions.Num();++I)
 {
  FButtonStyle S;S.Normal=HOOAero::Brush(I==Focus?FLinearColor(.04f,.31f,.39f):HOOAero::Surface);
  S.Hovered=HOOAero::Brush(FLinearColor(.06f,.40f,.48f));S.Pressed=HOOAero::Brush(FLinearColor(.02f,.20f,.26f));
  S.Disabled=HOOAero::Brush(FLinearColor(.06f,.08f,.09f));S.NormalPadding=FMargin(14,6);S.PressedPadding=FMargin(14,8,14,4);
  Actions[I]->SetStyle(S);
 }
}
void UHOORunnerMenuWidget::Present(AHOORunnerPawn* Runner,AHOORunnerHUD* OwnerHUD)
{
 HUD=OwnerHUD;const auto& R=Runner->GetRun();
 const int32 NewPhase=static_cast<int32>(R.Phase);
 // Menus do not receive Present while running. Phase alone would reuse the previous
 // run's result, or the previous pause's score, when the same screen reopens.
 if(NewPhase==Phase && PresentedRunId==Runner->GetRunId() && PresentedTicks==R.SimulationTicks &&
    PresentedBestScore==Runner->BestScore && PresentedBestDistance==Runner->BestDistance)return;
 Phase=NewPhase;PresentedRunId=Runner->GetRunId();PresentedTicks=R.SimulationTicks;
 PresentedBestScore=Runner->BestScore;PresentedBestDistance=Runner->BestDistance;Focus=0;StyleButtons();
 Ready=R.Phase==EHOORunnerPhase::Ready;const bool Paused=R.Phase==EHOORunnerPhase::Paused;
 Badge->SetText(Ready?LOCTEXT("Edition","FIVE WORLDS  /  SKYLINE RUSH"):Paused?LOCTEXT("PauseBadge","TAKE A BREATH  /  잠시 쉬어가기"):Runner->IsNewRecord()?LOCTEXT("BestBadge","PERSONAL BEST  /  새로운 최고기록"):LOCTEXT("ResultBadge","RUN COMPLETE  /  오늘의 기록"));
 Title->SetText(Ready?LOCTEXT("Title","스카이라인\n러시"):Paused?LOCTEXT("PauseTitle","잠깐의 쉼,\n다음 질주를 위해"):LOCTEXT("ResultTitle","멋진 질주였어요!"));
 Description->SetText(Ready?LOCTEXT("Subtitle","바람을 가르는 나만의 질주"):Paused?LOCTEXT("PauseDesc","준비되면 카운트다운 후 이어 달려요."):LOCTEXT("ResultDesc","다음에는 조금 더 멀리, 조금 더 빠르게."));
 Score->SetText(FText::Format(LOCTEXT("Score","{0}점  ·  {1} m"),FText::AsNumber(Ready?Runner->BestScore:R.Score()),FText::AsNumber(FMath::FloorToInt(Ready?Runner->BestDistance:R.Distance/100.))));
 Detail->SetText(Ready?LOCTEXT("ReadyDetail","내 최고기록 · 코너에서는 길을 따라 자동 회전해요."):FText::Format(LOCTEXT("Detail","결정 {0}개  ·  피버 {1}회  ·  비행 {2}회"),FText::AsNumber(R.Coins),FText::AsNumber(R.FeverActivations),FText::AsNumber(R.Launches)));
 PrimaryLabel->SetText(Ready?LOCTEXT("Start","달리기 시작  /  Enter"):Paused?LOCTEXT("Resume","이어서 달리기  /  Enter"):LOCTEXT("Again","한 번 더 달리기  /  Enter"));
 HomeLabel->SetText(Ready?LOCTEXT("New","새 코스로 출발"):LOCTEXT("Home","처음 화면"));
 PortraitPanel->SetVisibility(Ready?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
}
void UHOORunnerMenuWidget::Navigate(int32 D){Focus=(Focus+D+Actions.Num())%Actions.Num();StyleButtons();}
void UHOORunnerMenuWidget::Confirm(){switch(Focus){case 0:Primary();break;case 1:Records();break;case 2:Settings();break;case 3:Home();break;default:Quit();}}
void UHOORunnerMenuWidget::Primary(){if(HUD.IsValid())HUD->Execute(ERunnerMenuAction::Primary);}
void UHOORunnerMenuWidget::Records(){if(HUD.IsValid())HUD->Execute(ERunnerMenuAction::Records);}
void UHOORunnerMenuWidget::Settings(){if(HUD.IsValid())HUD->Execute(ERunnerMenuAction::Settings);}
void UHOORunnerMenuWidget::Home(){if(HUD.IsValid())HUD->Execute(Ready?ERunnerMenuAction::NewCourse:ERunnerMenuAction::Home);}
void UHOORunnerMenuWidget::Quit(){if(HUD.IsValid())HUD->Execute(ERunnerMenuAction::Quit);}
#undef LOCTEXT_NAMESPACE
