#include "Tile.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include <Engine/DamageEvents.h>
#include "TimerManager.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include <Components/SphereComponent.h>
#include "CheddarCharacter.h"

#include "Life_AC.h"
#include "Chunk.h"
#include "GridBuilder.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);
	TendrilSpline = CreateDefaultSubobject<USplineComponent>(TEXT("TendrilSpline"));
	TendrilSpline->SetupAttachment(RootComponent);
}

void ATile::BeginPlay()
{
	Super::BeginPlay();

	InfectableTiles.Add(this);
	InfectionInitialTimer = FMath::RandRange(RandInfectionInitialTimerMin, RandInfectionInitialTimerMax);
}

void ATile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (SphereInstanceIndex == INDEX_NONE)
	{
		SetActorTickEnabled(false);
		return;
	}
	const FVector CurrentScale = CachedInstanceScale;

	const FVector TargetScale = FVector(FMath::Max(SMALL_NUMBER, FMath::Min(3, InfectionLevel) * InfectionSize));

	const FVector NewScale = FMath::VInterpTo(CurrentScale, TargetScale, DeltaSeconds, 5.0f);

	FTransform InstTransform;
	GridBuilder->SphereMeshInstance->GetInstanceTransform(SphereInstanceIndex, InstTransform, true);

	InstTransform.SetScale3D(NewScale);
	CachedInstanceScale = NewScale;
	GridBuilder->SphereMeshInstance->UpdateInstanceTransform(SphereInstanceIndex, InstTransform, true, true);

	if (bIsAnEye && EyeIndex != INDEX_NONE)
	{
		FTransform EyeTransform;
		GridBuilder->SphereMeshInstanceEye->GetInstanceTransform(EyeIndex, EyeTransform, true);
		// Apply only the scale from the sphere
		EyeTransform.SetScale3D(NewScale);

		GridBuilder->SphereMeshInstanceEye->UpdateInstanceTransform(EyeIndex, EyeTransform, true, true);
	}


	if (FMath::Abs(TargetScale.X - NewScale.X) < SMALL_NUMBER)
	{
		if (InfectionLevel == 0)
		{
			FVector FarAwayLocation(100000.f, 100000.f, 100000.f);
			TeleportMesh(GridBuilder->SphereMeshInstance, FarAwayLocation, SphereInstanceIndex);
		}

		SetActorTickEnabled(false);
	}
}

void ATile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorldTimerManager().ClearTimer(TimerHandle);
}

void ATile::InfectTile()
{
	UpdateInfectionLevel(1);
}

void ATile::TryToInfect()
{
	if (bInBurnCooldown)
		return;

	if (InfectableTiles.IsEmpty())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
		return;
	}
	if (bInPlayerRange)
	{
		ATile* Target = FindClosestToTarget(GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation());
		Target->InfectionInitialTimer = InfectionChaseSpeed;
		Target->InfectTile();
		return;
	}
	if (!CurrentTargetChunk.IsValid())
		SetNewTargetChunk();
	if (CurrentTargetChunk.IsValid())
	{
		ATile* Target = FindClosestToTarget(CurrentTargetLocation);
		if (CurrentTargetChunk.IsValid() && CurrentTargetChunk->IsInside(Target))
		{
			CurrentTargetChunk->Destroy();
			CurrentTargetChunk = nullptr;
			SetNewTargetChunk();
		}
		Target->InfectTile();
		return;
	}
	ATile* Target = InfectableTiles[FMath::RandRange(0, InfectableTiles.Num() - 1)];
	Target->InfectTile();
}

void ATile::Burn()
{
	ApplyBurnCooldown();

	if (InfectionLevel == EyeChanceLevelTreshold && bIsAnEye)
	{
		ApplyInfectionMesh();
	}

	// Reduce infection only on the burned tile
	UpdateInfectionLevel(-1);

	// Apply cooldown to neighbors
	for (ATile* Neighbor : NeighborTiles)
	{
		if (IsValid(Neighbor))
		{
			Neighbor->ApplyBurnCooldown();
		}
	}
}

void ATile::ApplyBurnCooldown()
{
	bInBurnCooldown = true;

	// Stop its infection timer
	GetWorldTimerManager().ClearTimer(TimerHandle);

	// Start the burn cooldown
	GetWorldTimerManager().SetTimer(
		BurnCooldownHandle,
		this,
		&ATile::EndBurnCooldown,
		BurnCooldownDuration,
		false
	);
}

void ATile::EndBurnCooldown()
{
	bInBurnCooldown = false;

	// If infection level is still 1, restart infecting
	if (InfectionLevel >= 1)
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(TimerHandle))
		{
			SetInfectTimer();
		}
	}
}

void ATile::SetOwningChunk(AChunk* Chunk)
{
	OwningChunk = Chunk;
}

void ATile::SetPillarBase(AChunk* Chunk)
{
	PillarBase = Chunk;
}

void ATile::AddGridBuilder(AGridBuilder* Gb)
{
	GridBuilder = Gb;
}

AChunk* ATile::GetOwningChunk()
{
	return OwningChunk.Get();
}

ATile* ATile::FindClosestToTarget(FVector Target)
{
	ATile* Closest = nullptr;
	float ShortestDistance = INFINITY;
	for (ATile* Tile : InfectableTiles)
	{
		if (!Closest && Tile)
		{
			Closest = Tile;
			continue;
		}
		float Distance = FVector::DistSquared(Target, Tile->GetActorLocation());
		if (Distance < ShortestDistance)
		{
			ShortestDistance = Distance;
			Closest = Tile;
		}
	}
	return Closest;
}

void ATile::SetNewTargetChunk()
{
	if (!GridBuilder || GridBuilder->TargetBoxes.IsEmpty())
		return;

	float ShortestDistance = MaxTargetDistanceSquared;
	for (int32 I = GridBuilder->TargetBoxes.Num() - 1; I >= 0; --I)
	{
		AChunk* TargetChunk = GridBuilder->TargetBoxes[I];

		if (!IsValid(TargetChunk))
		{
			GridBuilder->TargetBoxes.RemoveAtSwap(I);
			continue;
		}
		float Distance = FVector::DistSquared(GetActorLocation(), TargetChunk->GetActorLocation());
		if (Distance < ShortestDistance)
		{
			ShortestDistance = Distance;
			CurrentTargetChunk = TargetChunk;
			CurrentTargetLocation = TargetChunk->GetActorLocation();
		}
	}
}

void ATile::SetOnPillar(bool b)
{
	bOnPillar = b;
}

bool ATile::IsOnPillar() const
{
	return bOnPillar;
}

void ATile::RemoveFromNeighborInfectable()
{
	TArray<ATile*> NeighborsCopy = NeighborTiles;
	InfectableTiles.RemoveSwap(this);
	for (ATile* Tile : NeighborsCopy)
	{
		if (!IsValid(Tile))
		{
			continue;
		}
		Tile->InfectableTiles.RemoveSwap(this);
	}
}

void ATile::AddInNeighborInfectable()
{
	TArray<ATile*> NeighborsCopy = NeighborTiles;
	InfectableTiles.AddUnique(this);
	for (ATile* Tile : NeighborsCopy)
	{
		if (IsValid(Tile))
			Tile->InfectableTiles.AddUnique(this);
	}
}

void ATile::UpdateInfectionLevel(int32 IncreaseAmount)
{
	if (!IsValid(this))
	{
		return;
	}

	InfectionLevel = FMath::Clamp((InfectionLevel + IncreaseAmount), 0, MaxInfectionLevel);
	SetActorTickEnabled(true);

	if (bIsAnEye && InfectionLevel == EyeChanceLevelTreshold)
	{
		ApplyEyeMesh();
	}

	if (!SphereMesh && SphereInstanceIndex == INDEX_NONE)
	{
		// Random offset for the sphere
		FVector RandomOffset;
		RandomOffset.X = FMath::RandRange(-OffsetRange, OffsetRange);
		RandomOffset.Y = FMath::RandRange(-OffsetRange, OffsetRange);
		RandomOffset.Z = FMath::RandRange(-OffsetRange, OffsetRange);
		SpawnSphereMeshComponent(InfectionSize * InfectionLevel, RandomOffset);

		InfectionTimer = FMath::RandRange(InfectionTimerMin, InfectionTimerMax);
		MaxInfectionLevel = FMath::RandRange(RandMinInfectionLevel, RandMaxInfectionLevel);
		InfectionSize = InfectionSize + FMath::RandRange(RandMinSize, RandMaxSize);

		// Chance to be an eye if its max level is equal or above the level treshold
		if (MaxInfectionLevel >= EyeChanceLevelTreshold && FMath::RandRange(1, RandEyeChance) == 1)
		{
			bIsAnEye = true;
		}
	}

	SphereMesh->SetSphereRadius(FMath::Max(SMALL_NUMBER, FMath::Min(3, InfectionLevel) * InfectionSize * 50));

	if (InfectionLevel == 0)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
		if (GridBuilder && IncreaseAmount < 0)
		{
			GridBuilder->RemoveInfectedTile(this);
			if (PillarBase.IsValid())
				PillarBase->DecreaseInfectionCount(this);
		}
		SphereMesh->SetGenerateOverlapEvents(false);
		SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else if (InfectionLevel == 1)
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(TimerHandle))
			SetInfectTimer();
		if (GridBuilder && IncreaseAmount > 0)
		{
			TeleportMesh(GridBuilder->SphereMeshInstance, WorldLocation, SphereInstanceIndex);
			GridBuilder->AddInfectedTile(this);
			SphereMesh->SetGenerateOverlapEvents(true);
			SphereMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			if (PillarBase.IsValid())
				PillarBase->IncreaseInfectionCount(this);
		}
	}
	else if (InfectionLevel == MaxInfectionLevel - 1)
	{
		AddInNeighborInfectable();
	}
	else if (InfectionLevel == MaxInfectionLevel)
	{
		RemoveFromNeighborInfectable();
	}
}

void ATile::SetInfectTimer()
{
	if (GridBuilder)
	{
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ATile::TryToInfect, InfectionTimer * GridBuilder->InfectionSpeedMultiplier, true, InfectionInitialTimer * GridBuilder->InfectionSpeedMultiplier);
		return;
	}
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ATile::TryToInfect, InfectionTimer, true, InfectionInitialTimer);
}

void ATile::TeleportMesh(UInstancedStaticMeshComponent* Mesh, FVector NewLocation, int32 Index)
{
	FTransform CurrentTransform;
	Mesh->GetInstanceTransform(Index, CurrentTransform, true);

	// Keep the scale and rotation!
	CurrentTransform.SetLocation(NewLocation);

	Mesh->UpdateInstanceTransform(Index, CurrentTransform, true, true);
}

void ATile::ApplyEyeMesh()
{
	if (!GridBuilder || SphereInstanceIndex == INDEX_NONE)
		return;

	FTransform OldTransform;
	GridBuilder->SphereMeshInstance->GetInstanceTransform(SphereInstanceIndex, OldTransform, true);

	FVector FarAwayLocation(100000.f, 100000.f, 100000.f);
	TeleportMesh(GridBuilder->SphereMeshInstance, FarAwayLocation, SphereInstanceIndex);

	FTransform EyeTransform;
	EyeTransform.SetLocation(OldTransform.GetLocation());
	EyeTransform.SetRotation(OldTransform.GetRotation());
	EyeTransform.SetScale3D(CachedInstanceScale); // use the same scale as the sphere

	if (EyeIndex == INDEX_NONE)
	{
		EyeIndex = GridBuilder->SphereMeshInstanceEye->AddInstance(EyeTransform);
	}
	else
	{
		GridBuilder->SphereMeshInstanceEye->UpdateInstanceTransform(EyeIndex, EyeTransform, true, true);
	}
}

void ATile::ApplyInfectionMesh()
{
	if (!GridBuilder || EyeIndex == INDEX_NONE)
		return;

	FTransform OldTransform;
	GridBuilder->SphereMeshInstanceEye->GetInstanceTransform(EyeIndex, OldTransform, true);

	FVector FarAwayLocation(100000.f, 100000.f, 100000.f);
	TeleportMesh(GridBuilder->SphereMeshInstanceEye, FarAwayLocation, EyeIndex);

	FTransform InfectionTransform;
	InfectionTransform.SetLocation(OldTransform.GetLocation());
	InfectionTransform.SetRotation(OldTransform.GetRotation());
	InfectionTransform.SetScale3D(FVector(InfectionSize * InfectionLevel)); // use the same scale as the sphere

	GridBuilder->SphereMeshInstance->UpdateInstanceTransform(SphereInstanceIndex, InfectionTransform, true, true);
}

void ATile::SpawnSphereMeshComponent(float Scale, const FVector& Offset)
{
	if (!SurfaceNormal.IsNearlyZero())
	{
		// Make tile face outward along the surface normal
		SurfaceRotation = SurfaceNormal.Rotation();

		// Apply the rotation to the actor
		SetActorRotation(SurfaceRotation);
	}
	// Remove surface normal component from the offset
	FVector PlanarOffset = Offset - FVector::DotProduct(Offset, SurfaceNormal) * SurfaceNormal;

	FTransform Transform;
	Transform.SetLocation(PlanarOffset + GetActorLocation());
	Transform.SetRotation(GetActorRotation().Quaternion());

	CachedInstanceScale = FVector(.1f);
	Transform.SetScale3D(CachedInstanceScale);
	WorldLocation = GetActorLocation()+ Offset;
	SphereInstanceIndex = GridBuilder->SphereMeshInstance->AddInstance(Transform);

	USphereComponent* SphereCollider = NewObject<USphereComponent>(this);
	SphereCollider->RegisterComponent();
	SphereCollider->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	SphereCollider->SetGenerateOverlapEvents(true);

	SphereCollider->SetHiddenInGame(true);
	SphereCollider->SetVisibility(false);
	SphereCollider->SetSphereRadius(Scale);

	SphereCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollider->SetCollisionObjectType(ECC_WorldDynamic);
	SphereCollider->SetCollisionResponseToAllChannels(ECR_Overlap);
	SphereCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	SphereCollider->SetWorldLocation(Offset + GetActorLocation());

	SphereMesh = SphereCollider;
}

void ATile::StartAttacking()
{
	if (InfectionLevel >= AttackLevelTreshold && bIsAnEye)
	{
		if (!GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
		{
			GetWorldTimerManager().SetTimer(
				AttackTimerHandle,
				this,
				&ATile::TryAttack,
				AttackDelay,
				true,
				InitialAttackDelay
			);
		}
	}
}

bool ATile::HasLineOfSightToPlayer() const
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!Player) return false;

	FVector Start = GetActorLocation() + SurfaceNormal;
	FVector End = Player->GetActorLocation();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);     // Ignore the tile itself

	FHitResult Hit;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);


	if (!bHit)
		return true;

	return Hit.GetActor() == Player;
}

void ATile::TryAttack()
{
	if (!bShouldAttack || InfectionLevel < AttackLevelTreshold || !bIsAnEye || !HasLineOfSightToPlayer())
	{
		CleanupTendril();
		GetWorldTimerManager().ClearTimer(AttackTimerHandle);
		FTransform InstanceTransform;

		if (GridBuilder->SphereMeshInstanceEye->GetInstanceTransform(
			EyeIndex, InstanceTransform, true))
		{
			ACheddarCharacter* Player = Cast<ACheddarCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
			InstanceTransform.SetRotation(SurfaceRotation.Quaternion());

			GridBuilder->SphereMeshInstanceEye->UpdateInstanceTransform(EyeIndex, InstanceTransform, true, true, true);
		}
		return;
	}

	FTransform InstanceTransform;

	if (GridBuilder->SphereMeshInstanceEye->GetInstanceTransform(
		EyeIndex, InstanceTransform, true))
	{
		ACheddarCharacter* Player = Cast<ACheddarCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)); 
		InstanceTransform.SetRotation(
			(Player->GetActorLocation() - GetActorLocation()).Rotation().Quaternion());

		GridBuilder->SphereMeshInstanceEye->UpdateInstanceTransform(EyeIndex, InstanceTransform, true, true, true);
	}
	int32 Roll = FMath::RandRange(1, AttackChance);
	if (Roll == 1)
	{
		AttackPlayer();
	}
}

void ATile::AttackPlayer()
{
	if (!bShouldAttack)
	{
		CleanupTendril();
		GetWorldTimerManager().ClearTimer(AttackTimerHandle);
		return;
	}

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!Player) return;

	// Clear previous
	CleanupTendril();

	FVector P0 = GetActorLocation();
	FVector P2 = Player->GetActorLocation();

	FVector Dir = (P2 - P0).GetSafeNormal();
	FVector Right = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();

	if (FMath::RandBool())
		Right *= -1.f;  // randomly flip left/right

	float SideOffset = FMath::RandRange(150.f, 350.f);
	float UpOffset = FMath::RandRange(-150.f, 200.f);

	FVector P1 = P0 + Right * SideOffset + FVector::UpVector * UpOffset;

	// Build 25 points
	TendrilPoints.Empty();
	const int32 Count = 25;
	for (int32 I = 0; I < Count; I++)
	{
		float T = (float)I / (Count - 1);
		FVector Pos =
			FMath::Pow(1 - T, 2) * P0 +
			2 * (1 - T) * T * P1 +
			FMath::Pow(T, 2) * P2;

		TendrilPoints.Add(Pos);
	}

	// Start growth animation
	CurrentGrowIndex = 0;

	GetWorldTimerManager().SetTimer(
		TendrilGrowTimer,
		this,
		&ATile::GrowTendrilStep,
		AttackSphereSpawnRate,
		true
	);
}

void ATile::GrowTendrilStep()
{
	if (CurrentGrowIndex >= TendrilPoints.Num())
	{
		GetWorldTimerManager().ClearTimer(TendrilGrowTimer);

		GetWorldTimerManager().SetTimer(
			TendrilCleanupTimer,
			this,
			&ATile::CleanupTendril,
			TendrilDispawnTimer,   // cleanup delay
			false
		);

		return;
	}

	// Build spline gradually
	TendrilSpline->ClearSplinePoints();
	for (int32 I = 0; I <= CurrentGrowIndex; I++)
		TendrilSpline->AddSplinePoint(TendrilPoints[I], ESplineCoordinateSpace::World);

	TendrilSpline->UpdateSpline();

	UpdateTendrilMeshes();

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (Player)
	{
		float Distance = FVector::Dist(TendrilPoints[CurrentGrowIndex], Player->GetActorLocation());
		if (Distance <= AttackRadius)
		{
			ULife_AC* Life = Player->FindComponentByClass<ULife_AC>();
			if (Life)
			{
				Life->TakeDamage(AttackDamage, true, true);
			}
		}
	}

	CurrentGrowIndex++;
}

void ATile::UpdateTendrilMeshes()
{
	// Destroy old mesh
	for (auto* M : TendrilMeshes)
		if (M) M->DestroyComponent();
	TendrilMeshes.Empty();

	// Build new mesh segments
	for (int32 I = 0; I < TendrilSpline->GetNumberOfSplinePoints() - 1; I++)
	{
		USplineMeshComponent* Mesh = NewObject<USplineMeshComponent>(this);
		Mesh->RegisterComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->AttachToComponent(TendrilSpline, FAttachmentTransformRules::KeepRelativeTransform);

		//  tendril mesh
		Mesh->SetStaticMesh(TendrilStaticMesh);
		Mesh->SetForwardAxis(ESplineMeshAxis::Z, true);

		FVector StartPos, StartTangent;
		FVector EndPos, EndTangent;

		TendrilSpline->GetLocationAndTangentAtSplinePoint(I, StartPos, StartTangent, ESplineCoordinateSpace::Local);
		TendrilSpline->GetLocationAndTangentAtSplinePoint(I + 1, EndPos, EndTangent, ESplineCoordinateSpace::Local);

		Mesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

		// Tapering
		float T = (float)I / (float)(TendrilSpline->GetNumberOfSplinePoints() - 1);
		float StartScale = FMath::Lerp(1.0f, 0.2f, T);

		Mesh->SetStartScale(FVector2D(StartScale));
		Mesh->SetEndScale(FVector2D(StartScale * 0.9f));

		TendrilMeshes.Add(Mesh);
	}
}

void ATile::CleanupTendril()
{
	if (!TendrilSpline)
		return;

	for (auto* M : TendrilMeshes)
		if (M) M->DestroyComponent();
	TendrilMeshes.Empty();

	TendrilSpline->ClearSplinePoints();
}

void ATile::OnDealDamage(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
		return;

	APawn* PlayerPawn = Cast<APawn>(OtherActor);
	if (PlayerPawn)
	{
		ULife_AC* LifeComp = PlayerPawn->FindComponentByClass<ULife_AC>();
		if (LifeComp)
		{
			LifeComp->TakeDamage(AttackDamage, true, true);
		}

		/*
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
		}
		*/
	}
}