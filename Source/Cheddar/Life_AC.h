// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WBP_HealthBar.h"   
#include "Camera/CameraShakeBase.h"
#include "Life_AC.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComponentDeathSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLifeChanged, float, NewLife);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHEDDAR_API ULife_AC : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULife_AC();

	UFUNCTION(BlueprintCallable)
	bool IsDead();

	UFUNCTION(BluePrintCallable)
	float TakeDamage(float Amount, bool InvicibilityFrames, bool bInvicibilityAffected);

	void StartInvincibility();
	void EndInvincibility();

	void StartRegen();
	void RegenTick();

	UPROPERTY(BlueprintAssignable)
	FComponentDeathSignature OnDeath;

	UFUNCTION(BlueprintCallable, Category = "Life")
	float GetLifePercent() const { return CurrentLife / MaxLife; }

	UPROPERTY(BlueprintAssignable, Category = "Life")
	FLifeChanged OnLifeChanged;

	UPROPERTY()
	UWBP_HealthBar* HealthWidget;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWBP_HealthBar> HealthWidgetClass;



protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxLife = 10.0;

	// Value set to maxlife in begin play
	UPROPERTY(VisibleInstanceOnly)
	float CurrentLife = -1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Life")
	float InvincibilityDuration = 0.5f; // seconds

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Life")
	float RegenDelay = 3.0f; // seconds after last damage

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Life")
	float RegenRate = 1.0f; // life per second

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage|ScreenShake")
	TSubclassOf<UCameraShakeBase> DamageCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	USoundBase* HitSoundCue;

	bool bInvincible = false;

	FTimerHandle InvincibilityTimer;
	FTimerHandle RegenTimer;

		
};
