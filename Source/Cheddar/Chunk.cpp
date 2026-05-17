#include "Chunk.h"

#include "Tile.h"

#include "CheddarCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Math/Box.h"



AChunk::AChunk()
{
    PrimaryActorTick.bCanEverTick = false;

    ChunkBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ChunkBounds"));
    SetRootComponent(ChunkBounds);

    ChunkBounds->SetBoxExtent(FVector(350.0f));
    ChunkBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ChunkBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    ChunkBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AChunk::BeginPlay()
{
    Super::BeginPlay();

    OnActorBeginOverlap.AddDynamic(this, &AChunk::OnOverlapBegin);
    OnActorEndOverlap.AddDynamic(this, &AChunk::OnOverlapEnd);
}

void AChunk::InitBounds()
{
    CachedBounds = ChunkBounds->Bounds.GetBox();
}

void AChunk::AddTile(ATile* Tile)
{
	ChunkTiles.Add(Tile);
}

void AChunk::AddChunk(AChunk* Chunk)
{
	NeighborChunk.Add(Chunk);
}

void AChunk::IncreaseInfectionCount(ATile* Tile)
{
    int32 PreviousCount = InfectedTiles.Num();
    InfectedTiles.Add(Tile);
    int32 NewCount = InfectedTiles.Num();
    if (NewCount >= InfectedTileThreshold && PreviousCount < InfectedTileThreshold)
    {
        for (ATile* ChunkTile : ChunkTiles)
        {
            ChunkTile->AddInNeighborInfectable();
        }
    }
}

void AChunk::DecreaseInfectionCount(ATile* Tile)
{
    int32 PreviousCount = InfectedTiles.Num();
    InfectedTiles.Remove(Tile);
    int32 NewCount = InfectedTiles.Num();
    if (NewCount < InfectedTileThreshold && PreviousCount >= InfectedTileThreshold)
    {
        for (ATile* ChunkTile : ChunkTiles)
        {
            if (ChunkTile->IsOnPillar())
                ChunkTile->RemoveFromNeighborInfectable();
        }
    }
}

bool AChunk::IsInside(ATile* Tile) const
{
    const FVector Point = Tile->GetActorLocation();

    return CachedBounds.IsInside(Point);
}

bool AChunk::IsInside(FVector Point) const
{
    return CachedBounds.IsInside(Point);
}

bool AChunk::Intersect(AChunk* Chunk) const
{
    return CachedBounds.Intersect(Chunk->CachedBounds);
}

void AChunk::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
    if (!Cast<ACheddarCharacter>(OtherActor))
        return;

    PlayerInsideCount++;
    if (PlayerInsideNeighborCount > 0 || PlayerInsideCount > 0)
        EnableTiles();

    for (AChunk* Chunk : NeighborChunk)
    {
        Chunk->NotifyNeighborEntered();
    }
}

void AChunk::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
    if (!Cast<ACheddarCharacter>(OtherActor))
        return;

    PlayerInsideCount--;

    if (PlayerInsideNeighborCount <= 0 && PlayerInsideCount <= 0)
        DisableTiles();

    for (AChunk* Chunk : NeighborChunk)
    {
        Chunk->NotifyNeighborExited();
    }
}

void AChunk::NotifyNeighborEntered()
{
    PlayerInsideNeighborCount++;
    if (PlayerInsideNeighborCount > 0 || PlayerInsideCount > 0)
        EnableTiles();
}

void AChunk::NotifyNeighborExited()
{
    PlayerInsideNeighborCount--;
    if (PlayerInsideNeighborCount <= 0 && PlayerInsideCount <= 0)
        DisableTiles();
}

void AChunk::EnableTiles()
{
    for (ATile* Tile : ChunkTiles)
    {
        if (Tile && Tile->SphereMesh)
        {
            Tile->SphereMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        }
    }
}


void AChunk::DisableTiles()
{
    for (ATile* Tile : ChunkTiles)
    {
        if (Tile && Tile->SphereMesh)
        {
            Tile->SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
}