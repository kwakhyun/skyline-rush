#include "Gameplay/HOORunnerTypes.h"

namespace
{
    uint32 Mix(uint32 X)
    {
        X ^= X >> 16; X *= 0x7feb352dU; X ^= X >> 15; X *= 0x846ca68bU;
        return X ^ (X >> 16);
    }
}

int32 HOORunner::Difficulty(double DistanceCm)
{
    return FMath::Clamp(1 + FMath::FloorToInt(FMath::Max(0.0, DistanceCm) / 30000.0), 1, 8);
}

float HOORunner::Speed(double DistanceCm)
{
    return FMath::Clamp(InitialSpeed + static_cast<float>(FMath::Max(0.0, DistanceCm)) * AccelerationPerCm,
        InitialSpeed, MaxSpeed);
}

FHOORunnerTile HOORunner::Tile(int64 Index, int32 Seed)
{
    FHOORunnerTile Result;
    Result.Index = Index;
    const double Local=FMath::Fmod((Index+.5)*TileLength,ThemeCycleLength);
    const auto Section=Theme((Index+.5)*TileLength);
    if(Section==EHOORunnerTheme::Loop || Section==EHOORunnerTheme::Launch || Section==EHOORunnerTheme::Flight)
    {
        Result.bLaunchPad=Local>=236400 && Local<240000;
        Result.bFlightGap=Section==EHOORunnerTheme::Flight;
        if(Result.bFlightGap) for(auto& Hazard:Result.Lanes) Hazard=EHOORunnerHazard::Gap;
        return Result;
    }
    if(Section==EHOORunnerTheme::JumpIsles && Local>=179000 && Index%24==12)
    {
        Result.bJumpRow=true;
        for(auto& Hazard:Result.Lanes) Hazard=EHOORunnerHazard::Gap;
        return Result;
    }
    if (Index < 24) return Result; // 144 m of readable, unobstructed opening.
    // A clear centre-lane island, 12m before the nearest possible encounter.
    // First booster at 315m, then every 360m. Rendering and pickup share this definition.
    Result.bBooster = Index >= 52 && (Index - 52) % 60 == 0;
    Result.BoosterLane = 0;
    const int32 Level = Difficulty(Index * TileLength);
    const int64 Group = Index / 12;
    const int32 Phase = static_cast<int32>(Index % 12);
    const uint32 Random = Mix(static_cast<uint32>(Group) ^ static_cast<uint32>(Seed));
    // Every encounter retains the centre or an adjacent lane. Never jump -1 to +1.
    static constexpr int32 SafeCycle[] = {-1, 0, 1, 0};
    Result.SafeLane = Level < 3 ? 0 : SafeCycle[(Group + (Seed & 3)) % 4];
    const bool bRow = Phase == 0 || (Level >= 4 && Phase == 6);
    if (bRow)
    {
        if (Level < 3)
        {
            // Tutorial: centre obstruction with both side exits. Later alternates side obstacles.
            const int32 Lane = Group % 3 == 2 ? 0 : (Group % 2 ? -1 : 1);
            Result.SafeLane = Lane == 0 ? 1 : 0;
            Result.Lanes[Lane + 1] = Level >= 2 && Group % 2 == 0
                ? EHOORunnerHazard::Overhead : EHOORunnerHazard::Barrier;
        }
        else
        {
            const int32 Safe = Phase == 6 ? 0 : Result.SafeLane;
            Result.SafeLane = Safe;
            for (int32 Lane = -1; Lane <= 1; ++Lane)
            {
                if (Lane == Safe) continue;
                const uint32 Kind = (Random + Lane + 3 + Phase) % 3;
                Result.Lanes[Lane + 1] = static_cast<EHOORunnerHazard>(Kind + 1);
            }
        }
    }
    // Elevated narrow sections start at 1200m: the centre deck remains continuous.
    if (Level >= 5 && (Phase == 8 || Phase == 9))
    {
        Result.SafeLane = 0;
        Result.Lanes[(Group % 2) ? 0 : 2] = EHOORunnerHazard::Gap;
        if (Level >= 7) Result.Lanes[(Group % 2) ? 2 : 0] = EHOORunnerHazard::Gap;
    }
    return Result;
}

EHOORunnerTheme HOORunner::Theme(double S)
{
    const double U=FMath::Fmod(FMath::Max(0.,S),ThemeCycleLength);
    if(U<60000) return EHOORunnerTheme::City;
    if(U<96000) return EHOORunnerTheme::Climb;
    if(U<126000) return EHOORunnerTheme::Dive;
    if(U<166000) return EHOORunnerTheme::Underwater;
    if(U<206000) return EHOORunnerTheme::JumpIsles;
    if(U<236000) return EHOORunnerTheme::Loop;
    if(U<240000) return EHOORunnerTheme::Launch;
    if(U<260000) return EHOORunnerTheme::Flight;
    return EHOORunnerTheme::Clouds;
}
const TCHAR* HOORunner::ThemeName(EHOORunnerTheme T)
{
    switch(T)
    {
    case EHOORunnerTheme::City:return TEXT("햇살 도심");
    case EHOORunnerTheme::Climb:return TEXT("하늘 오르막");
    case EHOORunnerTheme::Dive:return TEXT("스카이 다이브");
    case EHOORunnerTheme::Underwater:return TEXT("푸른 바닷길");
    case EHOORunnerTheme::JumpIsles:return TEXT("점프 아일랜드");
    case EHOORunnerTheme::Loop:return TEXT("360 스카이 루프");
    case EHOORunnerTheme::Launch:return TEXT("별빛 발사대");
    case EHOORunnerTheme::Flight:return TEXT("구름으로 날아가요");
    default:return TEXT("구름 정원");
    }
}
static FVector LegacyCenter(double S)
{
    if(S<0) return FVector(S,0,1300);
    const double Cycle=FMath::FloorToDouble(S/HOORunner::ThemeCycleLength);
    const double U=S-Cycle*HOORunner::ThemeCycleLength;
    auto Ease=[](double T) { T=FMath::Clamp(T,0.,1.);return T*T*(3.-2.*T); };
    double X=U,Y=0,Z=1300;
    if(U<60000) Y=600*FMath::Square(FMath::Sin(PI*U/60000));
    else if(U<96000) Z=1300+16000*Ease((U-60000)/36000);
    else if(U<126000) Z=17300-19500*Ease((U-96000)/30000);
    else if(U<166000) Z=-2200;
    else if(U<206000)
    {
        Z=-2200+4000*Ease((U-166000)/12000);
        if(U>180000) Z+=450*FMath::Square(FMath::Sin(PI*(U-180000)/26000));
    }
    else if(U<236000)
    {
        const double T=(U-206000)/30000,Angle=2*PI*T;
        X=206000+5000*FMath::Sin(Angle)+2000*Ease(T);
        Y=1800*Ease(T);
        Z=1800+5000*(1-FMath::Cos(Angle));
    }
    else
    {
        X=U-28000;Y=1800;
        Z=1800+3200*Ease((U-240000)/20000);
        if(U>280000) Z=5000-3700*Ease((U-280000)/20000);
    }
    return FVector(Cycle*272000+X,Cycle*1800+Y,Z);
}

namespace
{
    // Geometry only: the simulation still advances in deterministic course centimetres.
    // Integrated unit tangent gives constant longitudinal scale and a bounded 128 KB LUT.
    struct FCorner { double Start, End, Degrees; };
    static constexpr FCorner Corners[]={
        {18000,32000,60},{36000,54000,-60}, // broad right and left S bend
        {62000,78000,90},{90000,108000,-90}, // elevated ninety and descending return
        {130000,148000,-55},{154000,172000,55}, // aquarium sweep
        {234000,246000,40},{250000,266000,-40} // cloud slalom
    };
    static double SpineYaw(double X)
    {
        double A=0;
        for(const auto& C:Corners) {const double T=FMath::Clamp((X-C.Start)/(C.End-C.Start),0.,1.);A+=C.Degrees*T*T*(3.-2.*T);}
        return FMath::DegreesToRadians(A);
    }
    struct FCourseSpine
    {
        static constexpr int32 Count=5441;
        FVector2D Points[Count];
        FCourseSpine()
        {
            Points[0]=FVector2D::ZeroVector;
            for(int32 I=1;I<Count;++I)
            {
                const double A=SpineYaw((I-.5)*50.);
                Points[I]=Points[I-1]+FVector2D(FMath::Cos(A),FMath::Sin(A))*50.;
            }
        }
        FVector2D At(double X) const
        {
            X=FMath::Clamp(X,0.,272000.);
            const int32 I=FMath::Min(Count-2,FMath::FloorToInt(X/50.));
            return FMath::Lerp(Points[I],Points[I+1],(X-I*50.)/50.);
        }
    };
    static const FCourseSpine& Spine() {static const FCourseSpine Value;return Value;}
    static FVector LocalRight(double S)
    {
        if(S<0)return FVector::RightVector;
        const double Cycle=FMath::FloorToDouble(S/HOORunner::ThemeCycleLength);
        const double A=SpineYaw(LegacyCenter(S).X-Cycle*272000.);
        return FVector(-FMath::Sin(A),FMath::Cos(A),0);
    }
}
FVector HOORunner::Center(double S)
{
    if(S<0)return LegacyCenter(S);
    const double Cycle=FMath::FloorToDouble(S/ThemeCycleLength);
    const FVector Base=LegacyCenter(S);
    const FVector2D XY=Spine().At(Base.X-Cycle*272000.)+
        (Spine().At(272000.)+FVector2D(0,1800))*Cycle;
    return FVector(XY.X,XY.Y,Base.Z)+LocalRight(S)*(Base.Y-Cycle*1800.);
}
FTransform HOORunner::Frame(double S)
{
    const FVector Forward=(Center(S+1)-Center(S-1)).GetSafeNormal();
    const FVector Ref=LocalRight(S);
    // Course-local reference remains nonsingular at 90 degree horizontal turns AND vertical loops.
    const FVector Right=(Ref-Forward*FVector::DotProduct(Forward,Ref)).GetSafeNormal();
    return FTransform(FRotationMatrix::MakeFromXY(Forward,Right).ToQuat(),Center(S));
}
float HOORunner::TurnPreview(double S)
{
    const auto ThemeNow=Theme(S);
    if(ThemeNow==EHOORunnerTheme::Loop || ThemeNow==EHOORunnerTheme::Flight)return 0;
    const FVector A=LocalRight(S),B=LocalRight(S+10000.);
    return FMath::RadiansToDegrees(FMath::Atan2(FVector::CrossProduct(A,B).Z,FVector::DotProduct(A,B)));
}
FRotator HOORunner::Heading(double S) { return Frame(S).Rotator(); }

const TCHAR* HOORunner::HazardName(EHOORunnerHazard Hazard)
{
    switch (Hazard)
    {
    case EHOORunnerHazard::Barrier: return TEXT("BARRIER");
    case EHOORunnerHazard::Overhead: return TEXT("OVERHEAD");
    case EHOORunnerHazard::Gap: return TEXT("GAP");
    default: return TEXT("NONE");
    }
}

void FHOORunnerState::Reset(int32 InSeed)
{
    *this = FHOORunnerState();
    Seed = InSeed;
}

void FHOORunnerState::Start()
{
    if (Phase == EHOORunnerPhase::Ready) Phase = EHOORunnerPhase::Running;
}

void FHOORunnerState::Move(int32 Direction)
{
    if (Phase == EHOORunnerPhase::Running && !IsFlying())
        TargetLane = FMath::Clamp(TargetLane + FMath::Clamp(Direction, -1, 1), -1, 1);
}

void FHOORunnerState::Jump()
{
    if (Phase == EHOORunnerPhase::Ready) { Start(); return; }
    if (Phase == EHOORunnerPhase::Running && !IsSliding() && !IsFlying()) JumpBuffer = .14f;
}

void FHOORunnerState::Slide()
{
    if (Phase != EHOORunnerPhase::Running || IsSliding() || IsFlying()) return;
    if (Height < 1.0f) SlideRemaining = HOORunner::SlideDuration;
    else { VerticalSpeed = FMath::Min(VerticalSpeed, -950.0f); bSlideOnLanding = true; }
}

void FHOORunnerState::TogglePause()
{
    if (Phase == EHOORunnerPhase::Running) Phase = EHOORunnerPhase::Paused;
    else if (Phase == EHOORunnerPhase::Paused) Phase = EHOORunnerPhase::Running;
    Accumulator = 0.0;
}

void FHOORunnerState::Advance(float DeltaSeconds)
{
    if (Phase != EHOORunnerPhase::Running) { Accumulator = 0.0; return; }
    if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;
    // Bound hitch catch-up; each simulated slice still checks hazards and never teleports.
    Accumulator += FMath::Min(DeltaSeconds, .25f);
    while (Accumulator + 1.e-9 >= HOORunner::FixedStep && Phase == EHOORunnerPhase::Running)
    {
        Step(static_cast<float>(HOORunner::FixedStep));
        Accumulator -= HOORunner::FixedStep;
    }
}

void FHOORunnerState::ChargeFever(int32 Amount)
{
    if(IsFever()) return;
    FeverCharge = FMath::Min(HOORunner::FeverThreshold, FeverCharge + Amount);
    if(FeverCharge >= HOORunner::FeverThreshold)
    {
        FeverCharge = 0;
        FeverRemaining = HOORunner::FeverDuration;
        ++FeverActivations;
    }
}

void FHOORunnerState::Step(float Dt)
{
    if (Phase != EHOORunnerPhase::Running) return;
    Elapsed += Dt;
    ++SimulationTicks;
    RecoveryRemaining=FMath::Max(0.f,RecoveryRemaining-Dt);
    HitSlowRemaining=FMath::Max(0.f,HitSlowRemaining-Dt);
    ChainRemaining = FMath::Max(0.f, ChainRemaining - Dt);
    if(ChainRemaining <= 0) { Chain = 0; FeverCharge = 0; }
    FeverRemaining = FMath::Max(0.f,FeverRemaining-Dt);
    BoostRemaining = FMath::Max(0.f,BoostRemaining-Dt);
    const float PowerTarget = IsBoosting() ? 1.f : IsFever() ? .6f : 0.f;
    PowerSpeedAlpha = FMath::FInterpConstantTo(PowerSpeedAlpha,PowerTarget,Dt,PowerTarget>PowerSpeedAlpha?2.f:1.5f);
    // Effects use the strongest speed bonus, never multiply into unsafe speeds.
    CurrentSpeed = HOORunner::Speed(Distance) * (1.f + HOORunner::PowerSpeedBonus * PowerSpeedAlpha);
    CurrentSpeed *= 1.f-.35f*(HitSlowRemaining/HOORunner::HitSlowDuration);
    Distance += CurrentSpeed * Dt;
    Lateral = FMath::FInterpConstantTo(Lateral, TargetLane * HOORunner::LaneWidth, Dt, 1800.0f);
    SlideRemaining = FMath::Max(0.0f, SlideRemaining - Dt);
    if (JumpBuffer > 0.0f && Height <= 0.0f && !IsSliding())
    {
        VerticalSpeed = HOORunner::JumpVelocity;
        JumpBuffer = 0.0f;
    }
    JumpBuffer = FMath::Max(0.0f, JumpBuffer - Dt);
    if(FlightStart>=0)
    {
        if(Distance>=FlightEnd) { FlightStart=-1;FlightEnd=-1;Height=0;VerticalSpeed=0; }
        else
        {
            const double U=FMath::Clamp((Distance-FlightStart)/(FlightEnd-FlightStart),0.,1.);
            Height=6500*4*U*(1-U);VerticalSpeed=0;SlideRemaining=0;JumpBuffer=0;
        }
    }
    if (!IsFlying() && (Height > 0.0f || VerticalSpeed > 0.0f))
    {
        Height += VerticalSpeed * Dt - .5f * HOORunner::Gravity * Dt * Dt;
        VerticalSpeed -= HOORunner::Gravity * Dt;
        if (Height <= 0.0f)
        {
            Height = 0.0f; VerticalSpeed = 0.0f;
            if (bSlideOnLanding) { SlideRemaining = HOORunner::SlideDuration; bSlideOnLanding = false; }
        }
    }
    const int64 TileIndex = FMath::FloorToInt64(Distance / HOORunner::TileLength);
    for (int64 I = FMath::Max<int64>(0, TileIndex - 1); I <= TileIndex + 1; ++I)
    {
        const auto Tile = HOORunner::Tile(I, Seed);
        const double Along = FMath::Abs(Distance - (I + .5) * HOORunner::TileLength);
        const int64 Cycle=FMath::FloorToInt64(Distance/HOORunner::ThemeCycleLength);
        if(Tile.bLaunchPad && Along<275 && Cycle>LastLaunchCycle && Height<280)
        {
            LastLaunchCycle=Cycle;++Launches;
            FlightStart=Distance;FlightEnd=Cycle*HOORunner::ThemeCycleLength+260100;
            TargetLane=0;SlideRemaining=0;VerticalSpeed=0;JumpBuffer=0;bSlideOnLanding=false;
        }
        for (int32 Lane = -1; Lane <= 1; ++Lane)
        {
            const auto Hazard = Tile.Lanes[Lane + 1];
            if (Hazard == EHOORunnerHazard::None) continue;
            const float Width = Hazard == EHOORunnerHazard::Gap ? 118.0f : 142.0f;
            if (FMath::Abs(Lateral - Lane * HOORunner::LaneWidth) > Width) continue;
            const double Depth = Hazard == EHOORunnerHazard::Gap ? 275.0 : 112.0;
            if (Along > Depth) continue;
            const bool bHit = (Hazard == EHOORunnerHazard::Barrier && Height < 130.0f)
                || (Hazard == EHOORunnerHazard::Overhead && !IsSliding())
                || (Hazard == EHOORunnerHazard::Gap && Height < 24.0f);
            if (bHit && !IsFlying())
            {
                if(IsFever() || RecoveryRemaining>0) ProtectedThroughTile = FMath::Max(ProtectedThroughTile,I);
                // A tile already crossed under the shield stays safe when fever expires mid-row.
                if(I > ProtectedThroughTile)
                {
                    LastHit=Hazard;++Hits;--Lives;
                    if(Lives<=0) { Lives=0;Failure=Hazard;Phase=EHOORunnerPhase::Crashed;return; }
                    RecoveryRemaining=HOORunner::RecoveryDuration;HitSlowRemaining=HOORunner::HitSlowDuration;
                    ProtectedThroughTile=I;Chain=0;ChainRemaining=0;FeverCharge=0;
                    BoostRemaining=0;PowerSpeedAlpha=0;
                    if(Hazard==EHOORunnerHazard::Gap) {Height=80;VerticalSpeed=450;TargetLane=0;}

                }
            }
        }
        if(Tile.bBooster && I > LastBoosterTile && Along < 140.0
            && FMath::Abs(Lateral-Tile.BoosterLane*HOORunner::LaneWidth)<110.f && Height<160.f)
        {
            LastBoosterTile = I;
            ++BoostersCollected;
            BoostRemaining = HOORunner::BoostDuration;
            ChainRemaining = 2.5f;
            ChargeFever(HOORunner::BoosterCharge);
        }
        if (I % 2 == 1 && I > LastCollectedTile && Along < 120.0
            && (IsFever() || (FMath::Abs(Lateral - Tile.SafeLane * HOORunner::LaneWidth) < 105.0f && Height < 140.0f)))
        {
            ++Coins; LastCollectedTile = I;
            ++Chain; BestChain = FMath::Max(BestChain, Chain);
            ChainRemaining = 2.5f;
            ChargeFever(1);
            PickupScore += 20 * Multiplier();
        }
    }
}
