#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/Box.h"
#include "Chunk.generated.h"

UCLASS(Blueprintable)
class CHEDDAR_API AChunk : public AActor
{
	GENERATED_BODY()
	
public:
	AChunk();

	virtual void BeginPlay() override;

	void InitBounds();
	void AddTile(class ATile* Tile);
	void AddChunk(AChunk* Chunk);

	void IncreaseInfectionCount(class ATile* Tile);
	void DecreaseInfectionCount(class ATile* Tile);
	bool IsInside(ATile* Tile) const;
	bool IsInside(FVector Point) const;
	bool Intersect(AChunk* Chunk) const;

	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UBoxComponent* ChunkBounds;
	FBox CachedBounds;

	int32 PlayerInsideCount = 0;
	int32 PlayerInsideNeighborCount = 0;

	UPROPERTY(EditAnywhere)
	int32 InfectedTileThreshold;

	TArray<class ATile*> ChunkTiles;
	TSet<ATile*> InfectedTiles;
	TArray<AChunk*> NeighborChunk;

private:
	void NotifyNeighborEntered();
	void NotifyNeighborExited();
	void EnableTiles();
	void DisableTiles();

	int32 InfectedTileCount = 0;
	bool bIsPillarBase = false;
};
