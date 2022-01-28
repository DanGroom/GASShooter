// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GSDamageTextWidgetComponent.generated.h"

/**
 * For the floating Damage Numbers when a Character receives damage.
 */
UCLASS()
class GASSHOOTERALS_API UGSDamageTextWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetDamageText(float Damage, const FGameplayTagContainer& Tags);
};
