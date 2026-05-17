// Fill out your copyright notice in the Description page of Project Settings.


#include "Life_AC.h"

#include "Kismet/GameplayStatics.h"
// Sets default values for this component's properties
ULife_AC::ULife_AC()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void ULife_AC::BeginPlay()
{
    Super::BeginPlay();


    CurrentLife = MaxLife;

    if (HealthWidgetClass)
    {
        // Create widget ONLY if owner is a player
        // (Viewport widgets require a PlayerController)
        APlayerController* PC = GetWorld()->GetFirstPlayerController();

        if (PC)
        {
            HealthWidget = CreateWidget<UWBP_HealthBar>(PC, HealthWidgetClass);

            if (HealthWidget)
            {
                HealthWidget->AddToViewport();
                HealthWidget->UpdateHealth(CurrentLife, MaxLife);
            }
        }
    }

}

float ULife_AC::TakeDamage(float Amount, bool bInvicibilityFrames,bool bInvicibilityAffected)
{
    if (bInvincible && bInvicibilityAffected)
    {
        return 0;
    }

    // Stop regen when taking damage
    GetWorld()->GetTimerManager().ClearTimer(RegenTimer);

    // Apply damage
    CurrentLife -= Amount;

    if (CurrentLife < 0)
        CurrentLife = 0;

    // Update widget
    if (HealthWidget)
        HealthWidget->UpdateHealth(CurrentLife, MaxLife);

    // Death detection
    if (CurrentLife <= 0.0f)
    {
        OnDeath.Broadcast();
        return Amount;
    }

    // Start invincibility
    if (bInvicibilityFrames)
    {
        StartInvincibility();
    }
    if (bInvicibilityFrames)
    {
        if (HitSoundCue)
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                HitSoundCue,
                GetOwner()->GetActorLocation()
            );
        }

        if (DamageCameraShake)
        {
            if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
            {
                PC->ClientStartCameraShake(DamageCameraShake);
            }
        }
    }

    // Restart regen after delay
    GetWorld()->GetTimerManager().SetTimer(
        RegenTimer,
        this,
        &ULife_AC::StartRegen,
        RegenDelay,
        false
    );

    return Amount;
}

void ULife_AC::StartInvincibility()
{
    bInvincible = true;

    GetWorld()->GetTimerManager().SetTimer(
        InvincibilityTimer,
        this,
        &ULife_AC::EndInvincibility,
        InvincibilityDuration,
        false
    );
}

void ULife_AC::EndInvincibility()
{
    bInvincible = false;
}

void ULife_AC::StartRegen()
{
    GetWorld()->GetTimerManager().SetTimer(
        RegenTimer,
        this,
        &ULife_AC::RegenTick,
        0.1f,   // regen frequency
        true
    );
}

void ULife_AC::RegenTick()
{
    if (CurrentLife >= MaxLife)
    {
        CurrentLife = MaxLife;
        GetWorld()->GetTimerManager().ClearTimer(RegenTimer);
        return;
    }

    CurrentLife += RegenRate * 0.1f;

    if (HealthWidget)
        HealthWidget->UpdateHealth(CurrentLife, MaxLife);
}

bool ULife_AC::IsDead()
{

    return CurrentLife == 0;
}