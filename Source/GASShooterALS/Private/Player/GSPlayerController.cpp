// Copyright 2020 Dan Kestranek.

//modified by Dan Groom


#include "Player/GSPlayerController.h"
#include "Characters/Abilities/AttributeSets/GSAmmoAttributeSet.h"
#include "Characters/Abilities/AttributeSets/GSAttributeSetBase.h"
#include "Characters/Abilities/GSAbilitySystemComponent.h"
#include "Characters/Heroes/GSHeroCharacter.h"
#include "Characters/Heroes/GSALSPlayerCameraManager.h"
#include "Characters/Components/GSALSDebugComponent.h"
#include "Player/GSPlayerState.h"
#include "UI/GSHUDWidget.h"
#include "Weapons/GSWeapon.h"

void AGSPlayerController::CreateHUD()
{
	// Only create once
	if (UIHUDWidget)
	{
		return;
	}

	if (!UIHUDWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing UIHUDWidgetClass. Please fill in on the Blueprint of the PlayerController."), *FString(__FUNCTION__));
		return;
	}

	// Only create a HUD for local player
	if (!IsLocalPlayerController())
	{
		return;
	}

	// Need a valid PlayerState to get attributes from
	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (!PS)
	{
		return;
	}

	UIHUDWidget = CreateWidget<UGSHUDWidget>(this, UIHUDWidgetClass);
	UIHUDWidget->AddToViewport();

	// Set attributes
	UIHUDWidget->SetCurrentHealth(PS->GetHealth());
	UIHUDWidget->SetMaxHealth(PS->GetMaxHealth());
	UIHUDWidget->SetHealthPercentage(PS->GetHealth() / PS->GetMaxHealth());
	UIHUDWidget->SetCurrentMana(PS->GetMana());
	UIHUDWidget->SetMaxMana(PS->GetMaxMana());
	UIHUDWidget->SetManaPercentage(PS->GetMana() / PS->GetMaxMana());
	UIHUDWidget->SetHealthRegenRate(PS->GetHealthRegenRate());
	UIHUDWidget->SetManaRegenRate(PS->GetManaRegenRate());
	UIHUDWidget->SetCurrentStamina(PS->GetStamina());
	UIHUDWidget->SetMaxStamina(PS->GetMaxStamina());
	UIHUDWidget->SetStaminaPercentage(PS->GetStamina() / PS->GetMaxStamina());
	UIHUDWidget->SetStaminaRegenRate(PS->GetStaminaRegenRate());
	UIHUDWidget->SetCurrentShield(PS->GetShield());
	UIHUDWidget->SetMaxShield(PS->GetMaxShield());
	UIHUDWidget->SetShieldRegenRate(PS->GetShieldRegenRate());
	UIHUDWidget->SetShieldPercentage(PS->GetShield() / PS->GetMaxShield());
	UIHUDWidget->SetExperience(PS->GetXP());
	UIHUDWidget->SetGold(PS->GetGold());
	UIHUDWidget->SetHeroLevel(PS->GetCharacterLevel());

	AGSHeroCharacter* Hero = GetPawn<AGSHeroCharacter>();
	if (Hero)
	{
		AGSWeapon* CurrentWeapon = Hero->GetCurrentWeapon();
		if (CurrentWeapon)
		{
			UIHUDWidget->SetEquippedWeaponSprite(CurrentWeapon->PrimaryIcon);
			UIHUDWidget->SetEquippedWeaponStatusText(CurrentWeapon->GetDefaultStatusText());
			UIHUDWidget->SetPrimaryClipAmmo(Hero->GetPrimaryClipAmmo());
			UIHUDWidget->SetReticle(CurrentWeapon->GetPrimaryHUDReticleClass());

			// PlayerState's Pawn isn't set up yet so we can't just call PS->GetPrimaryReserveAmmo()
			if (PS->GetAmmoAttributeSet())
			{
				FGameplayAttribute Attribute = PS->GetAmmoAttributeSet()->GetReserveAmmoAttributeFromTag(CurrentWeapon->PrimaryAmmoType);
				if (Attribute.IsValid())
				{
					UIHUDWidget->SetPrimaryReserveAmmo(PS->GetAbilitySystemComponent()->GetNumericAttribute(Attribute));
				}
			}
		}
	}
}

UGSHUDWidget* AGSPlayerController::GetGSHUD()
{
	return UIHUDWidget;
}

void AGSPlayerController::SetEquippedWeaponPrimaryIconFromSprite(UPaperSprite* InSprite)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetEquippedWeaponSprite(InSprite);
	}
}

void AGSPlayerController::SetEquippedWeaponStatusText(const FText& StatusText)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetEquippedWeaponStatusText(StatusText);
	}
}

void AGSPlayerController::SetPrimaryClipAmmo(int32 ClipAmmo)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetPrimaryClipAmmo(ClipAmmo);
	}
}

void AGSPlayerController::SetPrimaryReserveAmmo(int32 ReserveAmmo)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetPrimaryReserveAmmo(ReserveAmmo);
	}
}

void AGSPlayerController::SetSecondaryClipAmmo(int32 SecondaryClipAmmo)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetSecondaryClipAmmo(SecondaryClipAmmo);
	}
}

void AGSPlayerController::SetSecondaryReserveAmmo(int32 SecondaryReserveAmmo)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetSecondaryReserveAmmo(SecondaryReserveAmmo);
	}
}

void AGSPlayerController::SetHUDReticle(TSubclassOf<UGSHUDReticle> ReticleClass)
{
	// !GetWorld()->bIsTearingDown Stops an error when quitting PIE while targeting when the EndAbility resets the HUD reticle
	if (UIHUDWidget && GetWorld() && !GetWorld()->bIsTearingDown)
	{
		UIHUDWidget->SetReticle(ReticleClass);
	}
}

void AGSPlayerController::ShowDamageNumber_Implementation(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags)
{
	if (IsValid(TargetCharacter))
	{
		TargetCharacter->AddDamageNumber(DamageAmount, DamageNumberTags);
	}
}

bool AGSPlayerController::ShowDamageNumber_Validate(float DamageAmount, AGSCharacterBase* TargetCharacter, FGameplayTagContainer DamageNumberTags)
{
	return true;
}

void AGSPlayerController::SetRespawnCountdown_Implementation(float RespawnTimeRemaining)
{
	if (UIHUDWidget)
	{
		UIHUDWidget->SetRespawnCountdown(RespawnTimeRemaining);
	}
}

bool AGSPlayerController::SetRespawnCountdown_Validate(float RespawnTimeRemaining)
{
	return true;
}

void AGSPlayerController::ClientSetControlRotation_Implementation(FRotator NewRotation)
{
	SetControlRotation(NewRotation);
}

bool AGSPlayerController::ClientSetControlRotation_Validate(FRotator NewRotation)
{
	return true;
}

void AGSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (PS)
	{
		// Init ASC with PS (Owner) and our new Pawn (AvatarActor)
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, InPawn);
	}

	//ALS
	PossessedCharacter = Cast<AGSCharacterBase>(InPawn);
	if (!IsRunningDedicatedServer())
	{
		// Servers want to setup camera only in listen servers.
		SetupCamera();
	}
	SetupDebugInputs();
}

void AGSPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// For edge cases where the PlayerState is repped before the Hero is possessed.
	CreateHUD();
}

void AGSPlayerController::Kill()
{
	ServerKill();
}

void AGSPlayerController::ServerKill_Implementation()
{
	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (PS)
	{
		PS->GetAttributeSetBase()->SetHealth(0.0f);
	}
}

bool AGSPlayerController::ServerKill_Validate()
{
	return true;
}


///////////////////////////////////////////////////////////////////////////
// BEGIN ALS
///////////////////////////////////////////////////////////////////////////

void AGSPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	PossessedCharacter = Cast<AGSCharacterBase>(GetPawn());
	SetupCamera();
}

void AGSPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		SetupDebugInputs();
	}
}

void AGSPlayerController::SetupCamera()
{
	// Call "OnPossess" in Player Camera Manager when possessing a pawn
	AGSALSPlayerCameraManager* CastedMgr = Cast<AGSALSPlayerCameraManager>(PlayerCameraManager);
	if (PossessedCharacter && CastedMgr)
	{
		CastedMgr->OnPossess(PossessedCharacter);
	}
}

void AGSPlayerController::SetupDebugInputs()
{
	// Bind inputs for debugging
	if (PossessedCharacter)
	{
		UActorComponent* Comp = PossessedCharacter->GetComponentByClass(UGSALSDebugComponent::StaticClass());
		if (Comp)
		{
			UGSALSDebugComponent* DebugComp = Cast<UGSALSDebugComponent>(Comp);
			if (InputComponent && DebugComp)
			{
				InputComponent->BindKey(EKeys::Tab, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::ToggleHud);
				InputComponent->BindKey(EKeys::V, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::ToggleDebugView);
				InputComponent->BindKey(EKeys::T, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::ToggleTraces);
				InputComponent->BindKey(EKeys::Y, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::ToggleDebugShapes);
				InputComponent->BindKey(EKeys::U, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::ToggleLayerColors);
				InputComponent->BindKey(EKeys::I, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::ToggleCharacterInfo);
				InputComponent->BindKey(EKeys::Z, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::ToggleSlomo);
				InputComponent->BindKey(EKeys::Comma, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::PreviousFocusedDebugCharacter);
				InputComponent->BindKey(EKeys::Period, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::NextFocusedDebugCharacter);
				InputComponent->BindKey(EKeys::M, EInputEvent::IE_Pressed, DebugComp, &UGSALSDebugComponent::ToggleDebugMesh);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////
// END ALS
///////////////////////////////////////////////////////////////////////////