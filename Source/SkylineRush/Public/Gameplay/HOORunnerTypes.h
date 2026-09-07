#pragma once

#include "CoreMinimal.h"

enum class EHOORunnerPhase : uint8 { Ready, Running, Paused, Crashed };
enum class EHOORunnerHazard : uint8 { None, Barrier, Overhead, Gap };
enum class EHOORunnerTheme : uint8 { City, Climb, Dive, Underwater, JumpIsles, Loop, Launch, Flight, Clouds };

struct FHOORunnerTile
{
    int64 Index = 0;
    int32 SafeLane = 0;
    bool bBooster = false;
    bool bLaunchPad = false;
    bool bFlightGap = false;
    bool bJumpRow = false;
    int32 BoosterLane = 0;
    EHOORunnerHazard Lanes[3] = {};
};

namespace HOORunner
{
    constexpr double TileLength = 600.0;
    constexpr float LaneWidth = 300.0f;
    constexpr double FixedStep = 1.0 / 120.0;
    constexpr float InitialSpeed = 1400.0f;
    constexpr float MaxSpeed = 3800.0f;
    constexpr float AccelerationPerCm = .05f;
    constexpr float JumpVelocity = 1120.0f;
    constexpr float Gravity = 2800.0f;
    constexpr float SlideDuration = 0.82f;
    constexpr int32 VisibleTiles = 80;
    constexpr int32 MaxLives = 3;
    constexpr float RecoveryDuration = 2.5f;
    constexpr float HitSlowDuration = 1.2f;
    constexpr int32 FeverThreshold = 24;
    constexpr int32 BoosterCharge = 6;
    constexpr float FeverDuration = 8.f;
    constexpr float BoostDuration = 4.f;
    constexpr float PowerSpeedBonus = .25f;
    constexpr double ThemeCycleLength = 300000.;

    int32 Difficulty(double DistanceCm);
    float Speed(double DistanceCm);
    FHOORunnerTile Tile(int64 Index, int32 Seed);
    EHOORunnerTheme Theme(double DistanceCm);
    const TCHAR* ThemeName(EHOORunnerTheme Theme);
    float TurnPreview(double S);
    FVector Center(double DistanceCm);
    FTransform Frame(double DistanceCm);
    FRotator Heading(double DistanceCm);
    const TCHAR* HazardName(EHOORunnerHazard Hazard);
}

/** All gameplay collision takes place in track coordinates; rendering uses the same Tile(). */
struct FHOORunnerState
{
    EHOORunnerPhase Phase = EHOORunnerPhase::Ready;
    double Distance = 0.0;
    float CurrentSpeed = HOORunner::InitialSpeed;
    float Lateral = 0.0f;
    int32 TargetLane = 0;
    float Height = 0.0f;
    float VerticalSpeed = 0.0f;
    float SlideRemaining = 0.0f;
    float JumpBuffer = 0.0f;
    bool bSlideOnLanding = false;
    int32 Lives = HOORunner::MaxLives;
    int32 Hits = 0;
    float RecoveryRemaining = 0;
    float HitSlowRemaining = 0;
    EHOORunnerHazard LastHit = EHOORunnerHazard::None;
    int32 Coins = 0;
    int32 Chain = 0;
    int32 BestChain = 0;
    int32 PickupScore = 0;
    float ChainRemaining = 0;
    double Elapsed = 0;
    int32 SimulationTicks = 0;
    int32 FeverCharge = 0;
    int32 FeverActivations = 0;
    int32 BoostersCollected = 0;
    float FeverRemaining = 0;
    float BoostRemaining = 0;
    float PowerSpeedAlpha = 0;
    int64 LastBoosterTile = -1;
    int64 ProtectedThroughTile = -1;
    int32 Launches = 0;
    int64 LastLaunchCycle = -1;
    double FlightStart = -1;
    double FlightEnd = -1;
    int32 Seed = 409;
    int64 LastCollectedTile = -1;
    double Accumulator = 0.0;
    EHOORunnerHazard Failure = EHOORunnerHazard::None;

    void Reset(int32 InSeed);
    void Start();
    void Move(int32 Direction);
    void Jump();
    void Slide();
    void TogglePause();
    void Advance(float DeltaSeconds);
    void Step(float DeltaSeconds);
    bool IsSliding() const { return SlideRemaining > 0.0f && Height < 1.0f; }
    bool IsFlying() const { return FlightStart>=0 && Distance<FlightEnd; }
    bool IsFever() const { return FeverRemaining > 0; }
    bool IsBoosting() const { return BoostRemaining > 0; }
    void ChargeFever(int32 Amount);
    int32 Multiplier() const { return (1 + FMath::Min(3, Chain / 10)) * (IsFever() ? 2 : 1); }
    int32 Score() const { return FMath::FloorToInt(Distance / 100.0) + PickupScore; }
};
