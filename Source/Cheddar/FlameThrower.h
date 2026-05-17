// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SpotLightComponent.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "NiagaraComponent.h"
#include "FlameThrower.generated.h"

UCLASS()
class CHEDDAR_API AFlameThrower : public AActor
{
	GENERATED_BODY()
	
public:	
	AFlameThrower();

	void Fire();
	void PerformFire();
	void StopFire();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	class ACheddarCharacter* Character;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* FlamethrowerSound;

	UPROPERTY()
	UAudioComponent* FlamethrowerAudioComponent;

	UPROPERTY()
	class UWBP_HealthBar* FuelWidget;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UWBP_HealthBar> FuelWidgetClass;

	UPROPERTY(EditAnywhere)
	float FlameWidth;

	UPROPERTY(EditAnywhere)
	float FlameHeight;

	UPROPERTY(EditAnywhere)
	float FlameLength;

	UPROPERTY(EditAnywhere)
	float MaxFuelCapacity;

	UPROPERTY(EditAnywhere)
	float FuelComsumedByUse;

	UPROPERTY(EditAnywhere)
	float FuelRegenRate;

	UPROPERTY(EditAnywhere)
	float FuelRegen;

	UPROPERTY(EditAnywhere)
	float MinFuelToUse;

	UPROPERTY(EditAnywhere)
	float FireRate;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	UNiagaraComponent* FlameNiagara;
	UPROPERTY(EditAnywhere)
	bool bInfiniteFuel = false;

protected:
	virtual void BeginPlay() override;

private:
	void Regen();

	FTimerHandle FireTimerHandle;
	FTimerHandle TimerHandle;
	float CurrentFuel;
	bool bIsHoldingFire;


};
