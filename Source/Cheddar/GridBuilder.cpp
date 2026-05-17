#include "GridBuilder.h"
#include "Tile.h"
#include "Chunk.h"

#include "Engine/World.h"
#include "EngineUtils.h" 
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"

AGridBuilder::AGridBuilder()
{
    PrimaryActorTick.bCanEverTick = true;

    SphereMeshInstance = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISM"));
    SphereMeshInstance->SetupAttachment(RootComponent);

    SphereMeshInstanceEye = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ISM_Eye"));
    SphereMeshInstanceEye->SetupAttachment(RootComponent);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshObj(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (MeshObj.Succeeded())
    {
        SphereMeshInstance->SetStaticMesh(MeshObj.Object);
        SphereMeshInstance->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SphereMeshInstance->SetCastShadow(false);

        SphereMeshInstanceEye->SetStaticMesh(MeshObj.Object);
        SphereMeshInstanceEye->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SphereMeshInstanceEye->SetCastShadow(false);
    }
}

void AGridBuilder::AddInfectedTile(ATile* Tile)
{
    InfectedTiles.Add(Tile);
    InfectionSpeedMultiplier = 1.0f + SpreadSpeedMultiplier * FMath::Loge((float)(InfectedTiles.Num() + 1));

    int32 CurrentStage = InfectedTiles.Num() / FogStageDifference;

    if (CurrentStage != LastFogStage)
    {
        LastFogStage = CurrentStage;
        UpdateFog();
    }
}

void AGridBuilder::RemoveInfectedTile(ATile* Tile)
{
    InfectedTiles.Remove(Tile);
    InfectionSpeedMultiplier = 1.0f + SpreadSpeedMultiplier * FMath::Loge((float)(InfectedTiles.Num() + 1));

    int32 CurrentStage = InfectedTiles.Num() / FogStageDifference;

    if (CurrentStage != LastFogStage)
    {
        LastFogStage = CurrentStage;
        UpdateFog();
    }
}

void AGridBuilder::BeginPlay()
{
    Super::BeginPlay();
    double StartTime = FPlatformTime::Seconds();
    UMaterialInstanceDynamic* LocalDynMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    SphereMeshInstance->SetMaterial(0, LocalDynMat);

    LocalDynMat = UMaterialInstanceDynamic::Create(EyeMaterial, this);
    SphereMeshInstanceEye->SetMaterial(0, LocalDynMat);

    CacheSpreadableVolumes();
    BuildGrid();
    AssignNeighbors();
    DestroyPillars();

    for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
    {
        FogActor = *It;
        break; // use the first fog found
    }
}

void AGridBuilder::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!FogActor) return;

    UExponentialHeightFogComponent* FogComp = FogActor->GetComponentByClass<UExponentialHeightFogComponent>();

    if (!FogComp) return;

    float CurrentDensity = FogComp->FogDensity;

    float NewDensity = FMath::FInterpTo(CurrentDensity, TargetFogDensity, DeltaTime, FogLerpSpeed);

    FogComp->SetFogDensity(NewDensity);

}

void AGridBuilder::UpdateFog()
{
    if (!FogActor) return;

    float InfectionValue = (float)InfectedTiles.Num() * FogSaturationSpeed;
    float InfectionRatio = FMath::Clamp(InfectionValue, 0.f, 1.f);

    TargetFogDensity = FMath::Lerp(MinFogDensity, MaxFogDensity, InfectionRatio);
}

void AGridBuilder::BuildGrid()
{
    Tiles.Empty();

    FVector Origin = GetActorLocation();

    static const TArray<FVector> Directions =
    {
        FVector::DownVector,
        FVector::UpVector,
        FVector::XAxisVector,
        -FVector::XAxisVector,
        FVector::YAxisVector,
        -FVector::YAxisVector
    };

    for (int32 Z = 0; Z < GridZ; Z++)
    {
        for (int32 X = 0; X < GridX; X++)
        {
            for (int32 Y = 0; Y < GridY; Y++)
            {
                FVector BasePos = Origin;
                BasePos.X += X * CellSize;
                BasePos.Y += Y * CellSize;
                BasePos.Z += Z * CellSize;

                for (const FVector& Dir : Directions)
                {
                    SpawnTileIfHit(BasePos, Dir, X, Y);
                }
            }
        }
    }
}

void AGridBuilder::SpawnTileIfHit(const FVector& Start, const FVector& Dir, int32 X, int32 Y)
{
    FVector End = Start + Dir * RayLength;

    FHitResult Hit;

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);

    if (!bHit)
    {
        return;
    }
  
    FVector SpawnLoc = Hit.ImpactPoint;

    AChunk* Chunk = nullptr;
    if (!IsInsideSpreadableVolume(SpawnLoc, Chunk))
    {
        return; // not allowed to spawn here
    }

    FIntVector Key = FIntVector(
        FMath::RoundToInt(SpawnLoc.X / CellSize), 
        FMath::RoundToInt(SpawnLoc.Y / CellSize), 
        FMath::RoundToInt(SpawnLoc.Z / CellSize));

    if (TileMap.Contains(Key))
        return;

    ATile* Tile = GetWorld()->SpawnActor<ATile>(TileClass, SpawnLoc, FRotator::ZeroRotator);

    if (!Tile)
        return;

    TileMap.Add(Key, Tile);

    Chunk->AddTile(Tile);
    Tile->SetOwningChunk(Chunk);
    Tile->AddGridBuilder(this);
    if (Hit.GetActor() && Hit.GetActor()->ActorHasTag("Pillar"))
        Tile->SetOnPillar(true);
    if (IsInsidePillarBase(SpawnLoc, Chunk))
    {
        Tile->SetPillarBase(Chunk);
        Chunk->AddTile(Tile);
    }

    Tile->SurfaceNormal = Hit.ImpactNormal; 
    Tiles.Add(Tile);

    if (IsInsideStartingVolume(Hit.ImpactPoint))
        Tile->InfectTile();
}

bool AGridBuilder::IsInsideSpreadableVolume(const FVector& Point, AChunk*& OutChunk)
{
    for (AChunk* Chunk : SpreadableBoxes)
    {
        if (Chunk->IsInside(Point))
        {
            OutChunk = Chunk;
            return true;
        }
    }
    return false;
}

bool AGridBuilder::IsInsideStartingVolume(const FVector& Point)
{
    for (AChunk* Chunk : StartingBoxes)
    {
        if (Chunk->IsInside(Point))
        {
            return true;
        }
    }
    return false;
}

bool AGridBuilder::IsInsidePillar(const FVector& Point)
{
    for (AChunk* Chunk : Pillars)
    {
        if (Chunk->IsInside(Point))
        {
            return true;
        }
    }
    return false;
}

bool AGridBuilder::IsInsidePillarBase(const FVector& Point, AChunk*& OutChunk)
{
    for (AChunk* Chunk : PillarBases)
    {
        if (Chunk->IsInside(Point))
        {
            return true;
        }
    }
    return false;
}

void AGridBuilder::CacheSpreadableVolumes()
{
    SpreadableBoxes.Empty();
    StartingBoxes.Empty();
    TargetBoxes.Empty();

    for (TActorIterator<AChunk> It(GetWorld()); It; ++It)
    {
        AChunk* Chunk = *It;
        
        if (!Chunk || !Chunk->ChunkBounds)
            continue;
        Chunk->InitBounds();
        if (Chunk->ActorHasTag("Spreadable")) 
        {
            SpreadableBoxes.Add(Chunk);
        }
        else if (Chunk->ActorHasTag("Starting")) 
        {
            StartingBoxes.Add(Chunk);
        }
        else if (Chunk->ActorHasTag("Target"))
        {
            TargetBoxes.Add(Chunk);
        }
        else if (Chunk->ActorHasTag("Pillar"))
        {
            Pillars.Add(Chunk);
        }
        else if (Chunk->ActorHasTag("PillarBase"))
        {
            PillarBases.Add(Chunk);
        }
    }

    for (int32 I = 0; I < SpreadableBoxes.Num(); I++)
    {
        for (int32 J = I + 1; J < SpreadableBoxes.Num(); J++)
        {
            AChunk* A = SpreadableBoxes[I];
            AChunk* B = SpreadableBoxes[J];

            if (!A || !A->Intersect(B))
                continue;

            A->AddChunk(B);
            B->AddChunk(A);
        }
    }
}

void AGridBuilder::DestroyPillars()
{
    for (AChunk* Pillar : Pillars)
    {
        Pillar->Destroy();
    }
    Pillars.Empty();
}

void AGridBuilder::AssignNeighbors()
{
    const float MaxDist = CellSize * 1.5f;

    for (ATile* Tile : Tiles)
    {
        AChunk* Chunk = Tile->GetOwningChunk();
        if (!Tile)
            continue;

        FVector Pos = Tile->GetActorLocation();

        AssignNeighborsInChunk(Chunk, Tile);

        for (AChunk* C : Chunk->NeighborChunk)
        {
            AssignNeighborsInChunk(C, Tile);
        }
    }
}

void AGridBuilder::AssignNeighborsInChunk(AChunk* Chunk, ATile* Tile)
{
    // MaxDist =  CellSize * 1.5f
    const float MaxDistSquared = CellSize * CellSize * 2.25f;
    FVector Pos = Tile->GetActorLocation();

    for (ATile* Other : Chunk->ChunkTiles)
    {
        if (!Other || Other == Tile)
            continue;

        FVector Delta = Other->GetActorLocation() - Pos;

        float Dist = Delta.SizeSquared();
        if (Dist > MaxDistSquared)
            continue;

        Tile->NeighborTiles.Add(Other);
        if (!Other->IsOnPillar())
        {
            Tile->InfectableTiles.Add(Other);
        }
    }
}
