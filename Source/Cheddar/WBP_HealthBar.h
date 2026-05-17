// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "WBP_HealthBar.generated.h"

/**
 * 
 */
UCLASS()
class CHEDDAR_API UWBP_HealthBar : public UUserWidget
{
	GENERATED_BODY()
	
public:

	// Called after the underlying slate widget is constructed
	virtual void NativeConstruct() override;

	// Function you call to update the health bar %
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UpdateHealth(float CurrentLife, float MaxLife);

protected:

	// Bind this to the ProgressBar in your UMG designer
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthProgress;
};
