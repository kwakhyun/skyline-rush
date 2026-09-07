#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HOORunnerWorlds.generated.h"

// Presentation only: never serialize this enum into a competitive replay.
enum class EHOORunnerBiome : uint8 { Forest, City, Interior, Jungle, Dimension };
namespace HOORunnerWorld
{
    EHOORunnerBiome Biome(double Distance);
    const TCHAR* Name(EHOORunnerBiome Value);
    double Remaining(double Distance);
    float Progress(double Distance);
}
class UInstancedStaticMeshComponent;
UCLASS()
class SKYLINERUSH_API AHOORunnerWorlds : public AActor
{
    GENERATED_BODY()
public:
    AHOORunnerWorlds();
    void Present(double Distance);
private:
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Pools;
    TArray<int64> Slots;
    TArray<int32> Counts;
    void Place(int32 Slot,int64 Tile);
};
