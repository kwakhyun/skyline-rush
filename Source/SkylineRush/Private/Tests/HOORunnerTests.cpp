#include "World/HOORunnerWorlds.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/HOORunnerTypes.h"
#include "Gameplay/HOORunnerGameMode.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerUploadChoice,"HOO.Runner.BestEligibleOnlineRecord",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerUploadChoice::RunTest(const FString&)
{
    TArray<FHOORunnerScoreEntry> Runs;
    TestTrue(TEXT("No run cannot be submitted"),HOORunnerRecords::BestSubmittable(Runs)==nullptr);
    FHOORunnerScoreEntry Long;Long.RunId=TEXT("long");Long.ReplayJson=TEXT("[]");Long.Ticks=144001;Long.Score=90000;Long.Distance=70000;
    FHOORunnerScoreEntry Valid=Long;Valid.RunId=TEXT("valid");Valid.Ticks=10000;Valid.Score=8000;Valid.Distance=3500;
    FHOORunnerScoreEntry Missing=Valid;Missing.RunId=TEXT("missing");Missing.ReplayJson.Empty();Missing.Score=95000;
    Runs={Long,Missing,Valid};
    TestTrue(TEXT("Long best and missing replay do not block a valid lower record"),HOORunnerRecords::BestSubmittable(Runs)==&Runs[2]);
    auto Tied=Valid;Tied.RunId=TEXT("tied");Tied.Distance=3600;Runs.Add(Tied);
    TestTrue(TEXT("Equal scores choose greater distance"),HOORunnerRecords::BestSubmittable(Runs)==&Runs[3]);
    Runs[3].Ticks=144000;
    TestTrue(TEXT("Exactly 20 minutes is eligible"),HOORunnerRecords::BestSubmittable(Runs)==&Runs[3]);
    Runs[3].Seed=1000001;Runs[2].Ticks=0;
    TestTrue(TEXT("Invalid seed or empty simulation cannot be submitted"),HOORunnerRecords::BestSubmittable(Runs)==nullptr);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerProgression,"HOO.Runner.ProgressionAndDeterminism",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerProgression::RunTest(const FString&)
{
    TestEqual(TEXT("Opening speed"),HOORunner::Speed(0),1400.f);
    TestEqual(TEXT("Top speed"),HOORunner::Speed(48000),3800.f);
    TestEqual(TEXT("Speed cap"),HOORunner::Speed(100000000),3800.f);
    TestEqual(TEXT("First threshold"),HOORunner::Difficulty(30000),2);
    TestEqual(TEXT("Early acceleration at 100m"),HOORunner::Speed(10000),1900.f);
    TestEqual(TEXT("Difficulty cap"),HOORunner::Difficulty(100000000),8);
    FHOORunnerState A,B;A.Start();B.Start();
    for(int I=0;I<120;++I) A.Advance(1.f/30);
    for(int I=0;I<576;++I) B.Advance(1.f/144);
    TestTrue(TEXT("Frame independent distance"),FMath::Abs(A.Distance-B.Distance)<.02);
    A.TogglePause();const double Distance=A.Distance;A.Advance(.2f);
    TestEqual(TEXT("Pause freezes movement"),A.Distance,Distance);
    A.Reset(17);
    TestTrue(TEXT("Reset clears run"),A.Distance==0 && A.Height==0 && A.Coins==0 && A.TargetLane==0 && A.Seed==17 && A.Phase==EHOORunnerPhase::Ready);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerHazards,"HOO.Runner.JumpSlideGapAndHitch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerHazards::RunTest(const FString&)
{
    for(const auto Kind:{EHOORunnerHazard::Barrier,EHOORunnerHazard::Overhead,EHOORunnerHazard::Gap})
    {
        int64 Row=-1;int32 Lane=0;
        for(int64 I=24;I<800 && Row<0;++I)
        {
            const auto T=HOORunner::Tile(I,409);
            for(int32 L=0;L<3;++L) if(T.Lanes[L]==Kind) { Row=I;Lane=L-1;break; }
        }
        TestTrue(TEXT("Hazard exists"),Row>=0);
        const double Center=(Row+.5)*HOORunner::TileLength;
        FHOORunnerState Hit;Hit.Reset(409);Hit.Start();Hit.Distance=Center-300;
        Hit.Lives=1;Hit.TargetLane=Lane;Hit.Lateral=Lane*300;Hit.Advance(.25f);
        TestTrue(TEXT("Grounded collision catches long frames"),Hit.Phase==EHOORunnerPhase::Crashed && Hit.Failure==Kind);
        FHOORunnerState Avoid;Avoid.Reset(409);Avoid.Start();
        Avoid.Distance=Center-HOORunner::Speed(Center)*.31;
        Avoid.TargetLane=Lane;Avoid.Lateral=Lane*300;
        if(Kind==EHOORunnerHazard::Overhead) Avoid.Slide();else Avoid.Jump();
        while(Avoid.Distance<Center+310 && Avoid.Phase==EHOORunnerPhase::Running) Avoid.Advance(1.f/60);
        TestTrue(TEXT("Correct action clears obstacle"),Avoid.Phase==EHOORunnerPhase::Running);
    }
    FHOORunnerState S;S.Start();S.Jump();
    for(int I=0;I<12;++I) S.Advance(1.f/60);
    S.Slide();
    for(int I=0;I<20 && !S.IsSliding();++I) S.Advance(1.f/60);
    TestTrue(TEXT("Air slide fast-falls into slide"),S.IsSliding());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerReachable,"HOO.Runner.ReachableGeneratedRoutes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerReachable::RunTest(const FString&)
{
    for(int Seed=400;Seed<416;++Seed)
    {
        FHOORunnerState S;S.Reset(Seed);S.Start();
        int32 Iterations=0;
        double MaxSpeedTime=0;
        while(S.Distance<350000 && S.Phase==EHOORunnerPhase::Running && ++Iterations<30000)
        {
            const int64 Current=FMath::FloorToInt64(S.Distance/HOORunner::TileLength);
            for(int64 I=FMath::Max<int64>(0,Current-1);I<Current+20;++I)
            {
                const auto T=HOORunner::Tile(I,Seed);
                if((I+.5)*600+280<S.Distance) continue;
                if(T.Lanes[0]==EHOORunnerHazard::None && T.Lanes[1]==EHOORunnerHazard::None && T.Lanes[2]==EHOORunnerHazard::None) continue;
                if(T.bFlightGap || S.IsFlying()) continue;
                if(T.bJumpRow)
                {
                    if(((I+.5)*600-S.Distance)/FMath::Max(S.CurrentSpeed,1400.f)<.31) S.Jump();
                    break;
                }
                if(T.Lanes[T.SafeLane+1]!=EHOORunnerHazard::None) { AddError(TEXT("Safe lane is blocked"));return false; }
                if(S.TargetLane!=T.SafeLane) S.Move(T.SafeLane>S.TargetLane?1:-1);
                break;
            }
            // Stress every course at the strongest boost, with no fever shield.
            S.BoostRemaining=HOORunner::BoostDuration;S.FeverRemaining=0;S.FeverCharge=0;
            S.Advance(1.f/60);
            if(MaxSpeedTime==0 && S.CurrentSpeed>=HOORunner::MaxSpeed) MaxSpeedTime=S.Elapsed;
        }
        TestTrue(TEXT("Boost reaches speed earlier"),MaxSpeedTime>10 && MaxSpeedTime<20);
        TestEqual(TEXT("Safe route needs no shield"),S.ProtectedThroughTile,static_cast<int64>(-1));
        TestTrue(TEXT("Powered speed is capped"),S.CurrentSpeed<=HOORunner::MaxSpeed*1.25f);
        if(S.Phase==EHOORunnerPhase::Crashed || S.Distance<350000)
        {
            AddError(FString::Printf(TEXT("Unreachable run seed=%d distance=%.1f lane=%d hazard=%s"),Seed,S.Distance,S.TargetLane,HOORunner::HazardName(S.Failure)));
            return false;
        }
    }
    return true;
}




IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerChain,"HOO.Runner.ChainRewardsAndPause",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerChain::RunTest(const FString&)
{
    FHOORunnerState R;R.Start();
    while(R.Distance<14000 && R.Phase==EHOORunnerPhase::Running) R.Advance(1.f/60);
    TestTrue(TEXT("Opening route supports a 10 shard chain"),R.Chain>=10 && R.Multiplier()==2);
    TestTrue(TEXT("A chain earns more than flat shard points"),R.PickupScore>R.Coins*20);
    const auto Remaining=R.ChainRemaining;
    const auto Score=R.Score();
    R.TogglePause();R.Advance(.25f);
    TestEqual(TEXT("Pause preserves chain window"),R.ChainRemaining,Remaining);
    R.TogglePause();R.Move(1);
    for(int I=0;I<160;++I) R.Advance(1.f/60);
    TestEqual(TEXT("Missing shards resets multiplier"),R.Multiplier(),1);
    TestTrue(TEXT("Earned points survive an expired chain"),R.Score()>=Score && R.BestChain>=10);
    R.Reset(409);
    TestTrue(TEXT("New run clears transient scoring"),R.Chain==0 && R.BestChain==0 && R.PickupScore==0 && R.Elapsed==0);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerPowers,"HOO.Runner.FeverAndBoosterLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerPowers::RunTest(const FString&)
{
    for(int Seed=400;Seed<416;++Seed)
        for(int64 I=0;I<2000;++I)
        {
            const auto T=HOORunner::Tile(I,Seed);
            if(!T.bBooster) continue;
            TestTrue(TEXT("Booster sits on a clear centre-lane island"),T.BoosterLane==0 && T.Lanes[0]==EHOORunnerHazard::None && T.Lanes[1]==EHOORunnerHazard::None && T.Lanes[2]==EHOORunnerHazard::None);
            TestTrue(TEXT("Booster schedule is bounded and reproducible"),I>=52 && (I-52)%60==0);
        }
    FHOORunnerState B;B.Start();B.Distance=31350;B.Advance(1.f/60);
    TestEqual(TEXT("Touching a ring collects one booster"),B.BoostersCollected,1);
    TestEqual(TEXT("Booster gives six fever charge"),B.FeverCharge,HOORunner::BoosterCharge);
    const float Time=B.BoostRemaining;B.TogglePause();B.Advance(.25f);
    TestEqual(TEXT("Pause freezes booster"),B.BoostRemaining,Time);
    B.TogglePause();B.Advance(.25f);B.Advance(.25f);
    TestEqual(TEXT("Ring cannot be collected twice"),B.BoostersCollected,1);
    TestTrue(TEXT("Boost ramps to 25 percent"),B.PowerSpeedAlpha>.99f && B.CurrentSpeed>HOORunner::Speed(B.Distance)*1.24f);
    for(int I=0;I<300 && B.Phase==EHOORunnerPhase::Running;++I)
    {
        B.Move(1);B.Advance(1.f/60);
    }
    // Test expiration on an empty opening, independent of later course collisions.
    FHOORunnerState Expire;Expire.Start();Expire.BoostRemaining=.05f;Expire.PowerSpeedAlpha=1;
    for(int I=0;I<60;++I) Expire.Advance(1.f/60);
    TestTrue(TEXT("Boost expires and returns smoothly to base speed"),!Expire.IsBoosting() && Expire.PowerSpeedAlpha==0);
    for(bool Air:{false,true})
    {
        FHOORunnerState Miss;Miss.Start();Miss.Distance=31350;
        if(Air) Miss.Height=230;else {Miss.TargetLane=-1;Miss.Lateral=-300;}
        Miss.Advance(1.f/60);
        TestEqual(TEXT("Wrong lane or too high misses ring"),Miss.BoostersCollected,0);
    }
    FHOORunnerState F;F.Start();F.Distance=31350;F.FeverCharge=23;F.ChainRemaining=2;
    F.Advance(1.f/60);
    TestTrue(TEXT("Booster can trigger fever"),F.IsBoosting() && F.IsFever() && F.FeverActivations==1 && F.FeverCharge==0);
    const float FeverTime=F.FeverRemaining;F.TogglePause();F.Advance(.25f);
    TestEqual(TEXT("Pause freezes fever"),F.FeverRemaining,FeverTime);
    F.TogglePause();
    F.FeverRemaining=HOORunner::FeverDuration;F.BoostRemaining=HOORunner::BoostDuration;
    F.Advance(.25f);F.Advance(.25f);
    TestTrue(TEXT("Overlapping effects never stack speed bonuses"),F.CurrentSpeed<=HOORunner::MaxSpeed*1.25f && F.PowerSpeedAlpha<=1.f);

    FHOORunnerState Magnet;Magnet.Start();Magnet.Distance=780;Magnet.Lateral=-300;Magnet.TargetLane=-1;Magnet.Height=200;Magnet.FeverRemaining=8;
    Magnet.Advance(1.f/60);
    TestEqual(TEXT("Fever magnet collects from another lane while airborne"),Magnet.Coins,1);
    TestEqual(TEXT("Fever doubles shard points"),Magnet.PickupScore,40);
    TestEqual(TEXT("Active fever does not recharge itself"),Magnet.FeverCharge,0);
    Magnet.Chain=30;
    TestEqual(TEXT("Fever plus chain gives x8"),Magnet.Multiplier(),8);
    FHOORunnerState Charge;Charge.Start();Charge.FeverCharge=23;Charge.ChainRemaining=.03f;Charge.Advance(.1f);
    TestEqual(TEXT("Broken chain drops unspent fever charge"),Charge.FeverCharge,0);

    for(const auto Kind:{EHOORunnerHazard::Barrier,EHOORunnerHazard::Overhead,EHOORunnerHazard::Gap})
    {
        int64 Row=-1;int32 Lane=0;
        for(int64 I=24;I<800 && Row<0;++I)
        {
            const auto T=HOORunner::Tile(I,409);
            for(int32 L=0;L<3;++L) if(T.Lanes[L]==Kind) {Row=I;Lane=L-1;break;}
        }
        const double Center=(Row+.5)*HOORunner::TileLength;
        FHOORunnerState Shield;Shield.Reset(409);Shield.Start();
        Shield.Distance=Center-(Kind==EHOORunnerHazard::Gap?290:127);
        Shield.Lateral=Lane*300;Shield.TargetLane=Lane;Shield.FeverRemaining=.08f;
        while(Shield.Distance<Center+310 && Shield.Phase==EHOORunnerPhase::Running) Shield.Advance(1.f/120);
        TestTrue(TEXT("Fever protects every hazard and permits exit after expiry"),Shield.Phase==EHOORunnerPhase::Running && Shield.ProtectedThroughTile==Row && !Shield.IsFever());
        FHOORunnerState NoShield;NoShield.Reset(409);NoShield.Start();NoShield.Distance=Center-300;
        NoShield.Lateral=Lane*300;NoShield.TargetLane=Lane;NoShield.BoostRemaining=4;
        NoShield.Lives=1;NoShield.Advance(.25f);
        TestTrue(TEXT("Boost alone still requires avoiding hazards"),NoShield.Phase==EHOORunnerPhase::Crashed);
    }
    F.Reset(410);
    TestTrue(TEXT("Retry clears all power state"),F.FeverCharge==0 && F.FeverRemaining==0 && F.BoostRemaining==0 && F.PowerSpeedAlpha==0 && F.BoostersCollected==0 && F.FeverActivations==0 && F.LastBoosterTile==-1 && F.ProtectedThroughTile==-1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerPowerDeterminism,"HOO.Runner.PowerDeterminism",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerPowerDeterminism::RunTest(const FString&)
{
    FHOORunnerState A,B;
    for(auto* S:{&A,&B}) {S->Start();S->Distance=31350;S->FeverCharge=23;S->ChainRemaining=2;}
    for(int I=0;I<120;++I) A.Advance(1.f/30);
    for(int I=0;I<576;++I) B.Advance(1.f/144);
    TestTrue(TEXT("Power pickup and timers are frame independent"),FMath::Abs(A.Distance-B.Distance)<.03 && FMath::Abs(A.FeverRemaining-B.FeverRemaining)<.0001f && FMath::Abs(A.BoostRemaining-B.BoostRemaining)<.0001f);
    TestTrue(TEXT("Powered rewards are deterministic"),A.Score()==B.Score() && A.BoostersCollected==B.BoostersCollected && A.FeverActivations==B.FeverActivations);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerWorld,"HOO.Runner.ThemesLoopAndLaunch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerWorld::RunTest(const FString&)
{
    for(double Boundary:{60000.,96000.,126000.,166000.,180000.,206000.,236000.,240000.,260000.,280000.,300000.})
        TestTrue(TEXT("Theme connections are continuous"),(HOORunner::Center(Boundary+1)-HOORunner::Center(Boundary-1)).Size()<5);
    TestTrue(TEXT("Climb reaches high altitude"),HOORunner::Center(96000).Z>17000);
    TestTrue(TEXT("Dive has a steep negative pitch"),HOORunner::Frame(111000).GetUnitAxis(EAxis::X).Z<-.6);
    TestTrue(TEXT("Underwater track lies below the sea"),HOORunner::Center(145000).Z<-2000);
    double MinimumUp=1;
    for(double S=206000;S<=236000;S+=50)
    {
        const auto Frame=HOORunner::Frame(S);
        TestFalse(TEXT("Loop frame remains finite"),Frame.ContainsNaN());
        const auto Next=HOORunner::Frame(S+50);
        TestTrue(TEXT("Loop curvature avoids tight folds"),Frame.GetRotation().AngularDistance(Next.GetRotation())<.04);
        TestTrue(TEXT("Loop travel remains smooth"),(HOORunner::Center(S+1)-HOORunner::Center(S-1)).Length()>1.7);
        MinimumUp=FMath::Min(MinimumUp,Frame.GetUnitAxis(EAxis::Z).Z);
    }
    TestTrue(TEXT("Loop genuinely turns upside down"),MinimumUp<-.75);
    FHOORunnerState Fly;Fly.Start();Fly.Distance=236350;
    float Peak=0;
    while(Fly.Distance<260500 && Fly.Phase==EHOORunnerPhase::Running) {Fly.Advance(1.f/60);Peak=FMath::Max(Peak,Fly.Height);}
    TestTrue(TEXT("Pad flies across missing deck and lands in the next theme"),Fly.Launches==1 && Fly.Phase==EHOORunnerPhase::Running && !Fly.IsFlying() && Fly.Height==0 && Peak>6400 && HOORunner::Theme(Fly.Distance)==EHOORunnerTheme::Clouds);
    FHOORunnerState Gap;Gap.Start();Gap.Distance=194500;Gap.Lives=1;Gap.Advance(.1f);
    TestTrue(TEXT("Jump island full-width gaps require an action"),Gap.Phase==EHOORunnerPhase::Crashed);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerReplayExport,"HOO.Runner.OnlineReplayFixtures",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerReplayExport::RunTest(const FString&)
{
    TArray<FString> Fixtures;
    for(int Seed:{409,410,411})
    {
        FHOORunnerState S;S.Reset(Seed);S.Start();TArray<FString> Inputs;
        auto Input=[&](const TCHAR* Action)
        {
            Inputs.Add(FString::Printf(TEXT("{\"tick\":%d,\"action\":\"%s\"}"),S.SimulationTicks,Action));
            if(FCString::Strcmp(Action,TEXT("l"))==0) S.Move(-1);
            else if(FCString::Strcmp(Action,TEXT("r"))==0) S.Move(1);
            else if(FCString::Strcmp(Action,TEXT("s"))==0) S.Slide();
            else S.Jump();
        };
        while(S.Distance<350000 && S.Phase==EHOORunnerPhase::Running && S.SimulationTicks<20000)
        {
            const int64 Current=FMath::FloorToInt64(S.Distance/600);
            for(int64 I=FMath::Max<int64>(0,Current-1);I<Current+25 && !S.IsFlying();++I)
            {
                const auto T=HOORunner::Tile(I,Seed);const double Ahead=(I+.5)*600-S.Distance;
                if(Ahead<-280 || T.bFlightGap) continue;
                if(T.bBooster && Ahead>0 && Ahead<3500) {if(S.TargetLane!=0) Input(S.TargetLane>0?TEXT("l"):TEXT("r"));break;}
                if(T.Lanes[0]==EHOORunnerHazard::None && T.Lanes[1]==EHOORunnerHazard::None && T.Lanes[2]==EHOORunnerHazard::None) continue;
                if(Seed==411 && T.Risk!=EHOORunnerHazard::None)
                {
                    if(S.TargetLane!=T.RiskLane) Input(T.RiskLane>S.TargetLane?TEXT("r"):TEXT("l"));
                    if(Ahead>0 && Ahead/S.CurrentSpeed<(T.Risk==EHOORunnerHazard::Gap?.31:.24))
                    {
                        if(T.Risk==EHOORunnerHazard::Overhead) {if(!S.IsSliding())Input(TEXT("s"));}
                        else if(S.Height<1) Input(TEXT("j"));
                    }
                    break;
                }
                if(T.bJumpRow) {if(Ahead/S.CurrentSpeed<.31) Input(TEXT("j"));break;}
                if(S.TargetLane!=T.SafeLane) Input(T.SafeLane>S.TargetLane?TEXT("r"):TEXT("l"));
                break;
            }
            S.Advance(1.f/60);
        }
        TestTrue(TEXT("Replay fixture completes every theme"),S.Distance>=350000 && S.Launches==1);
        if(Seed==411) TestTrue(TEXT("Risk replay actually exercises optional scoring"),S.RiskClears>0 && S.StyleScore>0);
        Fixtures.Add(FString::Printf(TEXT("{\"seed\":%d,\"ticks\":%d,\"rules_version\":7,\"client_score\":%d,\"distance\":%.6f,\"shards\":%d,\"fevers\":%d,\"launches\":%d,\"events\":[%s]}"),
            Seed,S.SimulationTicks,S.Score(),S.Distance/100,S.Coins,S.FeverActivations,S.Launches,*FString::Join(Inputs,TEXT(","))));
        Fixtures.Last().RemoveFromEnd(TEXT("}"));
        Fixtures.Last()+=FString::Printf(TEXT(",\"riskScore\":%d,\"styleScore\":%d,\"risks\":%d,\"nearMisses\":%d}"),S.RiskScore,S.StyleScore,S.RiskClears,S.NearMisses);
    }
    {
        FHOORunnerState S;S.Reset(409);S.Start();
        while(S.Phase==EHOORunnerPhase::Running && S.SimulationTicks<30000) S.Advance(1.f/60);
        TestTrue(TEXT("No-input replay uses all three lives"),S.Lives==0 && S.Hits==3);
        Fixtures.Add(FString::Printf(TEXT("{\"seed\":409,\"ticks\":%d,\"rules_version\":7,\"client_score\":%d,\"distance\":%.6f,\"shards\":%d,\"fevers\":%d,\"launches\":%d,\"lives\":0,\"hits\":3,\"events\":[]}"),S.SimulationTicks,S.Score(),S.Distance/100,S.Coins,S.FeverActivations,S.Launches));
    }
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("QA/Replay");
    IFileManager::Get().MakeDirectory(*Dir,true);
    TestTrue(TEXT("Export real C++ replay evidence"),FFileHelper::SaveStringToFile(TEXT("[")+FString::Join(Fixtures,TEXT(","))+TEXT("]"),*(Dir/TEXT("ReplayFixtures.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerLives,"HOO.Runner.ThreeLivesRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerLives::RunTest(const FString&)
{
 FHOORunnerState S;S.Start();TestEqual(TEXT("Starts with three lives"),S.Lives,3);
 while(S.Hits==0 && S.SimulationTicks<3000)S.Advance(1.f/120);
 TestTrue(TEXT("First impact recovers"),S.Lives==2 && S.Phase==EHOORunnerPhase::Running && S.RecoveryRemaining>2);
 const auto Recovery=S.RecoveryRemaining;S.TogglePause();S.Advance(.25f);TestEqual(TEXT("Pause freezes recovery"),S.RecoveryRemaining,Recovery);S.TogglePause();
 S.Advance(.25f);TestEqual(TEXT("Impact cannot repeat within one row"),S.Hits,1);
 TestTrue(TEXT("Impact briefly slows runner"),S.CurrentSpeed<HOORunner::Speed(S.Distance));
 while(S.Phase==EHOORunnerPhase::Running && S.SimulationTicks<30000)S.Advance(1.f/120);
 TestTrue(TEXT("Third impact ends run"),S.Hits==3 && S.Lives==0 && S.Phase==EHOORunnerPhase::Crashed);
 S.Reset(410);TestTrue(TEXT("Retry restores lives and clears recovery"),S.Lives==3 && S.Hits==0 && S.RecoveryRemaining==0);
 S.Start();S.Distance=194400;S.Advance(.1f);
 TestTrue(TEXT("Gap recovers instead of ending first life"),S.Lives==2 && S.Height>0 && S.Phase==EHOORunnerPhase::Running);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerCorners,"HOO.Runner.CornerSpineAndLaneFrames",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerCorners::RunTest(const FString&)
{
    const FVector Start=HOORunner::Center(0),End=HOORunner::Center(300000);
    const FVector Delta=End-Start;
    TestTrue(TEXT("Right ninety heading"),FMath::Abs(HOORunner::Heading(82000).Yaw-90)<.05);
    TestTrue(TEXT("Left water sweep"),HOORunner::Frame(150000).GetUnitAxis(EAxis::X).Y<-.7);
    TestTrue(TEXT("Turn warning available 100m ahead"),HOORunner::TurnPreview(14000)>5);
    bool Continuous=true,Orthonormal=true,LaneWidth=true,Periodic=true;
    for(double S=0;S<1200000;S+=100)
    {
        const auto F=HOORunner::Frame(S);
        const FVector X=F.GetUnitAxis(EAxis::X),Y=F.GetUnitAxis(EAxis::Y),Z=F.GetUnitAxis(EAxis::Z);
        Continuous&=(HOORunner::Center(S+1)-HOORunner::Center(S-1)).Size()<4;
        Orthonormal&=!F.ContainsNaN() && FMath::Abs(X.SizeSquared()-1)<.00001 && FMath::Abs(FVector::DotProduct(X,Y))<.00001 && FVector::CrossProduct(X,Y).Equals(Z,.00001);
        LaneWidth&=(F.TransformPosition(FVector(0,300,0))-F.TransformPosition(FVector(0,-300,0))).Size()>599.99;
        Periodic&=(HOORunner::Center(S+300000)-HOORunner::Center(S)).Equals(Delta,.001);
    }
    TestTrue(TEXT("Continuous through all eight bends and cycle joins"),Continuous);
    TestTrue(TEXT("90 degree bends and vertical loops never lose local right axis"),Orthonormal);
    TestTrue(TEXT("Three lane width retained"),LaneWidth);
    TestTrue(TEXT("Infinite translated cycles do not accumulate error"),Periodic);
    TestTrue(TEXT("Visual audit anchor remains unchanged"),HOORunner::Center(8000).Equals(FVector(8000,600*FMath::Square(FMath::Sin(PI*8000/60000)),1300),.0001));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerWorldSequence,"HOO.Runner.FiveWorldSequence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerWorldSequence::RunTest(const FString&)
{
    const double Bounds[]={0,60000,126000,166000,206000,300000};
    for(int Cycle=0;Cycle<100;++Cycle)for(int B=0;B<5;++B)
    {
        const double Start=Cycle*300000.+Bounds[B],End=Cycle*300000.+Bounds[B+1];
        TestEqual(TEXT("Exact biome entry"),static_cast<int>(HOORunnerWorld::Biome(Start)),B);
        TestEqual(TEXT("Last centimetre belongs to previous space"),static_cast<int>(HOORunnerWorld::Biome(End-1)),B);
        TestEqual(TEXT("Transition countdown reaches one centimetre"),HOORunnerWorld::Remaining(End-1),1.);
        TestEqual(TEXT("Progress resets at threshold"),HOORunnerWorld::Progress(Start),0.f);
    }
    TestEqual(TEXT("Negative preview tiles are forest"),static_cast<int>(HOORunnerWorld::Biome(-600)),0);
    TestTrue(TEXT("Physical aquarium remains replay-compatible"),HOORunner::Theme(145000)==EHOORunnerTheme::Underwater);
    TestTrue(TEXT("Jungle retains jump gaps"),HOORunner::Tile(324,409).bJumpRow);
    TestTrue(TEXT("Dimensional flight retains missing deck"),HOORunner::Tile(413,409).bFlightGap);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunnerSpecials,"HOO.Runner.SpecialRiskRewardAndNearMiss",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRunnerSpecials::RunTest(const FString&)
{
    for(int Seed=400;Seed<416;++Seed)for(int Cycle=0;Cycle<20;++Cycle)for(int Start:{60,144,228,288,452})
    {
        int Encounters=0;
        for(int P=0;P<24;++P)
        {
            const auto T=HOORunner::Tile(Cycle*500+Start+P,Seed);
            if(T.Risk!=EHOORunnerHazard::None){++Encounters;TestTrue(TEXT("36m entrance and encounter warning"),P==6 || P==12 || P==18);}
            TestTrue(TEXT("Special centre lane always open"),T.SafeLane==0 && T.Lanes[1]==EHOORunnerHazard::None);
        }
        TestEqual(TEXT("Exactly three optional layouts in each special section"),Encounters,3);
    }
    for(int I=0;I<3;++I)
    {
        const int64 Row=66+I*6;const auto T=HOORunner::Tile(Row,409);const double Depth=I==2?275:112,Center=(Row+.5)*600;
        for(int Mode=0;Mode<3;++Mode)
        {
            FHOORunnerState S;S.Start();S.Distance=Center-Depth-10;S.TargetLane=T.RiskLane;S.Lateral=T.RiskLane*300;
            if(I==0)S.Height=170;else if(I==1)S.SlideRemaining=.18f;else S.Height=75;
            if(Mode==1){S.Height=0;S.SlideRemaining=0;}
            if(Mode==2)S.FeverRemaining=2;
            while(S.Distance<Center+Depth+160 && S.Phase==EHOORunnerPhase::Running)S.Advance(1.f/120);
            if(Mode==0)
            {
                TestEqual(TEXT("Precise clean pass pays one near miss"),S.NearMisses,1);
                TestEqual(TEXT("Each optional obstacle pays once"),S.RiskClears,1);
                TestEqual(TEXT("Score breakdown sums exactly"),S.Score(),S.DistanceScore()+S.PickupScore+S.RiskScore+S.StyleScore);
                const auto Bonus=S.StyleScore+S.RiskScore;S.TogglePause();S.Advance(.25f);S.TogglePause();S.Advance(.1f);
                TestEqual(TEXT("Pause and lingering cannot repeat bonus"),S.StyleScore+S.RiskScore,Bonus);
            }
            else TestTrue(TEXT("Hit or invulnerability cannot earn precision/risk bonuses"),S.NearMisses==0 && S.RiskClears==0);
            S.Reset(409);TestTrue(TEXT("Retry clears bonus counters and samples"),S.StyleScore==0 && S.RiskScore==0 && S.PassSamples[Row%4].Tile==-1);
        }
    }
    return true;
}
#endif
