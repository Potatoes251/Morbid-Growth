#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridBuilder.generated.h"

UCLASS()
class CHEDDAR_API AGridBuilder : public AActor
{
	GENERATED_BODY()
	
public:	
	AGridBuilder();

    void AddInfectedTile(class ATile* Tile);
    void RemoveInfectedTile(class ATile* Tile);
    void DrawSpreadSpeedFunction();
    TArray<class AChunk*> TargetBoxes;

    UPROPERTY(EditDefaultsOnly)
    UInstancedStaticMeshComponent* SphereMeshInstance;
    UInstancedStaticMeshComponent* SphereMeshInstanceEye;

    int32 InfectedTileCount = 0;
    float InfectionSpeedMultiplier = 1.f;

    UPROPERTY(EditDefaultsOnly)
    float SpreadSpeedMultiplier = .2f;

    //UPROPERTY(EditAnywhere, Category = "Fog")
    // 
    // 
    //float FogStartingDensity = 0.1f; 


    UPROPERTY(EditAnywhere, Category = "Fog")
    int32 FogStageDifference = 2000;

    UPROPERTY(EditAnywhere, Category = "Fog")
    float MaxFogDensity = 0.7f;

    UPROPERTY(EditAnywhere, Category = "Fog")
    float MinFogDensity = 0.00f;

    UPROPERTY(EditAnywhere, Category = "Fog")
    float FogSaturationSpeed = 0.005f; // how fast the fog builds

    UPROPERTY(EditAnywhere, Category = "Fog")
    float InfectionThreshold = 0.1f;

    UPROPERTY(EditAnywhere, Category = "Fog")
    float FogLerpSpeed = 1.5f;

    float TargetFogDensity = 0.f;

    UPROPERTY(EditAnywhere, Category = "Fog")
    int32 MaxFogStages = 10;

    int32 LastFogStage = 0;


protected:
	virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    void UpdateFog();

private:
    void BuildGrid();
    void AssignNeighbors();
    void AssignNeighborsInChunk(AChunk* Chunk, class ATile* Tile);

    void SpawnTileIfHit(const FVector& Start, const FVector& Dir, int32 X, int32 Y);

    bool IsInsideSpreadableVolume(const FVector& Point, class AChunk*& OutChunk);
    bool IsInsideStartingVolume(const FVector& Point);

    bool IsInsidePillar(const FVector& Point);
    bool IsInsidePillarBase(const FVector& Point, AChunk*& OutChunk);

    void CacheSpreadableVolumes();
    
    void DestroyPillars();

    UPROPERTY(EditAnywhere, Category = "Infection")
    UMaterialInterface* BaseMaterial;

    UPROPERTY(EditAnywhere, Category = "Infection")
    UMaterialInterface* EyeMaterial;

    UPROPERTY(EditAnywhere)
    float CellSize = 200.0f;

    UPROPERTY(EditAnywhere)
    int32 GridX = 50;

    UPROPERTY(EditAnywhere)
    int32 GridY = 50;
    
    UPROPERTY(EditAnywhere)
    int32 GridZ = 50;

    UPROPERTY(EditAnywhere)
    float RayLength = 3000.0f;

    UPROPERTY(EditAnywhere)
    TSubclassOf<class ATile> TileClass;

    TArray<class ATile*> Tiles;
    TSet<class ATile*> InfectedTiles;
    TMap<FIntVector, ATile*> TileMap;
    TArray<class AChunk*> SpreadableBoxes;
    TArray<class AChunk*> StartingBoxes;
    TArray<class AChunk*> Pillars;
    TArray<class AChunk*> PillarBases;

    class AExponentialHeightFog* FogActor;

};
