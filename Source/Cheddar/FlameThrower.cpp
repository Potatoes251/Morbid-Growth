#include "FlameThrower.h"

#include "Tile.h"

#include "WBP_HealthBar.h"  
#include "CheddarCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Camera/CameraComponent.h"

AFlameThrower::AFlameThrower()
{
	PrimaryActorTick.bCanEverTick = false;

	FlamethrowerAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FlamethrowerAudio"));
	FlamethrowerAudioComponent->bAutoActivate = false;
	FlamethrowerAudioComponent->SetupAttachment(RootComponent);
}

void AFlameThrower::BeginPlay()
{
	Super::BeginPlay();

	CurrentFuel = MaxFuelCapacity;

	Character = Cast<ACheddarCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (!Character)
		return;

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AFlameThrower::Fire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AFlameThrower::StopFire);
		}
	}

	if (FuelWidgetClass)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			FuelWidget = CreateWidget<UWBP_HealthBar>(PlayerController, FuelWidgetClass);

			if (FuelWidget)
			{
				FuelWidget->AddToViewport();
				FuelWidget->UpdateHealth(CurrentFuel, MaxFuelCapacity);
			}
		}
	}

	FlameNiagara = Character->FindComponentByClass<UNiagaraComponent>();
	if (FlameNiagara)
	{
		FlameNiagara->Deactivate();
	}

	if (FlamethrowerAudioComponent && FlamethrowerSound)
	{
		FlamethrowerAudioComponent->SetSound(FlamethrowerSound);
	}
}

void AFlameThrower::Regen()
{
	CurrentFuel += FuelRegen;
	if (CurrentFuel >= MaxFuelCapacity)
	{
		CurrentFuel = MaxFuelCapacity;
		GetWorldTimerManager().ClearTimer(TimerHandle);
	}
	if (FuelWidget)
		FuelWidget->UpdateHealth(CurrentFuel, MaxFuelCapacity);
}

void AFlameThrower::Fire()
{
	if (bIsHoldingFire || CurrentFuel < MinFuelToUse)
		return;

	bIsHoldingFire = true;

	if (FlameNiagara)
	{
		FlameNiagara->Activate(true);
	}

	if (FlamethrowerAudioComponent && !FlamethrowerAudioComponent->IsPlaying())
	{
		FlamethrowerAudioComponent->Play();
	}

	PerformFire();
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AFlameThrower::PerformFire, FireRate, true);
}


void AFlameThrower::PerformFire()
{
	if (!bIsHoldingFire || CurrentFuel < FuelComsumedByUse)
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);

		if (FlameNiagara)
		{
			FlameNiagara->Deactivate();
		}

		if (FlamethrowerAudioComponent && FlamethrowerAudioComponent->IsPlaying())
		{
			FlamethrowerAudioComponent->Stop(); // plays End section
		}

		GEngine->AddOnScreenDebugMessage(314, 3, FColor::Turquoise, FString::Printf(TEXT("not enough fuel %f"), CurrentFuel));
		return;
	}

	Character = Cast<ACheddarCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	if (!Character)
		return;

	if (!bInfiniteFuel)
		CurrentFuel -= FuelComsumedByUse;
	
	if (FuelWidget)
		FuelWidget->UpdateHealth(CurrentFuel, MaxFuelCapacity);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AFlameThrower::Regen, FuelRegenRate, true);

	FVector StartLocation = Character->GetActorLocation();
	FVector EndLocation = StartLocation + (Character->GetFirstPersonCameraComponent()->GetForwardVector() * FlameLength);
	FVector HalfSize = FVector(FlameWidth, FlameWidth, FlameHeight);
	FRotator Orientation = Character->GetFirstPersonCameraComponent()->GetComponentRotation();
	TArray<FHitResult> HitResult;
	FCollisionQueryParams Params;

	Params.AddIgnoredActor(this);

	if (GetWorld()->SweepMultiByChannel(
		HitResult, StartLocation, EndLocation, Orientation.Quaternion(), 
		ECC_WorldDynamic, FCollisionShape::MakeBox(HalfSize), Params))
	{
		for (const FHitResult& HitActor : HitResult)
		{
			ATile* HitTile = Cast<ATile>(HitActor.GetActor());
			if (!HitTile)
			{
				continue;
			}

			FVector TileLocation = HitTile->GetActorLocation();
			UPrimitiveComponent* TileRoot = Cast<UPrimitiveComponent>(HitTile->GetRootComponent());
			FVector TargetPoint;
			TileRoot->GetClosestPointOnCollision(StartLocation, TargetPoint);

			FHitResult OcclusionHit;
			FCollisionQueryParams OcclusionParams;
			OcclusionParams.AddIgnoredActor(this);
			OcclusionParams.AddIgnoredActor(HitTile);

			bool bBlocked = GetWorld()->LineTraceSingleByChannel(
				OcclusionHit, TargetPoint, StartLocation, ECC_WorldStatic, OcclusionParams);

			if (!bBlocked || OcclusionHit.GetActor() == Character)
			{
				HitTile->Burn();
			}
		}
	}
}

void AFlameThrower::StopFire()
{
	bIsHoldingFire = false;

	if (FlameNiagara)
	{
		FlameNiagara->Deactivate();
	}

	if (FlamethrowerAudioComponent && FlamethrowerAudioComponent->IsPlaying())
	{
		FlamethrowerAudioComponent->Stop(); // plays End section
	}

	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

