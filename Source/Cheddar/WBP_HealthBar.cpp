// Fill out your copyright notice in the Description page of Project Settings.


#include "WBP_HealthBar.h"


void UWBP_HealthBar::NativeConstruct()
{
	Super::NativeConstruct();

	// Optional: initialize to full bar
	if (HealthProgress)
	{
		HealthProgress->SetPercent(1.0f);
	}
}

void UWBP_HealthBar::UpdateHealth(float CurrentLife, float MaxLife)
{
	if (!HealthProgress || MaxLife <= 0)
		return;

	const float Percent = CurrentLife / MaxLife;
	HealthProgress->SetPercent(Percent);
}