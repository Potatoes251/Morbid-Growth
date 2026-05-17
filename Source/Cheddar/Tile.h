// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"


UCLASS()
class CHEDDAR_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	ATile();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// ---- INFECTION FUNCTIONS ----
    void    InfectTile();
    void    TryToInfect();
    void    RemoveFromNeighborInfectable();
    void    AddInNeighborInfectable();

	// ---- BURNING FUNCTIONS ----
    void    Burn();
    void    EndBurnCooldown();
    void    ApplyBurnCooldown();

	// ---- ASSOCIATION FUNCTIONS ----
    void    SetOwningChunk(class AChunk* Chunk);
    void    SetPillarBase(class AChunk* Chunk);
    void    AddGridBuilder(class AGridBuilder* GB);
    AChunk* GetOwningChunk();
    void    SetNewTargetChunk();
    void    SetOnPillar(bool b);
    bool    IsOnPillar() const;

    UFUNCTION(BlueprintCallable)
    void    StartAttacking();

    bool    HasLineOfSightToPlayer() const;


	// ---- INFECTION PROPERTIES ----
	// Size properties
    UPROPERTY(EditAnywhere, Category = "Size")
    float   InfectionSize = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Size")
    float   RandMaxSize = 0.10;

    UPROPERTY(EditAnywhere, Category = "Size")
    float   RandMinSize = -0.05;


    UPROPERTY(EditAnywhere, Category = "Size")
    float   OffsetRange = 0.10f;

	// Eye properties
    UPROPERTY(EditAnywhere, Category = "Eye")
    int32   RandEyeChance = 20;

    UPROPERTY(EditAnywhere, Category = "Eye")
    int32   EyeChanceLevelTreshold = 3;

	// Infection level properties
    int32   MaxInfectionLevel = 3;

    UPROPERTY(EditAnywhere, Category = "Level")
    int32   RandMaxInfectionLevel = 3;

    UPROPERTY(EditAnywhere, Category = "Level")
    int32   RandMinInfectionLevel = 1;

	// Timer properties

    UPROPERTY(EditAnywhere, Category = "Timer")
	float   RandInfectionInitialTimerMax = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Timer")
    float   RandInfectionInitialTimerMin = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Timer")
    float   InfectionTimerMin = 0.1f;

    UPROPERTY(EditAnywhere, Category = "Timer")
    float   InfectionTimerMax = 1.0f;

	// Infection Chase/Attack properties
    UPROPERTY(EditAnywhere, Category = "Chase/Attack")
    float   InfectionChaseSpeed = 0.05f;
    
    UPROPERTY(EditAnywhere, Category = "Chase/Attack")
    float InitialAttackDelay = 1;
    UPROPERTY(EditAnywhere, Category = "Chase/Attack")
    float AttackDelay = 1;
    UPROPERTY(EditAnywhere, Category = "Chase/Attack")
    int32 AttackChance = 10;

    UPROPERTY(EditAnywhere, Category = "Chase/Attack")
    int32 AttackLevelTreshold = 3;

    // Tendrils properties
    UPROPERTY(EditAnywhere, Category = "Tendrils")
    UStaticMesh*    TendrilStaticMesh;

    UPROPERTY(EditAnywhere, Category = "Tendrils")
    float   TendrilDispawnTimer = 0.6f;

    UPROPERTY(EditAnywhere, Category = "Tendrils")
    float   AttackSphereSpawnRate = 0.03f;

    UPROPERTY(EditAnywhere, Category = "Tendrils")
    float   AttackDamage = 1;

    UPROPERTY(EditAnywhere, Category = "Tendrils")
    float   AttackRadius = 50;

    UPROPERTY(EditAnywhere, Category = "Tendrils")
    TArray<class USplineMeshComponent*> TendrilMeshes;

	// Burn properties
    UPROPERTY(EditAnywhere, Category = "Burn")
    float   BurnCooldownDuration = 10.0f;


	// ---- END INFECTION PROPERTIES ----

    class   USphereComponent* SphereMesh;

    int32   InfectionLevel = 0;
    int32   SphereInstanceIndex = INDEX_NONE;
    int32   EyeIndex = INDEX_NONE;

    FVector     CachedInstanceScale;
    FVector     SurfaceNormal;

    TArray<ATile*>  InfectableTiles;
    TArray<ATile*>  NeighborTiles;

    UMaterialInstanceDynamic*   DynMat;

    UPROPERTY(BlueprintReadWrite)
    bool    bInPlayerRange = false;

    UPROPERTY(BlueprintReadWrite)
    bool    bShouldAttack = false;

private:

    void    UpdateInfectionLevel(int32 IncreaseAmount);
    void    SetInfectTimer();


    void    SpawnSphereMeshComponent(float scale, const FVector& Offset);
    void    ApplyEyeMesh();
    void    ApplyInfectionMesh();
    void    TeleportMesh(UInstancedStaticMeshComponent* Mesh, FVector NewLocation, int32 Index);


    ATile*  FindClosestToTarget(FVector Target);
    void    AttackPlayer();
    void    TryAttack();
    void    GrowTendrilStep();
    void    UpdateTendrilMeshes();
    void    CleanupTendril();
    void    OnDealDamage(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
            int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    class AGridBuilder*    GridBuilder;

    float   InfectionInitialTimer = 2.0f;
    float   InfectionTimer = 0.1f;
    
    TWeakObjectPtr<class AChunk>    PillarBase = nullptr;
    TWeakObjectPtr<class AChunk>    OwningChunk = nullptr;
    TWeakObjectPtr<class AChunk>    CurrentTargetChunk = nullptr;

    FVector     CurrentTargetLocation;
    FVector     WorldLocation;

    UPROPERTY(EditAnywhere)
    float MaxTargetDistanceSquared = 1000000.f;

    FTimerHandle    TimerHandle;
    FTimerHandle    AttackTimerHandle;
    FTimerHandle    TendrilCleanupTimer;
    FTimerHandle    TendrilGrowTimer;
    FTimerHandle    BurnCooldownHandle;

    TArray<UStaticMeshComponent*>   AttackSpheres;

    TArray<FVector>     TendrilPoints;

    int32   CurrentSpawnIndex = 0;
    int32   ComputedSphereCount = 0;
    int32   CurrentGrowIndex = 0;

    class USplineComponent* TendrilSpline;
    FRotator SurfaceRotation;

    bool bInBurnCooldown = false;
    bool bIsAnEye = false;
    bool bOnPillar = false;
};
