// Copyright 2020 Dan Kestranek.


#include "GASShooterALSGameModeBase.h"
#include "Engine/World.h"
#include "Characters/Heroes/GSHeroCharacter.h"
#include "Player/GSPlayerController.h"
#include "Player/GSPlayerState.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGASShooterALSGameModeBase::AGASShooterALSGameModeBase()
{
	RespawnDelay = 5.0f;

	HeroClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Game/GASShooterALS/Characters/GS_ALS_CharacterBP.GS_ALS_CharacterBP_C"));
	if (!HeroClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Failed to find HeroClass. If it was moved, please update the reference location in C++."), *FString(__FUNCTION__));
	}
}

void AGASShooterALSGameModeBase::HeroDied(AController* Controller)
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ASpectatorPawn* SpectatorPawn = GetWorld()->SpawnActor<ASpectatorPawn>(SpectatorClass, Controller->GetPawn()->GetActorTransform(), SpawnParameters);

	Controller->UnPossess();
	Controller->Possess(SpectatorPawn);

	FTimerHandle RespawnTimerHandle;
	FTimerDelegate RespawnDelegate;

	RespawnDelegate = FTimerDelegate::CreateUObject(this, &AGASShooterALSGameModeBase::RespawnHero, Controller);
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, RespawnDelay, false);

	AGSPlayerController* PC = Cast<AGSPlayerController>(Controller);
	if (PC)
	{
		PC->SetRespawnCountdown(RespawnDelay);
	}
}

void AGASShooterALSGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// Get the enemy hero spawn point
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		if (Actor->GetName() == FString("EnemyHeroSpawn"))
		{
			EnemySpawnPoint = Actor;
			break;
		}
	}

	if (!EnemySpawnPoint)
	{
		UE_LOG(LogTemp, Error, TEXT("%s EnemySpawnPoint is null."), *FString(__FUNCTION__));
	}
}

void AGASShooterALSGameModeBase::RespawnHero(AController* Controller)
{
	if (!IsValid(Controller))
	{
		return;
	}

	if (Controller->IsPlayerController())
	{
		// Respawn player hero
		AActor* PlayerStart = FindPlayerStart(Controller);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AGSHeroCharacter* Hero = GetWorld()->SpawnActor<AGSHeroCharacter>(HeroClass, PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation(), SpawnParameters);

		APawn* OldSpectatorPawn = Controller->GetPawn();
		Controller->UnPossess();
		OldSpectatorPawn->Destroy();
		Controller->Possess(Hero);
		
		AGSPlayerController* PC = Cast<AGSPlayerController>(Controller);
		if (PC)
		{
			PC->ClientSetControlRotation(PlayerStart->GetActorRotation());
		}
	}
	else
	{
		// Respawn AI hero
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AGSHeroCharacter* Hero = GetWorld()->SpawnActor<AGSHeroCharacter>(HeroClass, EnemySpawnPoint->GetActorTransform(), SpawnParameters);

		APawn* OldSpectatorPawn = Controller->GetPawn();
		Controller->UnPossess();
		OldSpectatorPawn->Destroy();
		Controller->Possess(Hero);
	}
}
