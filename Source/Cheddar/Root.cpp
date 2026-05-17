#include "Root.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

ARoot::ARoot()
{
    PrimaryActorTick.bCanEverTick = false;

    SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
    SetRootComponent(SphereMesh);

    // Make sure it sticks to surfaces
    SphereMesh->SetMobility(EComponentMobility::Static);
}

void ARoot::BeginPlay()
{
    Super::BeginPlay();

    // Schedule spreading attempts
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &ARoot::AttemptSpread, SpreadDelay, true);
}

void ARoot::AttemptSpread()
{
    if (ChildrenSpawned >= MaxChildren) return;
    if (!SphereClass) return;

    FVector Origin = GetActorLocation();
    FVector Normal = SphereMesh->GetUpVector(); // "Up" = Normal of surface

    // Random direction tangent to the surface
    FVector RandomDir = UKismetMathLibrary::RandomUnitVector();
    RandomDir = RandomDir - FVector::DotProduct(RandomDir, Normal) * Normal;
    RandomDir.Normalize();

    FVector Start = Origin + Normal * 5.f; // move slightly off the surface
    FVector End = Start + RandomDir * SpreadRange;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_WorldStatic,
        Params
    );

    if (bHit)
    {
        // Spawn sphere at surface hit
        FVector Location = Hit.ImpactPoint;
        FVector SurfaceNormal = Hit.ImpactNormal;

        // Orient sphere so its "up" aligns with surface
        FRotator Rotation = UKismetMathLibrary::MakeRotFromZ(SurfaceNormal);

        GetWorld()->SpawnActor<ARoot>(SphereClass, Location, Rotation);

        ChildrenSpawned++;
    }
}

