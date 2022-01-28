// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GASShooterALSGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class GASSHOOTERALS_API AGASShooterALSGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AGASShooterALSGameModeBase();

	void HeroDied(AController* Controller);

protected:
	float RespawnDelay;

	TSubclassOf<class AGSHeroCharacter> HeroClass;

	AActor* EnemySpawnPoint;

	virtual void BeginPlay() override;

	void RespawnHero(AController* Controller);
};
