#include "Gameplay/HOORunnerGameMode.h"
#include "QA/HOORunnerVisualQA.h"
#include "UI/HOORunnerMenuWidget.h"
#include "UI/HOORunnerStyle.h"
#include "Gameplay/HOORunnerPawn.h"
#include "UI/HOORunnerStatusWidget.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "CanvasItem.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Texture2D.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Framework/Application/SlateApplication.h"
#include "RenderUtils.h"

AHOORunnerGameMode::AHOORunnerGameMode()
{
    DefaultPawnClass=AHOORunnerPawn::StaticClass();
    PlayerControllerClass=AHOORunnerController::StaticClass();
    HUDClass=AHOORunnerHUD::StaticClass();
}
AHOORunnerController::AHOORunnerController() { bShowMouseCursor=true; }
void AHOORunnerController::BeginPlay()
{
    Super::BeginPlay();
    FInputModeGameOnly Mode; Mode.SetConsumeCaptureMouseDown(false); SetInputMode(Mode);
    // Automated captures manage their own foreground state. Normal play pauses on app switch.
    if(FSlateApplication::IsInitialized() && !FParse::Param(FCommandLine::Get(),TEXT("RunnerVisualQA")))
        ApplicationActivationHandle=FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this,&AHOORunnerController::OnApplicationActivationChanged);
}
void AHOORunnerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if(ApplicationActivationHandle.IsValid() && FSlateApplication::IsInitialized())
        FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ApplicationActivationHandle);
    Super::EndPlay(Reason);
}
void AHOORunnerController::OnApplicationActivationChanged(bool bActive)
{
    if(bActive) return; // Resume requires an explicit action, followed by the existing countdown.
    bStickHeld=false;bMenuStickHeld=false;
    if(auto* P=Cast<AHOORunnerPawn>(GetPawn()))
        if(P->GetRun().Phase==EHOORunnerPhase::Running) P->PauseRun();
}
void AHOORunnerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    for(const auto K:{EKeys::A,EKeys::Left,EKeys::Gamepad_DPad_Left}) InputComponent->BindKey(K,IE_Pressed,this,&AHOORunnerController::Left);
    for(const auto K:{EKeys::D,EKeys::Right,EKeys::Gamepad_DPad_Right}) InputComponent->BindKey(K,IE_Pressed,this,&AHOORunnerController::Right);
    for(const auto K:{EKeys::W,EKeys::Up,EKeys::Gamepad_DPad_Up}) InputComponent->BindKey(K,IE_Pressed,this,&AHOORunnerController::Up);
    for(const auto K:{EKeys::S,EKeys::Down,EKeys::Gamepad_DPad_Down}) InputComponent->BindKey(K,IE_Pressed,this,&AHOORunnerController::Down);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right,IE_Pressed,this,&AHOORunnerController::Cancel);
    for(const auto K:{EKeys::SpaceBar,EKeys::Enter,EKeys::Gamepad_FaceButton_Bottom}) InputComponent->BindKey(K,IE_Pressed,this,&AHOORunnerController::Confirm);
    for(const auto K:{EKeys::Escape,EKeys::P,EKeys::Gamepad_Special_Right}) InputComponent->BindKey(K,IE_Pressed,this,&AHOORunnerController::Pause);
    InputComponent->BindKey(EKeys::R,IE_Pressed,this,&AHOORunnerController::Restart);
    InputComponent->BindKey(EKeys::F2,IE_Pressed,this,&AHOORunnerController::Records);
    InputComponent->BindKey(EKeys::F1,IE_Pressed,this,&AHOORunnerController::Settings);
    InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&AHOORunnerController::ClickMenu);
    InputComponent->BindAxisKey(EKeys::Gamepad_LeftX,this,&AHOORunnerController::Stick);
    InputComponent->BindAxisKey(EKeys::Gamepad_LeftY,this,&AHOORunnerController::MenuStick);
}
void AHOORunnerController::DispatchRunnerInput(FName Command)
{
    auto* P=Cast<AHOORunnerPawn>(GetPawn());
    auto* H=Cast<AHOORunnerHUD>(GetHUD());
    if(!P || !H) return;
    if(H->IsEditingNickname()) {if(Command==TEXT("pause") || Command==TEXT("cancel")) H->Back();return;}
    const bool Menu=H->IsMenuOpen();
    if(Command==TEXT("cancel")) {if(Menu)H->Back();else P->SlideAction();return;}
    if(Command==TEXT("records")) { H->ToggleRecords();return; }
    if(Command==TEXT("settings")) { H->ToggleSettings();return; }
    if(Command==TEXT("pause")) { H->Back();return; }
    if(Command==TEXT("confirm")) { if(Menu) H->ConfirmSelection();else P->JumpAction();return; }
    if(Command==TEXT("restart")) { if(!H->IsSettingsOpen() && !H->IsRecordsOpen()) P->RestartRun();return; }
    if(Command!=TEXT("left") && Command!=TEXT("right") && Command!=TEXT("up") && Command!=TEXT("down"))return;
    const int32 Direction=(Command==TEXT("left") || Command==TEXT("up"))?-1:1;
    if(Menu) { H->Navigate(Direction);return; }
    if(Command==TEXT("left")) P->MoveLeft();
    else if(Command==TEXT("right")) P->MoveRight();
    else if(Command==TEXT("up")) P->JumpAction();
    else if(Command==TEXT("down")) P->SlideAction();
}
void AHOORunnerController::Left() { DispatchRunnerInput(TEXT("left")); }
void AHOORunnerController::Right() { DispatchRunnerInput(TEXT("right")); }
void AHOORunnerController::Up() { DispatchRunnerInput(TEXT("up")); }
void AHOORunnerController::Down() { DispatchRunnerInput(TEXT("down")); }
void AHOORunnerController::Confirm() { DispatchRunnerInput(TEXT("confirm")); }
void AHOORunnerController::Restart() { DispatchRunnerInput(TEXT("restart")); }
void AHOORunnerController::Pause() { DispatchRunnerInput(TEXT("pause")); }
void AHOORunnerController::Cancel() { DispatchRunnerInput(TEXT("cancel")); }
void AHOORunnerController::Records() { DispatchRunnerInput(TEXT("records")); }
void AHOORunnerController::Settings() { DispatchRunnerInput(TEXT("settings")); }
void AHOORunnerController::ClickMenu()
{
    float X,Y;
    if(GetMousePosition(X,Y)) if(auto* H=Cast<AHOORunnerHUD>(GetHUD())) H->ActivateMenu(X,Y);
}
void AHOORunnerController::Stick(float Value)
{
    if(FMath::Abs(Value)<.25f) bStickHeld=false;
    if(!bStickHeld && FMath::Abs(Value)>.6f) { bStickHeld=true; if(Value<0) Left();else Right(); }
}
void AHOORunnerController::MenuStick(float Value)
{
    if(FMath::Abs(Value)<.25f)bMenuStickHeld=false;
    auto* H=Cast<AHOORunnerHUD>(GetHUD());
    if(H && H->IsMenuOpen() && !bMenuStickHeld && FMath::Abs(Value)>.6f)
    {bMenuStickHeld=true;if(Value>0)Up();else Down();}
}
AHOORunnerHUD::AHOORunnerHUD()
{
    TextFont=CreateDefaultSubobject<UFont>(TEXT("RunnerKoreanFont"));
    TextFont->FontCacheType=EFontCacheType::Runtime;
    const FString FontPath=FPaths::EngineContentDir()/TEXT("Slate/Fonts/DroidSansFallback.ttf");
    auto& Typeface=TextFont->GetMutableInternalCompositeFont().DefaultTypeface;
    Typeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"),FontPath,EFontHinting::Default,EFontLoadingPolicy::LazyLoad));
    Typeface.Fonts.Add(FTypefaceEntry(TEXT("Bold"),FontPath,EFontHinting::Default,EFontLoadingPolicy::LazyLoad));
    static ConstructorHelpers::FObjectFinder<UTexture2D> Portrait(TEXT("/Game/SkylineRush/UI/T_Dialogue_StudentHero_Portrait_v1"));
    if(Portrait.Succeeded()) HeroPortrait=Portrait.Object;
}
void AHOORunnerHUD::ToggleRecords()
{
    auto* P=Cast<AHOORunnerPawn>(GetOwningPawn());if(!P || bSettingsOpen) return;
    if(!bRecordsOpen) {bResumeAfterRecords=P->GetRun().Phase==EHOORunnerPhase::Running;if(bResumeAfterRecords) P->PauseRun();}
    else {CloseNickname();if(bResumeAfterRecords && P->GetRun().Phase==EHOORunnerPhase::Paused) P->PauseRun();bResumeAfterRecords=false;}
    bRecordsOpen=!bRecordsOpen;FocusedButton=0;Buttons.Reset();
}
void AHOORunnerHUD::CloseNickname()
{
    if(NicknameOverlay.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(NicknameOverlay.ToSharedRef());
    NicknameOverlay.Reset();bEditingNickname=false;
    if(PlayerOwner) {FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);PlayerOwner->SetInputMode(Mode);}
}
void AHOORunnerHUD::EndPlay(const EEndPlayReason::Type Reason) {if(StatusWidget) StatusWidget->RemoveFromParent();if(MenuWidget) MenuWidget->RemoveFromParent();CloseNickname();Super::EndPlay(Reason);}
void AHOORunnerHUD::EditNickname()
{
    auto* P=Cast<AHOORunnerPawn>(GetOwningPawn());if(!P || !GEngine || !GEngine->GameViewport) return;
    CloseNickname();bEditingNickname=true;
    TWeakObjectPtr<AHOORunnerHUD> Weak(this);
    TSharedPtr<SEditableTextBox> Input;
    NicknameOverlay=SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
    [SNew(SBox).WidthOverride(390).HeightOverride(62)
    [SAssignNew(Input,SEditableTextBox).Text(FText::FromString(P->GetNickname())).Font(FSlateFontInfo(TextFont,22))
    .SelectAllTextWhenFocused(true)
    .OnTextCommitted_Lambda([Weak](const FText& Text,ETextCommit::Type Commit)
    {
        if(!Weak.IsValid() || Commit!=ETextCommit::OnEnter) return;
        if(auto* Pawn=Cast<AHOORunnerPawn>(Weak->GetOwningPawn())) if(Pawn->SetPlayerNickname(Text.ToString())) Weak->CloseNickname();
    })
    .OnKeyDownHandler_Lambda([Weak](const FGeometry&,const FKeyEvent& Event)
    {
        if(Event.GetKey()==EKeys::Escape && Weak.IsValid()) {Weak->CloseNickname();return FReply::Handled();}
        return FReply::Unhandled();
    })]];
    GEngine->GameViewport->AddViewportWidgetContent(NicknameOverlay.ToSharedRef(),100);
    FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(Input);PlayerOwner->SetInputMode(Mode);
    FSlateApplication::Get().SetKeyboardFocus(Input,EFocusCause::SetDirectly);
}
bool AHOORunnerHUD::IsMenuOpen() const
{
    const auto* P=Cast<AHOORunnerPawn>(GetOwningPawn());
    return bSettingsOpen || bRecordsOpen || (P && P->GetRun().Phase!=EHOORunnerPhase::Running);
}
void AHOORunnerHUD::ToggleSettings()
{
    auto* P=Cast<AHOORunnerPawn>(GetOwningPawn());
    if(!P || bRecordsOpen) return;
    if(!bSettingsOpen)
    {
        bResumeAfterSettings=P->GetRun().Phase==EHOORunnerPhase::Running;
        if(bResumeAfterSettings) P->PauseRun();
        bSettingsOpen=true;
    }
    else
    {
        bSettingsOpen=false;
        if(bResumeAfterSettings && P->GetRun().Phase==EHOORunnerPhase::Paused) P->PauseRun();
        bResumeAfterSettings=false;
    }
    FocusedButton=0;Buttons.Reset();
}
void AHOORunnerHUD::Back()
{
    if(bEditingNickname) CloseNickname();
    else if(bRecordsOpen) ToggleRecords();
    else if(bSettingsOpen) ToggleSettings();
    else if(auto* P=Cast<AHOORunnerPawn>(GetOwningPawn())) { P->PauseRun();FocusedButton=0;Buttons.Reset(); }
}
void AHOORunnerHUD::Navigate(int32 Direction)
{
    if(MenuWidget && MenuWidget->IsVisible()) {MenuWidget->Navigate(Direction);return;}
    for(int32 I=0;I<Buttons.Num();++I)
    {FocusedButton=(FocusedButton+Direction+Buttons.Num())%Buttons.Num();if(Buttons[FocusedButton].bEnabled)break;}
}
void AHOORunnerHUD::ConfirmSelection()
{
    if(MenuWidget && MenuWidget->IsVisible()) {MenuWidget->Confirm();return;}
    if(Buttons.IsValidIndex(FocusedButton)) {if(Buttons[FocusedButton].bEnabled)Execute(Buttons[FocusedButton].Action);}
    else if(!bSettingsOpen && !bRecordsOpen && !bEditingNickname) Execute(ERunnerMenuAction::Primary);
}
void AHOORunnerHUD::ActivateMenu(float X,float Y)
{
    if(MenuWidget && MenuWidget->IsVisible())return;
    for(const auto& Button:Buttons)
        if(Button.bEnabled && Button.Bounds.IsInside(FVector2D(X,Y))) { Execute(Button.Action);return; }
}
void AHOORunnerHUD::Execute(ERunnerMenuAction Action)
{
    auto* P=Cast<AHOORunnerPawn>(GetOwningPawn());
    if(!P) return;
    switch(Action)
    {
    case ERunnerMenuAction::Primary:P->StartRun();break;
    case ERunnerMenuAction::NewCourse:P->RestartRun();break;
    case ERunnerMenuAction::Home:P->ReturnToMenu();break;
    case ERunnerMenuAction::Settings:ToggleSettings();return;
    case ERunnerMenuAction::Close:if(bRecordsOpen) ToggleRecords();else ToggleSettings();return;
    case ERunnerMenuAction::Quit:P->QuitGame();return;
    case ERunnerMenuAction::Volume:P->CycleVolume();return;
    case ERunnerMenuAction::Quality:P->CycleQuality();return;
    case ERunnerMenuAction::Motion:P->ToggleReducedMotion();return;
    case ERunnerMenuAction::Records:ToggleRecords();return;
    case ERunnerMenuAction::Online:bOnlineTab=true;P->RefreshOnlineRanking();return;
    case ERunnerMenuAction::Local:bOnlineTab=false;return;
    case ERunnerMenuAction::Refresh:P->RefreshOnlineRanking();return;
    case ERunnerMenuAction::Submit:P->SubmitBestRun();return;
    case ERunnerMenuAction::Nickname:EditNickname();return;
    }
    FocusedButton=0;Buttons.Reset();
}

void AHOORunnerHUD::DrawHUD()
{
    CSV_SCOPED_TIMING_STAT(RunnerUI,Canvas);
    Super::DrawHUD();if(!Canvas || !TextFont) return;
    auto* P=Cast<AHOORunnerPawn>(GetOwningPawn());if(!P) return;
    const auto& R=P->GetRun();
    if(LastPhase!=static_cast<int32>(R.Phase)) {FocusedButton=0;LastPhase=static_cast<int32>(R.Phase);}
    const float S=FMath::Min(Canvas->SizeX/1600.f,Canvas->SizeY/900.f),W=Canvas->SizeX/S,H=Canvas->SizeY/S;
    const FLinearColor Ink=HOOAero::Text,Muted=HOOAero::Muted,White=HOOAero::Panel,
        Blue=HOOAero::Cyan,Mint=HOOAero::Gold,Pink=HOOAero::Coral,Gold=HOOAero::Coral,Pale=HOOAero::Surface;
    const bool Menu=IsMenuOpen();
    if(!StatusWidget && PlayerOwner) {StatusWidget=CreateWidget<UHOORunnerStatusWidget>(PlayerOwner);if(StatusWidget) StatusWidget->AddToViewport(1);}
    if(StatusWidget) StatusWidget->SetVisibility(Menu?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    if(PlayerOwner) PlayerOwner->bShowMouseCursor=Menu;
    if(PlayerOwner && bLastMenuInput!=Menu)
    {
        bLastMenuInput=Menu;
        if(Menu){FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PlayerOwner->SetInputMode(Mode);}
        else{FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);PlayerOwner->SetInputMode(Mode);}
    }
    if(!MenuWidget && PlayerOwner){MenuWidget=CreateWidget<UHOORunnerMenuWidget>(PlayerOwner);MenuWidget->AddToViewport(2);}
    const bool Shell=Menu && !bSettingsOpen && !bRecordsOpen && !bEditingNickname;
    if(MenuWidget){MenuWidget->SetVisibility(Shell?ESlateVisibility::Visible:ESlateVisibility::Collapsed);if(Shell){Buttons.Reset();MenuWidget->Present(P,this);return;}}

    float MX=-100,MY=-100;if(PlayerOwner) PlayerOwner->GetMousePosition(MX,MY);
    Buttons.Reset();
    auto Box=[&](float X,float Y,float BW,float BH,FLinearColor C){DrawRect(C,X*S,Y*S,BW*S,BH*S);};
    auto Round=[&](float X,float Y,float BW,float BH,FLinearColor C,float Radius=10.f)
    {
        const float D=FMath::Min3(Radius,BW*.5f,BH*.5f);
        TArray<FVector2D,TInlineAllocator<36>> Points;
        const FVector2D Centers[4]={{X+D,Y+D},{X+BW-D,Y+D},{X+BW-D,Y+BH-D},{X+D,Y+BH-D}};
        for(int32 Corner=0;Corner<4;++Corner) for(int32 J=0;J<=8;++J)
        {
            const float A=PI+Corner*PI*.5f+J*PI/16.f;
            Points.Add((Centers[Corner]+FVector2D(FMath::Cos(A)*D,FMath::Sin(A)*D))*S);
        }
        for(int32 I=0;I<Points.Num();++I)
        {
            FCanvasTriangleItem Tri(FVector2D((X+BW*.5f)*S,(Y+BH*.5f)*S),Points[I],Points[(I+1)%Points.Num()],GWhiteTexture);
            Tri.SetColor(C);Canvas->DrawItem(Tri);
        }
    };
    auto Card=[&](float X,float Y,float BW,float BH,FLinearColor C)
    {Round(X,Y+5,BW,BH,FLinearColor(.015f,.07f,.14f,.18f));Round(X,Y,BW,BH,C);};
    auto Text=[&](const FString& T,float X,float Y,float Size,FLinearColor C)
    {
        FSlateFontInfo Font(TextFont,FMath::Max(8,FMath::RoundToInt(Size*S*.75f)),TEXT("Regular"));
        FCanvasTextItem Item(FVector2D(X*S,Y*S),FText::FromString(T),Font,C);Canvas->DrawItem(Item);
    };
    auto Line=[&](float X,float Y,float EX,float EY,FLinearColor C,float Width=2.f){DrawLine(X*S,Y*S,EX*S,EY*S,C,Width*S);};
    auto Diamond=[&](float X,float Y,float D,FLinearColor C)
    {
        FCanvasTriangleItem A(FVector2D(X*S,(Y-D)*S),FVector2D((X+D)*S,Y*S),FVector2D(X*S,(Y+D)*S),GWhiteTexture);A.SetColor(C);Canvas->DrawItem(A);
        FCanvasTriangleItem B(FVector2D(X*S,(Y-D)*S),FVector2D(X*S,(Y+D)*S),FVector2D((X-D)*S,Y*S),GWhiteTexture);B.SetColor(C);Canvas->DrawItem(B);
    };
    auto Button=[&](ERunnerMenuAction Action,const FString& Label,float X,float Y,float BW,float BH,bool Primary=false,bool Enabled=true)
    {
        const FBox2D Bounds(FVector2D(X*S,Y*S),FVector2D((X+BW)*S,(Y+BH)*S));
        const bool Selected=Enabled && (Buttons.Num()==FocusedButton || Bounds.IsInside(FVector2D(MX,MY)));
        Round(X,Y+4,BW,BH,FLinearColor(0,0,0,.18f),10);
        Round(X,Y,BW,BH,!Enabled?HOOAero::Surface:Primary?(Selected?FLinearColor(.03f,.42f,.49f):FLinearColor(.02f,.30f,.37f)):(Selected?FLinearColor(.05f,.19f,.24f):Pale),10);
        if(Selected && !Primary) Round(X+7,Y+BH*.3f,4,BH*.4f,Blue,2);
        Text(Label,X+22,Y+(BH-24)/2,Primary?24:19,Enabled?Ink:Muted);
        Buttons.Add({Bounds,Action,Enabled});
    };

    if(Menu)
    {
    Card(24,24,294,96,White);Diamond(53,66,13,Mint);
    Text(FString::Printf(TEXT("%s"),*FText::AsNumber(R.Score()).ToString()),78,38,34,Ink);
    Text(FString::Printf(TEXT("점수   ·   크리스탈 %d개"),R.Coins),78,85,15,Muted);
    Card(W/2-172,24,344,96,White);
    Text(FString::Printf(TEXT("%s m"),*FText::AsNumber(FMath::FloorToInt(R.Distance/100)).ToString()),W/2-147,35,34,Ink);
    Text(FString::Printf(TEXT("%s  ·  %d단계"),P->GetDistrict(),HOORunner::Difficulty(R.Distance)),W/2-147,84,15,Blue);
    Card(W-242,24,218,96,White);
    Text(FString::Printf(TEXT("%d"),FMath::RoundToInt(R.CurrentSpeed*.036)),W-216,35,39,Blue);
    Text(TEXT("km/h"),W-112,57,18,Muted);
    Text(R.IsBoosting()?TEXT("부스터 가속 중!"):R.IsFever()?TEXT("피버 가속 중!"):TEXT("바람을 가르며 달려요"),W-216,87,14,R.IsBoosting()?Gold:Blue);

    }
    if(bEditingNickname)
    {
        Box(0,0,W,H,FLinearColor(.025f,.07f,.14f,.65f));
        Card(W/2-285,H/2-170,570,340,White);
        Text(TEXT("어떤 이름으로 달릴까요?"),W/2-247,H/2-133,30,Ink);
        Text(TEXT("한글·영문·숫자 2~12자"),W/2-245,H/2-83,18,Muted);
        Text(P->GetRankStatus(),W/2-245,H/2+60,16,Blue);
        Text(TEXT("Enter로 저장  ·  Esc로 돌아가기"),W/2-245,H/2+115,17,Muted);
        return;
    }
    if(bRecordsOpen)
    {
        Box(0,0,W,H,FLinearColor(.025f,.07f,.14f,.48f));
        const float X=W/2-525,Y=H/2-367;
        Card(X,Y,1050,734,White);
        Text(TEXT("우리의 달리기 기록"),X+34,Y+29,37,Ink);
        Text(TEXT("더 멀리, 더 신나게!"),X+37,Y+80,18,Muted);
        Button(ERunnerMenuAction::Nickname,P->GetNickname()+TEXT("  /  이름 바꾸기"),X+690,Y+36,326,56);
        Button(ERunnerMenuAction::Local,TEXT("내 최고기록"),X+34,Y+123,220,53,!bOnlineTab);
        Button(ERunnerMenuAction::Online,TEXT("온라인 랭킹"),X+269,Y+123,220,53,bOnlineTab);
        Text(bOnlineTab?TEXT("시즌 2 · 플레이어별 최고 점수"):TEXT("이 PC에서 달린 상위 10개 기록"),X+526,Y+141,18,Muted);
        Round(X+34,Y+196,982,37,Pale,10);
        Text(TEXT("순위"),X+55,Y+203,16,Muted);Text(TEXT("러너"),X+155,Y+203,16,Muted);
        Text(TEXT("점수"),X+643,Y+203,16,Muted);Text(TEXT("거리"),X+857,Y+203,16,Muted);
        const int32 Count=bOnlineTab?P->GetOnlineRanks().Num():P->GetLocalScores().Num();
        if(Count==0)
        {
            Text(bOnlineTab?TEXT("첫 번째 주인공이 되어 보세요!"):TEXT("첫 기록을 남겨 볼까요?"),X+275,Y+330,28,Blue);
            Text(bOnlineTab?P->GetRankStatus():TEXT("달리기를 마치면 기록이 여기에 쌓여요."),X+215,Y+381,18,Muted);
        }
        for(int32 I=0;I<FMath::Min(10,Count);++I)
        {
            FString Name;int32 Score,Rank;float Distance;
            if(bOnlineTab) {const auto& Entry=P->GetOnlineRanks()[I];Name=Entry.Nickname;Score=Entry.Score;Distance=Entry.Distance;Rank=Entry.Rank;}
            else {const auto& Entry=P->GetLocalScores()[I];Name=Entry.Nickname;Score=Entry.Score;Distance=Entry.Distance;Rank=I+1;}
            const float RowY=Y+244+I*35;
            if(I%2==0) Round(X+34,RowY-2,982,33,HOOAero::Surface,8);
            if(I<3) Round(X+50,RowY,34,28,I==0?Gold:I==1?FLinearColor(.69f,.78f,.88f):FLinearColor(.91f,.63f,.4f),12);
            Text(FString::FromInt(Rank),X+60,RowY+2,18,Ink);
            Text(Name,X+155,RowY+2,18,Ink);
            Text(FText::AsNumber(Score).ToString(),X+643,RowY+2,19,Blue);
            Text(FString::Printf(TEXT("%.0f m"),Distance),X+857,RowY+2,18,Muted);
        }
        Text(bOnlineTab?P->GetRankStatus():TEXT("기록은 자동으로 저장돼요."),X+36,Y+609,17,Blue);
        Button(ERunnerMenuAction::Submit,P->IsOnlineBusy()?TEXT("기록 확인 중..."):TEXT("등록 가능한 최고기록 올리기"),X+34,Y+645,436,52,true,!P->IsOnlineBusy() && P->HasSubmittableRun());
        Button(ERunnerMenuAction::Refresh,TEXT("새로고침"),X+486,Y+645,222,52,false,!P->IsOnlineBusy());
        Button(ERunnerMenuAction::Close,TEXT("돌아가기"),X+724,Y+645,292,52);
        Text(TEXT("등록 시 닉네임·최고기록 공개 · 온라인 기록은 20분 이내 주행만 지원해요."),X+36,Y+707,13,Muted);
        return;
    }
    if(bSettingsOpen)
    {
        Box(0,0,W,H,FLinearColor(.025f,.07f,.14f,.48f));
        const float X=W/2-300,Y=H/2-300;
        Card(X,Y,600,600,White);
        Text(TEXT("나에게 딱 맞게"),X+34,Y+30,38,Ink);
        Text(TEXT("항목을 누르면 바뀌고 자동으로 저장돼요."),X+36,Y+87,18,Muted);
        Button(ERunnerMenuAction::Volume,FString::Printf(TEXT("소리 크기                            %d%%"),P->VolumePercent),X+34,Y+151,532,65);
        const TCHAR* Quality[]={TEXT("가볍게"),TEXT("균형 있게"),TEXT("선명하게")};
        Button(ERunnerMenuAction::Quality,FString::Printf(TEXT("그래픽 품질                     %s"),Quality[P->QualityLevel]),X+34,Y+233,532,65);
        Button(ERunnerMenuAction::Motion,FString::Printf(TEXT("움직임 효과 줄이기               %s"),P->bReducedMotion?TEXT("켜짐"):TEXT("꺼짐")),X+34,Y+315,532,65);
        Text(TEXT("움직임 효과를 줄이면 화면 회전과 속도 연출이 편안해져요."),X+36,Y+408,16,Muted);
        Button(ERunnerMenuAction::Close,TEXT("좋아요, 돌아가기"),X+34,Y+472,532,65,true);
        Text(TEXT("방향키로 선택  ·  Enter로 변경  ·  Esc로 닫기"),X+36,Y+561,15,Muted);
        return;
    }

    if(!Menu)
    {
        const int64 Start=FMath::Max<int64>(0,FMath::FloorToInt64(R.Distance/600)-1);
        if(!R.IsFlying())
            for(int64 I=Start;I<Start+35;++I)
            {
                const auto Tile=HOORunner::Tile(I,R.Seed);
                const double Ahead=(I+.5)*600-R.Distance;
                if(Ahead<-275 || I<=R.ProtectedThroughTile) continue;
                const bool Any=Tile.Lanes[0]!=EHOORunnerHazard::None || Tile.Lanes[1]!=EHOORunnerHazard::None || Tile.Lanes[2]!=EHOORunnerHazard::None;
                if(!Any && !Tile.bLaunchPad) continue;
                Card(W-350,H-132,326,94,White);
                Text(Tile.bLaunchPad?TEXT("발사 타일! 밟으면 멀리 날아가요"):FString::Printf(TEXT("앞으로 %d m · 다음 길"),FMath::Max(0,FMath::CeilToInt(Ahead/100))),W-333,H-119,14,Muted);
                for(int32 L=0;L<3;++L)
                {
                    const auto Hazard=Tile.Lanes[L];
                    const auto C=Tile.bLaunchPad?Mint:Hazard==EHOORunnerHazard::None?Blue:Hazard==EHOORunnerHazard::Overhead?Pink:Gold;
                    const TCHAR* Label=Tile.bLaunchPad?TEXT("발사"):Hazard==EHOORunnerHazard::None?TEXT("안전"):Hazard==EHOORunnerHazard::Overhead?TEXT("숙여요"):TEXT("점프");
                    Round(W-333+L*99,H-86,92,30,C,12);Text(Label,W-311+L*99,H-81,16,FLinearColor::White);
                    if(L==R.TargetLane+1) Round(W-309+L*99,H-46,42,3,C,2);
                }
                break;
            }
        if(P->GetCountdown()>0)
        {
            Card(W/2-90,H*.34f,180,150,White);
            Text(FString::FromInt(FMath::CeilToInt(P->GetCountdown())),W/2-25,H*.34f+8,72,Blue);
            Text(R.Distance>0?TEXT("다시 출발해요!"):TEXT("준비됐나요?"),W/2-67,H*.34f+108,21,Ink);
        }
        if(R.IsSliding()) {Round(W/2-70,H-220,140,43,Pink,17);Text(TEXT("슬라이드!"),W/2-52,H-210,21,FLinearColor::White);}
        Text(TEXT("A / D  이동     Space  점프     S  슬라이드     Esc  쉬기     F1  설정     F2  기록"),W/2-275,H-24,11,HOOAero::Text);
        return;
    }
    Box(0,133,W,H-133,FLinearColor(.55f,.78f,.95f,.15f));
    const bool Ready=R.Phase==EHOORunnerPhase::Ready,Paused=R.Phase==EHOORunnerPhase::Paused;
    const float X=30,Y=FMath::Max(148.f,H/2-296);
    Card(X,Y,556,632,White);
    Round(X+30,Y+24,180,30,Pale,15);
    Text(Ready?TEXT("오늘도 신나게 달려요"):Paused?TEXT("잠깐 쉬어 가도 좋아요"):P->IsNewRecord()?TEXT("새로운 최고기록!"):TEXT("멋진 도전이었어요!"),X+44,Y+31,15,Blue);
    Text(Ready?TEXT("스카이라인"):Paused?TEXT("숨 고르기"):TEXT("달리기 완료!"),X+28,Y+76,48,Ink);
    if(Ready) Text(TEXT("러시"),X+28,Y+130,56,Blue);
    else Text(Paused?TEXT("준비되면 다시 출발해요."):TEXT("다음엔 더 멀리 갈 수 있어요."),X+30,Y+140,22,Muted);
    if(Ready)
    {
        Text(TEXT("도심부터 바닷속, 구름 위까지!"),X+32,Y+214,24,Ink);
        Text(TEXT("목숨 3개로 시작! 피버를 모아 최고기록에 도전해요."),X+32,Y+254,18,Muted);
        Text(FString::Printf(TEXT("내 최고기록  %.0f m  ·  %s점"),P->BestDistance,*FText::AsNumber(P->BestScore).ToString()),X+32,Y+297,20,Blue);
    }
    else
    {
        Text(FString::Printf(TEXT("%.0f m   ·   %s점"),R.Distance/100,*FText::AsNumber(R.Score()).ToString()),X+32,Y+207,32,Blue);
        Text(FString::Printf(TEXT("크리스탈 %d개   피버 %d회   비행 %d회"),R.Coins,R.FeverActivations,R.Launches),X+32,Y+256,18,Muted);
        Text(Paused?TEXT("카운트다운 후 이어서 달려요."):R.Failure==EHOORunnerHazard::Overhead?TEXT("보라색 문 아래에서는 S로 슬라이드해요."):TEXT("장애물과 빈 바닥은 점프하거나 피해서 지나가요."),X+32,Y+297,17,Muted);
    }
    Button(ERunnerMenuAction::Primary,Ready?TEXT("달리기 시작  /  Enter"):Paused?TEXT("이어서 달리기  /  Enter"):TEXT("한 번 더 달리기  /  Enter"),X+30,Y+347,496,64,true);
    Button(ERunnerMenuAction::Records,TEXT("기록과 랭킹"),X+30,Y+429,240,54);
    Button(ERunnerMenuAction::Settings,TEXT("게임 설정"),X+286,Y+429,240,54);
    Button(Ready?ERunnerMenuAction::NewCourse:ERunnerMenuAction::Home,Ready?TEXT("새 코스로 출발"):TEXT("처음 화면"),X+30,Y+501,240,54);
    Button(ERunnerMenuAction::Quit,TEXT("게임 종료"),X+286,Y+501,240,54);
    Text(TEXT("A / D 이동  ·  Space 점프  ·  S 슬라이드"),X+32,Y+583,17,Muted);
    if(Ready && HeroPortrait)
    {
        const float PX=W-358,PY=Y+120;
        Card(PX,PY,306,374,White);
        const float IW=250,IH=270;
        DrawTexture(HeroPortrait,(PX+28)*S,(PY+18)*S,IW*S,IH*S,0,0,1,1,FLinearColor::White,BLEND_Translucent);
        Text(TEXT("같이 달려 볼까요?"),PX+35,PY+310,25,Blue);
    }
}
