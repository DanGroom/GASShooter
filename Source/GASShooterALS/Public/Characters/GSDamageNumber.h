// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GASShooterALS/GASShooterALS.h"
#include "GSDamageNumber.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, ACharacter*, Character);

USTRUCT(BlueprintType)
struct GASSHOOTERALS_API FGSDamageNumber
{
	GENERATED_USTRUCT_BODY()

		float DamageAmount;

	FGameplayTagContainer Tags;

	FGSDamageNumber() {}

	FGSDamageNumber(float InDamageAmount, FGameplayTagContainer InTags) : DamageAmount(InDamageAmount)
	{
		// Copy tag container
		Tags.AppendTags(InTags);
	}
};
