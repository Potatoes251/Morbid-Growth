#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Root.generated.h"

UCLASS()
class CHEDDAR_API ARoot : public AActor
{
	GENERATED_BODY()
	
public:	
	ARoot();

protected:
	virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* SphereMesh;

    // How far we search for new surfaces
    UPROPERTY(EditAnywhere, Category = "Spread")
    float SpreadRange = 300.f;

    // How long to wait before trying to spread again
    UPROPERTY(EditAnywhere, Category = "Spread")
    float SpreadDelay = .1f;

    // Max number of children each sphere can spawn
    UPROPERTY(EditAnywhere, Category = "Spread")
    int MaxChildren = 4;

    int ChildrenSpawned = 0;

    // What class to spawn when spreading
    UPROPERTY(EditAnywhere, Category = "Spread")
    TSubclassOf<ARoot> SphereClass;

    void AttemptSpread();
};
