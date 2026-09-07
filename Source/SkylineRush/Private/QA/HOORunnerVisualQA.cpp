#include "QA/HOORunnerVisualQA.h"
#include "Gameplay/HOORunnerPawn.h"
#include "Gameplay/HOORunnerGameMode.h"
#include "UI/HOORunnerStatusWidget.h"
#include "UI/HOORunnerMenuWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderTimer.h"
#include "RHI.h"
#include "RHIStats.h"
#include "DynamicRHI.h"
#include "UObject/UObjectIterator.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
CSV_DEFINE_CATEGORY(RunnerUI,true);
AHOORunnerVisualQA::AHOORunnerVisualQA(){PrimaryActorTick.bCanEverTick=true;}
#if !UE_BUILD_SHIPPING
void AHOORunnerVisualQA::Initialize(AHOORunnerPawn* InRunner)
{
 Runner=InRunner;Tag=TEXT("Before");FParse::Value(FCommandLine::Get(),TEXT("RunnerQAOutput="),Tag);
 Tag=FPaths::MakeValidFileName(Tag);Out=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("QA")/Tag);
 IFileManager::Get().MakeDirectory(*Out,true);
 bVideo=FParse::Param(FCommandLine::Get(),TEXT("RunnerQAVideo"));
 StartTime=PreviousTime=FPlatformTime::Seconds();FApp::SetUseFixedTimeStep(true);FApp::SetFixedDeltaTime(1./60.);
 Runner->QualityLevel=2;Runner->bReducedMotion=false;Runner->ApplyPreferences();Runner->VolumePercent=0;
 Runner->Run.Reset(409);Runner->bVisualQAFrozen=true;Runner->BestScore=0;Runner->BestDistance=0;Runner->StartingBest=0;Runner->LocalScores.Reset();
 Runner->UpdateCourse(true);
 if(GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWindow())GEngine->GameViewport->GetWindow()->SetTitle(FText::FromString(TEXT("SKYLINE RUSH - Visual QA ")+Tag));
 for(const auto* C:{TEXT("r.VSync 0"),TEXT("r.ScreenPercentage 100"),TEXT("r.GPUStatsEnabled 1"),TEXT("r.GPUCsvStatsEnabled 1"),TEXT("csv.CompressionMode 0"),TEXT("t.IdleWhenNotForeground 0"),TEXT("Slate.bAllowThrottling 0"),TEXT("DisableAllScreenMessages")})UGameplayStatics::GetPlayerController(this,0)->ConsoleCommand(C,false);
 UGameplayStatics::GetPlayerController(this,0)->ConsoleCommand(bVideo?TEXT("t.MaxFPS 60"):TEXT("t.MaxFPS 0"),false);
 if(GEngine->GameViewport->GetWindow().IsValid())GEngine->GameViewport->GetWindow()->BringToFront(true);
 UE_LOG(LogTemp,Display,TEXT("VISUAL_QA_READY %s"),*Out);
}
void AHOORunnerVisualQA::Next(int32 Value){Stage=Value;StageFrames=0;}
void AHOORunnerVisualQA::CheckReview(const TCHAR* Name,bool bPassed)
{
 auto Entry=MakeShared<FJsonObject>();Entry->SetStringField(TEXT("check"),Name);Entry->SetBoolField(TEXT("passed"),bPassed);
 ReviewChecks.Add(MakeShared<FJsonValueObject>(Entry));
 if(!bPassed)UE_LOG(LogTemp,Error,TEXT("RUNNER_REVIEW_CHECK_FAILED %s"),Name);
}
void AHOORunnerVisualQA::Shot(const TCHAR* Name)
{
 FScreenshotRequest::RequestScreenshot(Out/(FString(Name)+TEXT(".png")),true,false);
 SkipSamples=5;
}
void AHOORunnerVisualQA::Drive()
{
 const auto& R=Runner->GetRun();if(R.IsFlying() || Runner->Countdown>0)return;
 const int64 Current=FMath::FloorToInt64(R.Distance/HOORunner::TileLength);
 for(int64 I=Current;I<=Current+14;++I)
 {
  const auto T=HOORunner::Tile(I,R.Seed);const double Ahead=(I+.5)*HOORunner::TileLength-R.Distance;
  if(Ahead < -280)continue;
  if(T.bBooster && Ahead>0 && Ahead<3500)
  {if(R.TargetLane<0)Runner->MoveRight();else if(R.TargetLane>0)Runner->MoveLeft();break;}
  bool Any=false,AllGap=true;for(auto H:T.Lanes){Any|=H!=EHOORunnerHazard::None;AllGap&=H==EHOORunnerHazard::Gap;}
  if(!Any)continue;
  if(AllGap){if(Ahead/FMath::Max(1.f,R.CurrentSpeed)<.31 && !T.bFlightGap)Runner->JumpAction();break;}
  if(R.TargetLane<T.SafeLane)Runner->MoveRight();else if(R.TargetLane>T.SafeLane)Runner->MoveLeft();break;
 }
}
void AHOORunnerVisualQA::Report()
{
 if(bFinished)return;bFinished=true;
 auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("tag"),Tag);J->SetStringField(TEXT("status"),TEXT("passed"));
 J->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());J->SetStringField(TEXT("gpu"),GRHIAdapterName);
 J->SetStringField(TEXT("cpu"),FPlatformMisc::GetCPUBrand());J->SetStringField(TEXT("map"),TEXT("/Game/SkylineRush/Maps/L_SkylineRush"));
 J->SetNumberField(TEXT("width"),GEngine->GameViewport->Viewport->GetSizeXY().X);J->SetNumberField(TEXT("height"),GEngine->GameViewport->Viewport->GetSizeXY().Y);
 J->SetNumberField(TEXT("samples"),Samples.Num());J->SetNumberField(TEXT("foreground_samples"),ForegroundSamples);
 const auto Mode=GEngine->GameViewport->GetWindow()->GetWindowMode();J->SetStringField(TEXT("window_mode"),Mode==EWindowMode::Fullscreen?TEXT("fullscreen"):Mode==EWindowMode::WindowedFullscreen?TEXT("borderless"):TEXT("windowed"));J->SetStringField(TEXT("method"),TEXT("Standalone Win64 DX12 High, 100% internal resolution, uncapped real wall frames, fixed 60 Hz simulation; screenshot frames excluded. Video pass is separate."));
 FVector4 Sum(0,0,0,0);double DrawSum=0;TArray<double> Wall;
 FString Raw=TEXT("frame,wall_ms,game_ms,render_ms,gpu_ms,draw_calls\n");
 for(int I=0;I<Samples.Num();++I){Sum+=Samples[I];DrawSum+=Draws[I];Wall.Add(Samples[I].X);Raw+=FString::Printf(TEXT("%d,%.5f,%.5f,%.5f,%.5f,%d\n"),I,Samples[I].X,Samples[I].Y,Samples[I].Z,Samples[I].W,Draws[I]);}
 if(Samples.Num())
 {
  Sum/=Samples.Num();Wall.Sort();
  J->SetNumberField(TEXT("fps"),1000./Sum.X);J->SetNumberField(TEXT("wall_ms"),Sum.X);J->SetNumberField(TEXT("game_ms"),Sum.Y);J->SetNumberField(TEXT("render_ms"),Sum.Z);J->SetNumberField(TEXT("gpu_ms"),Sum.W);J->SetNumberField(TEXT("draw_calls"),DrawSum/Samples.Num());
  J->SetNumberField(TEXT("p95_wall_ms"),Wall[FMath::Clamp(FMath::FloorToInt(Wall.Num()*.95),0,Wall.Num()-1)]);
 }
 FTextureMemoryStats Mem;RHIGetTextureMemoryStats(Mem);
 J->SetNumberField(TEXT("streaming_texture_mb"),Mem.StreamingMemorySize/(1024.*1024));J->SetNumberField(TEXT("nonstreaming_texture_mb"),Mem.NonStreamingMemorySize/(1024.*1024));J->SetNumberField(TEXT("dedicated_gpu_mb"),Mem.DedicatedVideoMemory/(1024.*1024));
 int32 Widgets=0,Nodes=0,Instances=0,ISMs=0;
 for(TObjectIterator<UUserWidget> It;It;++It)if(It->GetWorld()==GetWorld() && It->IsInViewport()){++Widgets;TArray<UWidget*> All;It->WidgetTree->GetAllWidgets(All);Nodes+=All.Num();}
 for(TObjectIterator<UInstancedStaticMeshComponent> It;It;++It)if(It->GetWorld()==GetWorld()){++ISMs;Instances+=It->GetInstanceCount();}
 J->SetNumberField(TEXT("user_widgets"),Widgets);J->SetNumberField(TEXT("widget_nodes"),Nodes);J->SetNumberField(TEXT("ism_components"),ISMs);J->SetNumberField(TEXT("instances"),Instances);
 J->SetStringField(TEXT("run_snapshot"),Runner->GetRunSnapshot());
 FString Json;TSharedRef<TJsonWriter<>> Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(J,Writer);
 FFileHelper::SaveStringToFile(Json,*(Out/TEXT("metrics.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 FFileHelper::SaveStringToFile(Raw,*(Out/TEXT("frames.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 UE_LOG(LogTemp,Display,TEXT("VISUAL_QA_COMPLETE %s"),*Json);
}
void AHOORunnerVisualQA::Tick(float Dt)
{
 Super::Tick(Dt);if(!Runner)return;
 const double Now=FPlatformTime::Seconds(),Wall=(Now-PreviousTime)*1000;PreviousTime=Now;++StageFrames;
 if(Now-StartTime>(bVideo?600.:240.)){UE_LOG(LogTemp,Error,TEXT("VISUAL_QA_TIMEOUT"));UGameplayStatics::GetPlayerController(this,0)->ConsoleCommand(TEXT("quit"),false);return;}
 auto& R=Runner->Run;auto* PC=UGameplayStatics::GetPlayerController(this,0);
 if(Stage==0 && Now-StartTime>6)
 {
  Shot(TEXT("main"));Next(1);
 }
 else if(Stage==1 && StageFrames>60)
 {
  Runner->bVisualQAFrozen=false;Runner->StartRun();Runner->Countdown=0;
  if(bVideo){FFileHelper::SaveStringToFile(TEXT("ready"),*(Out/TEXT("video-ready.flag")));Next(30);}
  else Next(2);
 }
 else if(Stage==2)
 {
  Drive();
  if(R.SimulationTicks>=600){Runner->bVisualQAFrozen=true;Next(3);}
 }
 else if(Stage==3 && StageFrames==60)
 {
  Shot(TEXT("gameplay"));
  auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("tick"),R.SimulationTicks);J->SetNumberField(TEXT("distance_cm"),R.Distance);J->SetStringField(TEXT("camera_location"),Runner->ChaseCamera->GetComponentLocation().ToString());J->SetStringField(TEXT("camera_rotation"),Runner->ChaseCamera->GetComponentRotation().ToString());J->SetNumberField(TEXT("fov"),Runner->ChaseCamera->FieldOfView);J->SetNumberField(TEXT("animation_position"),Runner->RunnerMesh->GetPosition());
  FString Text;auto W=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(J,W);FFileHelper::SaveStringToFile(Text,*(Out/TEXT("anchor.json")));
 }
 else if(Stage==3 && StageFrames>120)
 {
  Shot(TEXT("hud"));
  if(FParse::Param(FCommandLine::Get(),TEXT("RunnerQADebugViews"))){PC->ConsoleCommand(TEXT("viewmode shadercomplexity"),false);Next(40);}
  else{R.TogglePause();Next(4);}
 }
 else if(Stage==4 && StageFrames==60)Shot(TEXT("pause"));
 else if(Stage==4 && StageFrames>120 && FParse::Param(FCommandLine::Get(),TEXT("RunnerQACaptureOnly"))){PC->ConsoleCommand(TEXT("quit"),false);}
 else if(Stage==4 && StageFrames>120)
 {
  R.TogglePause();Runner->bVisualQAFrozen=false;if(GEngine->GameViewport->GetWindow().IsValid())GEngine->GameViewport->GetWindow()->BringToFront(true);if(FSlateApplication::IsInitialized())FSlateApplication::Get().SetAllUserFocusToGameViewport();FCsvProfiler::Get()->BeginCapture(-1,Out,TEXT("profile.csv"));bProfile=true;Next(5);
 }
 else if(Stage==5)
 {
  Drive();
  if(SkipSamples>0)--SkipSamples;else{ForegroundSamples+=FSlateApplication::Get().IsActive()?1:0;Samples.Add(FVector4(Wall,FPlatformTime::ToMilliseconds(GGameThreadTime),FPlatformTime::ToMilliseconds(GRenderThreadTime),FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0))));Draws.Add(GNumDrawCallsRHI[0]);}
  const int Dist=FMath::FloorToInt(R.Distance/100);
  for(int Target:{250,440,750,1030,1450,1900,2180,2480})if(Dist>=Target && !ShotStages.Contains(Target)){ShotStages.Add(Target);Shot(*FString::Printf(TEXT("route_%04d"),Target));}
  if(R.Elapsed>=(FParse::Param(FCommandLine::Get(),TEXT("RunnerQAFast"))?25:60) || R.Phase==EHOORunnerPhase::Crashed){FCsvProfiler::Get()->EndCapture();bProfile=false;Runner->bVisualQAFrozen=true;PC->ConsoleCommand(TEXT("stat unit"),false);PC->ConsoleCommand(TEXT("stat RHI"),false);Next(6);}
 }
 else if(Stage==6 && StageFrames==120)Shot(TEXT("profile"));
 else if(Stage==6 && StageFrames>180){PC->ConsoleCommand(TEXT("stat unit"),false);PC->ConsoleCommand(TEXT("stat RHI"),false);Runner->bVisualQAFrozen=false;Next(7);}
 else if(Stage==7)
 {
  if(R.Distance>=248000 && !ShotStages.Contains(2480)){ShotStages.Add(2480);Shot(TEXT("route_2480"));}
  if(R.Phase==EHOORunnerPhase::Crashed){Runner->bVisualQAFrozen=true;Next(8);}
 }
 else if(Stage==8 && StageFrames==60)Shot(TEXT("result"));
 else if(Stage==8 && StageFrames>180)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());H->Navigate(1);H->ConfirmSelection();Next(50);
 }
 else if(Stage==50 && StageFrames==60)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());const bool OK=H->IsRecordsOpen();Shot(TEXT("records"));FFileHelper::SaveStringToFile(OK?TEXT("records: passed\n"):TEXT("records: FAILED\n"),*(Out/TEXT("ui-flow.txt")));
 }
 else if(Stage==50 && StageFrames==90)Cast<AHOORunnerHUD>(PC->GetHUD())->Back();
 else if(Stage==50 && StageFrames>120)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());H->Navigate(1);H->ConfirmSelection();Next(51);
 }
 else if(Stage==51 && StageFrames==60)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());const bool OK=H->IsSettingsOpen();Shot(TEXT("settings"));FFileHelper::SaveStringToFile(OK?TEXT("settings: passed\n"):TEXT("settings: FAILED\n"),*(Out/TEXT("ui-flow.txt")),FFileHelper::EEncodingOptions::AutoDetect,&IFileManager::Get(),FILEWRITE_Append);
 }
 else if(Stage==51 && StageFrames==90)Cast<AHOORunnerHUD>(PC->GetHUD())->Back();
 else if(Stage==51 && StageFrames>120){Report();Next(FParse::Param(FCommandLine::Get(),TEXT("RunnerReviewQA"))?100:9);}
 else if(Stage==100 && StageFrames>10)
 {
  const auto DistanceBest=Runner->StartingBest;const auto ScoreBest=Runner->StartingBestScore;
  Runner->StartingBest=R.Distance/100.+10000;Runner->StartingBestScore=R.Score()-1;
  CheckReview(TEXT("Higher score with shorter distance earns record badge"),Runner->IsNewRecord());
  Runner->StartingBest=DistanceBest;Runner->StartingBestScore=ScoreBest;
  Runner->StartRun();Runner->Countdown=0;Runner->bVisualQAFrozen=false;
  auto* C=Cast<AHOORunnerController>(PC);C->DispatchRunnerInput(TEXT("cancel"));
  CheckReview(TEXT("Gamepad cancel remains slide while running"),R.IsSliding());
  C->OnApplicationActivationChanged(false);ReviewDistance=R.Distance;
  CheckReview(TEXT("App deactivation pauses a run"),R.Phase==EHOORunnerPhase::Paused);Next(101);
 }
 else if(Stage==101 && StageFrames>10)
 {
  CheckReview(TEXT("No distance advances while inactive"),R.Distance==ReviewDistance);
  auto* C=Cast<AHOORunnerController>(PC);C->OnApplicationActivationChanged(true);
  CheckReview(TEXT("App activation never auto-resumes"),R.Phase==EHOORunnerPhase::Paused);
  C->DispatchRunnerInput(TEXT("records"));Next(102);
 }
 else if(Stage==102 && StageFrames>10)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());auto* C=Cast<AHOORunnerController>(PC);
  CheckReview(TEXT("Records open from pause"),H->IsRecordsOpen());C->DispatchRunnerInput(TEXT("cancel"));
  CheckReview(TEXT("Cancel closes records and preserves pause"),!H->IsRecordsOpen() && R.Phase==EHOORunnerPhase::Paused);
  C->DispatchRunnerInput(TEXT("settings"));Next(103);
 }
 else if(Stage==103 && StageFrames>10)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());auto* C=Cast<AHOORunnerController>(PC);
  CheckReview(TEXT("Settings open from pause"),H->IsSettingsOpen());C->DispatchRunnerInput(TEXT("cancel"));
  CheckReview(TEXT("Cancel closes settings and preserves pause"),!H->IsSettingsOpen() && R.Phase==EHOORunnerPhase::Paused);
  C->DispatchRunnerInput(TEXT("cancel"));
  CheckReview(TEXT("Explicit resume has countdown"),R.Phase==EHOORunnerPhase::Running && Runner->Countdown>=1.f);Next(104);
 }
 else if(Stage==104 && StageFrames>10)
 {
  CheckReview(TEXT("Resume countdown freezes simulation"),Runner->Countdown>0 && R.Distance==ReviewDistance);
  Runner->Countdown=0;Next(109);
 }
 else if(Stage==109 && StageFrames>20)
 {
  Cast<AHOORunnerController>(PC)->DispatchRunnerInput(TEXT("pause"));Next(110);
 }
 else if(Stage==110 && StageFrames>10)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());
  CheckReview(TEXT("Second pause displays current score"),H->MenuWidget && H->MenuWidget->Score->GetText().ToString().StartsWith(FText::AsNumber(R.Score()).ToString()+TEXT("점")));
  Cast<AHOORunnerController>(PC)->DispatchRunnerInput(TEXT("records"));Next(105);
 }
 else if(Stage==105 && StageFrames>10)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());auto* C=Cast<AHOORunnerController>(PC);
  H->Execute(ERunnerMenuAction::Nickname);const auto Id=Runner->RunId;const auto Inputs=Runner->ReplayInputs.Num();
  C->DispatchRunnerInput(TEXT("restart"));C->DispatchRunnerInput(TEXT("left"));
  CheckReview(TEXT("Nickname editor blocks gameplay dispatch"),H->IsEditingNickname() && Id==Runner->RunId && Inputs==Runner->ReplayInputs.Num());
  C->DispatchRunnerInput(TEXT("cancel"));CheckReview(TEXT("Cancel dismisses nickname editor"),!H->IsEditingNickname());
  const FString Name=Runner->PlayerNickname;
  CheckReview(TEXT("Korean nickname is accepted"),Runner->SetPlayerNickname(TEXT("러너별빛")));
  CheckReview(TEXT("Invalid nickname is rejected"),!Runner->SetPlayerNickname(TEXT("!")));Runner->SetPlayerNickname(Name);
  H->Back();Runner->ReturnToMenu();
  if(H->StatusWidget){H->StatusWidget->ToastTime=1.5f;H->StatusWidget->ToastText->SetText(FText::FromString(TEXT("OLD FEVER")));H->StatusWidget->ScorePulse=.4f;}
  Runner->StartRun();Next(106);
 }
 else if(Stage==106 && StageFrames>10)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());auto* S=H->StatusWidget.Get();
  CheckReview(TEXT("Retry clears stale power toast and score pulse"),S && S->ToastTime==0 && S->ToastText->GetText().IsEmpty() && S->ScorePulse==0);
  H->ToggleSettings();Next(107);
 }
 else if(Stage==107 && StageFrames>10)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());auto* C=Cast<AHOORunnerController>(PC);C->DispatchRunnerInput(TEXT("cancel"));
  CheckReview(TEXT("Settings opened during play returns to countdown"),!H->IsSettingsOpen() && R.Phase==EHOORunnerPhase::Running && Runner->Countdown>=1.f);
  H->ToggleRecords();Next(108);
 }
 else if(Stage==108 && StageFrames>10)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());Cast<AHOORunnerController>(PC)->DispatchRunnerInput(TEXT("cancel"));
  CheckReview(TEXT("Records opened during play returns to countdown"),!H->IsRecordsOpen() && R.Phase==EHOORunnerPhase::Running && Runner->Countdown>=1.f);
  Runner->Countdown=0;Next(111);
 }
 else if(Stage==111 && R.Phase==EHOORunnerPhase::Crashed)Next(112);
 else if(Stage==112 && StageFrames>20)
 {
  auto* H=Cast<AHOORunnerHUD>(PC->GetHUD());
  CheckReview(TEXT("Retry result displays its own score"),H->MenuWidget && H->MenuWidget->Score->GetText().ToString().StartsWith(FText::AsNumber(R.Score()).ToString()+TEXT("점")));
  CheckReview(TEXT("Unassisted second run ends after three impacts"),R.Hits==3 && R.Lives==0);
  Shot(TEXT("retry_result"));
  auto J=MakeShared<FJsonObject>();J->SetArrayField(TEXT("checks"),ReviewChecks);
  FString Json;auto Writer=TJsonWriterFactory<>::Create(&Json);FJsonSerializer::Serialize(J,Writer);
  FFileHelper::SaveStringToFile(Json,*(Out/TEXT("review-checks.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
  UE_LOG(LogTemp,Display,TEXT("RUNNER_REVIEW_CHECKS_COMPLETE"));Next(9);
 }
 else if(Stage==9 && StageFrames>240){PC->ConsoleCommand(TEXT("quit"),false);}
 else if(Stage==40 && StageFrames==120)Shot(TEXT("shader-complexity"));
 else if(Stage==40 && StageFrames>180){PC->ConsoleCommand(TEXT("viewmode quadoverdraw"),false);Next(41);}
 else if(Stage==41 && StageFrames==120)Shot(TEXT("quad-overdraw"));
 else if(Stage==41 && StageFrames>180){PC->ConsoleCommand(TEXT("viewmode lit"),false);PC->ConsoleCommand(TEXT("quit"),false);}
 else if(Stage==30){Drive();if(StageFrames%2==0 && R.Elapsed>=2 && R.Elapsed<27){const FString Frames=FPaths::ProjectSavedDir()/TEXT("QA/Video")/Tag;IFileManager::Get().MakeDirectory(*Frames,true);FScreenshotRequest::RequestScreenshot(Frames/FString::Printf(TEXT("frame_%05d.png"),static_cast<int32>(R.SimulationTicks/4)-60),true,false);}if(R.Elapsed>=28){Runner->bVisualQAFrozen=true;FFileHelper::SaveStringToFile(TEXT("done"),*(Out/TEXT("video-done.flag")));Next(31);}}
 else if(Stage==31 && StageFrames>240)PC->ConsoleCommand(TEXT("quit"),false);
}

#else
void AHOORunnerVisualQA::Initialize(AHOORunnerPawn*){}
void AHOORunnerVisualQA::Tick(float Dt){Super::Tick(Dt);}
#endif
