// Copyright Epic Games, Inc. All Rights Reserved.

#include "CheddarGameMode.h"
#include "CheddarCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACheddarGameMode::ACheddarGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
