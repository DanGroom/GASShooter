// Copyright 2020 Dan Kestranek.
// modified by Dan Groom

#include "Characters/GSCharacterBase.h"
#include "Characters/Abilities/AttributeSets/GSAttributeSetBase.h"
#include "Characters/Abilities/GSAbilitySystemComponent.h"
#include "Characters/Abilities/GSAbilitySystemGlobals.h"
#include "Characters/Abilities/GSGameplayAbility.h"
#include "Characters/GSCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "UI/GSDamageTextWidgetComponent.h"

#include "Characters/GSALSCharacterAnimInstance.h"
#include "Character/Animation/ALSPlayerCameraBehavior.h"
#include "Library/ALSMathLibrary.h"
#include "Components/ALSDebugComponent.h"
#include "Curves/CurveFloat.h"
#include "Character/ALSCharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

/// /////////////////////////////////////////////////////////////////////////
/// GASShooter starts here
/// /////////////////////////////////////////////////////////////////////////
/// 

// Sets default values
AGSCharacterBase::AGSCharacterBase(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UGSCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = 0;
	bReplicates = true;
	SetReplicatingMovement(true);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	bAlwaysRelevant = true;

	// Cache tags
	DeadTag = FGameplayTag::RequestGameplayTag("State.Dead");
	EffectRemoveOnDeathTag = FGameplayTag::RequestGameplayTag("Effect.RemoveOnDeath");

	// Hardcoding to avoid having to manually set for every Blueprint child class
	DamageNumberClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Game/GASShooterALS/UI/WC_DamageText.WC_DamageText_C"));
	if (!DamageNumberClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Failed to find DamageNumberClass. If it was moved, please update the reference location in C++."), *FString(__FUNCTION__));
	}
}

UAbilitySystemComponent* AGSCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AGSCharacterBase::IsAlive() const
{
	return GetHealth() > 0.0f;
}

int32 AGSCharacterBase::GetAbilityLevel(EGSAbilityInputID AbilityID) const
{
	//TODO
	return 1;
}

void AGSCharacterBase::RemoveCharacterAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || !AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	// Remove any abilities added from a previous call. This checks to make sure the ability is in the startup 'CharacterAbilities' array.
	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if ((Spec.SourceObject == this) && CharacterAbilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	// Do in two passes so the removal happens after we have the full list
	for (int32 i = 0; i < AbilitiesToRemove.Num(); i++)
	{
		AbilitySystemComponent->ClearAbility(AbilitiesToRemove[i]);
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = false;
}

void AGSCharacterBase::Die()
{
	// Only runs on Server
	RemoveCharacterAbilities();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->GravityScale = 0;
	GetCharacterMovement()->Velocity = FVector(0);

	OnCharacterDied.Broadcast(this);

	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->CancelAllAbilities();

		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
	}

	//TODO replace with a locally executed GameplayCue
	/*
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
	else
	{
		FinishDying();
	}
	*/
}

void AGSCharacterBase::FinishDying()
{
	Destroy();
}

void AGSCharacterBase::AddDamageNumber(float Damage, FGameplayTagContainer DamageNumberTags)
{
	DamageNumberQueue.Add(FGSDamageNumber(Damage, DamageNumberTags));

	if (!GetWorldTimerManager().IsTimerActive(DamageNumberTimer))
	{
		GetWorldTimerManager().SetTimer(DamageNumberTimer, this, &AGSCharacterBase::ShowDamageNumber, 0.1, true, 0.0f);
	}
}

int32 AGSCharacterBase::GetCharacterLevel() const
{
	//TODO
	return 1;
}

float AGSCharacterBase::GetHealth() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetHealth();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMaxHealth() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMaxHealth();
	}
	
	return 0.0f;
}

float AGSCharacterBase::GetMana() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMana();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMaxMana() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMaxMana();
	}

	return 0.0f;
}

float AGSCharacterBase::GetStamina() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetStamina();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMaxStamina() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMaxStamina();
	}

	return 0.0f;
}

float AGSCharacterBase::GetShield() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetShield();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMaxShield() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMaxShield();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMoveSpeed() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMoveSpeed();
	}

	return 0.0f;
}

float AGSCharacterBase::GetMoveSpeedBaseValue() const
{
	if (IsValid(AttributeSetBase))
	{
		return AttributeSetBase->GetMoveSpeedAttribute().GetGameplayAttributeData(AttributeSetBase)->GetBaseValue();
	}

	return 0.0f;
}

// Called when the game starts or when spawned
void AGSCharacterBase::BeginPlay()
{
	{
		Super::BeginPlay();

		// If we're in networked game, disable curved movement
		bEnableNetworkOptimizations = !IsNetMode(NM_Standalone);

		// Make sure the mesh and animbp update after the CharacterBP to ensure it gets the most recent values.
		GetMesh()->AddTickPrerequisiteActor(this);

		// Set the Movement Model
		SetMovementModel();

		// Force update states to use the initial desired values.
		ForceUpdateCharacterState();

		if (Stance == EALSStance::Standing)
		{
			UnCrouch();
		}
		else if (Stance == EALSStance::Crouching)
		{
			Crouch();
		}

		// Set default rotation values.
		TargetRotation = GetActorRotation();
		LastVelocityRotation = TargetRotation;
		LastMovementInputRotation = TargetRotation;

		if (MainAnimInstance && GetLocalRole() == ROLE_SimulatedProxy)
		{
			MainAnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		}

		MyCharacterMovementComponent->SetMovementSettings(GetTargetMovementSettings());

		ALSDebugComponent = FindComponentByClass<UGSALSDebugComponent>();
	}
}

void AGSCharacterBase::AddCharacterAbilities()
{
	// Grant abilities, but only on the server	
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	for (TSubclassOf<UGSGameplayAbility>& StartupAbility : CharacterAbilities)
	{
		AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(StartupAbility, GetAbilityLevel(StartupAbility.GetDefaultObject()->AbilityID), static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID), this));
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = true;
}

void AGSCharacterBase::InitializeAttributes()
{
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	if (!DefaultAttributes)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing DefaultAttributes for %s. Please fill in the character's Blueprint."), *FString(__FUNCTION__), *GetName());
		return;
	}

	// Can run on Server and Client
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, GetCharacterLevel(), EffectContext);
	if (NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
	}
}

void AGSCharacterBase::AddStartupEffects()
{
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || AbilitySystemComponent->bStartupEffectsApplied)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> GameplayEffect : StartupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, GetCharacterLevel(), EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent);
		}
	}

	AbilitySystemComponent->bStartupEffectsApplied = true;
}

void AGSCharacterBase::ShowDamageNumber()
{
	if (DamageNumberQueue.Num() > 0 && IsValid(this))
	{
		check(DamageNumberClass != nullptr);
		UGSDamageTextWidgetComponent* DamageText = NewObject<UGSDamageTextWidgetComponent>(this, DamageNumberClass);
		DamageText->RegisterComponent();
		DamageText->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		DamageText->SetDamageText(DamageNumberQueue[0].DamageAmount, DamageNumberQueue[0].Tags);

		if (DamageNumberQueue.Num() < 1)
		{
			GetWorldTimerManager().ClearTimer(DamageNumberTimer);
		}

		DamageNumberQueue.RemoveAt(0);
	}
}

void AGSCharacterBase::SetHealth(float Health)
{
	if (IsValid(AttributeSetBase))
	{
		AttributeSetBase->SetHealth(Health);
	}
}

void AGSCharacterBase::SetMana(float Mana)
{
	if (IsValid(AttributeSetBase))
	{
		AttributeSetBase->SetMana(Mana);
	}
}

void AGSCharacterBase::SetStamina(float Stamina)
{
	if (IsValid(AttributeSetBase))
	{
		AttributeSetBase->SetStamina(Stamina);
	}
}

void AGSCharacterBase::SetShield(float Shield)
{
	if (IsValid(AttributeSetBase))
	{
		AttributeSetBase->SetShield(Shield);
	}
}

/// /////////////////////////////////////////////////////////////////////////
/// GASShooter ends here
/// /////////////////////////////////////////////////////////////////////////




/// /// /////////////////////////////////////////////////////////////////////////
/// ALS starts here
/// /////////////////////////////////////////////////////////////////////////

const FName NAME_FP_Camera(TEXT("FP_Camera"));
const FName NAME_Pelvis(TEXT("Pelvis"));
const FName NAME_RagdollPose(TEXT("RagdollPose"));
const FName NAME_RotationAmount(TEXT("RotationAmount"));
const FName NAME_YawOffset(TEXT("YawOffset"));
const FName NAME_pelvis(TEXT("pelvis"));
const FName NAME_root(TEXT("root"));
const FName NAME_spine_03(TEXT("spine_03"));

void AGSCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGSCharacterBase::PlayerForwardMovementInput);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGSCharacterBase::PlayerRightMovementInput);
	PlayerInputComponent->BindAxis("LookUp", this, &AGSCharacterBase::PlayerCameraUpInput);
	PlayerInputComponent->BindAxis("Turn", this, &AGSCharacterBase::PlayerCameraRightInput);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AGSCharacterBase::JumpPressedAction);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AGSCharacterBase::JumpReleasedAction);
	PlayerInputComponent->BindAction("StanceAction", IE_Pressed, this, &AGSCharacterBase::StancePressedAction);
	PlayerInputComponent->BindAction("WalkAction", IE_Pressed, this, &AGSCharacterBase::WalkPressedAction);
	PlayerInputComponent->BindAction("RagdollAction", IE_Pressed, this, &AGSCharacterBase::RagdollPressedAction);
	PlayerInputComponent->BindAction("SelectRotationMode_1", IE_Pressed, this,
		&AGSCharacterBase::VelocityDirectionPressedAction);
	PlayerInputComponent->BindAction("SelectRotationMode_2", IE_Pressed, this,
		&AGSCharacterBase::LookingDirectionPressedAction);
	PlayerInputComponent->BindAction("SprintAction", IE_Pressed, this, &AGSCharacterBase::SprintPressedAction);
	PlayerInputComponent->BindAction("SprintAction", IE_Released, this, &AGSCharacterBase::SprintReleasedAction);
	PlayerInputComponent->BindAction("AimAction", IE_Pressed, this, &AGSCharacterBase::AimPressedAction);
	PlayerInputComponent->BindAction("AimAction", IE_Released, this, &AGSCharacterBase::AimReleasedAction);
	PlayerInputComponent->BindAction("CameraAction", IE_Pressed, this, &AGSCharacterBase::CameraPressedAction);
	PlayerInputComponent->BindAction("CameraAction", IE_Released, this, &AGSCharacterBase::CameraReleasedAction);
}

void AGSCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	MyCharacterMovementComponent = Cast<UGSCharacterMovementComponent>(Super::GetMovementComponent());
}

void AGSCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGSCharacterBase, TargetRagdollLocation);
	DOREPLIFETIME_CONDITION(AGSCharacterBase, ReplicatedCurrentAcceleration, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AGSCharacterBase, ReplicatedControlRotation, COND_SkipOwner);

	DOREPLIFETIME(AGSCharacterBase, DesiredGait);
	DOREPLIFETIME_CONDITION(AGSCharacterBase, DesiredStance, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AGSCharacterBase, DesiredRotationMode, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(AGSCharacterBase, RotationMode, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AGSCharacterBase, OverlayState, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AGSCharacterBase, ViewMode, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AGSCharacterBase, VisibleMesh, COND_SkipOwner);
}

void AGSCharacterBase::OnBreakfall_Implementation()
{
	Replicated_PlayMontage(GetRollAnimation(), 1.35);
}

void AGSCharacterBase::Replicated_PlayMontage_Implementation(UAnimMontage* Montage, float PlayRate)
{
	// Roll: Simply play a Root Motion Montage.
	if (MainAnimInstance)
	{
		MainAnimInstance->Montage_Play(Montage, PlayRate);
	}
	Server_PlayMontage(Montage, PlayRate);
}

void AGSCharacterBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	if (GetMesh())
	{
		MainAnimInstance = Cast<UGSALSCharacterAnimInstance>(GetMesh()->GetAnimInstance());
	}
}

void AGSCharacterBase::SetAimYawRate(float NewAimYawRate)
{
	AimYawRate = NewAimYawRate;
	if (MainAnimInstance)
	{
		MainAnimInstance->GetCharacterInformationMutable().AimYawRate = AimYawRate;
	}
}

void AGSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Set required values
	SetEssentialValues(DeltaTime);

	if (MovementState == EALSMovementState::Grounded)
	{
		UpdateCharacterMovement();
		UpdateGroundedRotation(DeltaTime);
	}
	else if (MovementState == EALSMovementState::InAir)
	{
		UpdateInAirRotation(DeltaTime);
	}
	else if (MovementState == EALSMovementState::Ragdoll)
	{
		RagdollUpdate(DeltaTime);
	}

	// Cache values
	PreviousVelocity = GetVelocity();
	PreviousAimYaw = AimingRotation.Yaw;
}

void AGSCharacterBase::RagdollStart()
{
	if (RagdollStateChangedDelegate.IsBound())
	{
		RagdollStateChangedDelegate.Broadcast(true);
	}

	/** When Networked, disables replicate movement reset TargetRagdollLocation and ServerRagdollPull variable
	and if the host is a dedicated server, change character mesh optimisation option to avoid z-location bug*/
	MyCharacterMovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = 1;

	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
	{
		DefVisBasedTickOp = GetMesh()->VisibilityBasedAnimTickOption;
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
	TargetRagdollLocation = GetMesh()->GetSocketLocation(NAME_Pelvis);
	ServerRagdollPull = 0;

	// Step 1: Clear the Character Movement Mode and set the Movement State to Ragdoll
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	SetMovementState(EALSMovementState::Ragdoll);

	// Step 2: Disable capsule collision and enable mesh physics simulation starting from the pelvis.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionObjectType(ECC_PhysicsBody);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetAllBodiesBelowSimulatePhysics(NAME_Pelvis, true, true);

	// Step 3: Stop any active montages.
	if (MainAnimInstance)
	{
		MainAnimInstance->Montage_Stop(0.2f);
	}

	// Fixes character mesh is showing default A pose for a split-second just before ragdoll ends in listen server games
	GetMesh()->bOnlyAllowAutonomousTickPose = true;

	SetReplicateMovement(false);
}

void AGSCharacterBase::RagdollEnd()
{
	/** Re-enable Replicate Movement and if the host is a dedicated server set mesh visibility based anim
	tick option back to default*/

	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
	{
		GetMesh()->VisibilityBasedAnimTickOption = DefVisBasedTickOp;
	}

	// Revert back to default settings
	MyCharacterMovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = 0;
	GetMesh()->bOnlyAllowAutonomousTickPose = false;
	SetReplicateMovement(true);

	// Step 1: Save a snapshot of the current Ragdoll Pose for use in AnimGraph to blend out of the ragdoll
	if (MainAnimInstance)
	{
		MainAnimInstance->SavePoseSnapshot(NAME_RagdollPose);
	}

	// Step 2: If the ragdoll is on the ground, set the movement mode to walking and play a Get Up animation.
	// If not, set the movement mode to falling and update the character movement velocity to match the last ragdoll velocity.
	if (bRagdollOnGround)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		if (MainAnimInstance)
		{
			MainAnimInstance->Montage_Play(GetGetUpAnimation(bRagdollFaceUp), 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		}
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		GetCharacterMovement()->Velocity = LastRagdollVelocity;
	}

	// Step 3: Re-Enable capsule collision, and disable physics simulation on the mesh.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetAllBodiesSimulatePhysics(false);

	if (RagdollStateChangedDelegate.IsBound())
	{
		RagdollStateChangedDelegate.Broadcast(false);
	}
}

void AGSCharacterBase::Server_SetMeshLocationDuringRagdoll_Implementation(FVector MeshLocation)
{
	TargetRagdollLocation = MeshLocation;
}

void AGSCharacterBase::SetMovementState(const EALSMovementState NewState, bool bForce)
{
	if (bForce || MovementState != NewState)
	{
		PrevMovementState = MovementState;
		MovementState = NewState;
		OnMovementStateChanged(PrevMovementState);
	}
}

void AGSCharacterBase::SetMovementAction(const EALSMovementAction NewAction, bool bForce)
{
	if (bForce || MovementAction != NewAction)
	{
		const EALSMovementAction Prev = MovementAction;
		MovementAction = NewAction;
		OnMovementActionChanged(Prev);
	}
}

void AGSCharacterBase::SetStance(const EALSStance NewStance, bool bForce)
{
	if (bForce || Stance != NewStance)
	{
		const EALSStance Prev = Stance;
		Stance = NewStance;
		OnStanceChanged(Prev);
	}
}

void AGSCharacterBase::SetGait(const EALSGait NewGait, bool bForce)
{
	if (bForce || Gait != NewGait)
	{
		const EALSGait Prev = Gait;
		Gait = NewGait;
		OnGaitChanged(Prev);
	}
}


void AGSCharacterBase::SetDesiredStance(EALSStance NewStance)
{
	DesiredStance = NewStance;
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_SetDesiredStance(NewStance);
	}
}

void AGSCharacterBase::Server_SetDesiredStance_Implementation(EALSStance NewStance)
{
	SetDesiredStance(NewStance);
}

void AGSCharacterBase::SetDesiredGait(const EALSGait NewGait)
{
	DesiredGait = NewGait;
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_SetDesiredGait(NewGait);
	}
}

void AGSCharacterBase::Server_SetDesiredGait_Implementation(EALSGait NewGait)
{
	SetDesiredGait(NewGait);
}

void AGSCharacterBase::SetDesiredRotationMode(EALSRotationMode NewRotMode)
{
	DesiredRotationMode = NewRotMode;
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_SetDesiredRotationMode(NewRotMode);
	}
}

void AGSCharacterBase::Server_SetDesiredRotationMode_Implementation(EALSRotationMode NewRotMode)
{
	SetDesiredRotationMode(NewRotMode);
}

void AGSCharacterBase::SetRotationMode(const EALSRotationMode NewRotationMode, bool bForce)
{
	if (bForce || RotationMode != NewRotationMode)
	{
		const EALSRotationMode Prev = RotationMode;
		RotationMode = NewRotationMode;
		OnRotationModeChanged(Prev);

		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetRotationMode(NewRotationMode, bForce);
		}
	}
}


void AGSCharacterBase::Server_SetRotationMode_Implementation(EALSRotationMode NewRotationMode, bool bForce)
{
	SetRotationMode(NewRotationMode, bForce);
}

void AGSCharacterBase::SetViewMode(const EALSViewMode NewViewMode, bool bForce)
{
	if (bForce || ViewMode != NewViewMode)
	{
		const EALSViewMode Prev = ViewMode;
		ViewMode = NewViewMode;
		OnViewModeChanged(Prev);

		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetViewMode(NewViewMode, bForce);
		}
	}
}

void AGSCharacterBase::Server_SetViewMode_Implementation(EALSViewMode NewViewMode, bool bForce)
{
	SetViewMode(NewViewMode, bForce);
}

void AGSCharacterBase::SetOverlayState(const EALSOverlayState NewState, bool bForce)
{
	if (bForce || OverlayState != NewState)
	{
		const EALSOverlayState Prev = OverlayState;
		OverlayState = NewState;
		OnOverlayStateChanged(Prev);

		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetOverlayState(NewState, bForce);
		}
	}
}


void AGSCharacterBase::Server_SetOverlayState_Implementation(EALSOverlayState NewState, bool bForce)
{
	SetOverlayState(NewState, bForce);
}

void AGSCharacterBase::EventOnLanded()
{
	const float VelZ = FMath::Abs(GetCharacterMovement()->Velocity.Z);

	if (bRagdollOnLand && VelZ > RagdollOnLandVelocity)
	{
		ReplicatedRagdollStart();
	}
	else if (bBreakfallOnLand && bHasMovementInput && VelZ >= BreakfallOnLandVelocity)
	{
		OnBreakfall();
	}
	else
	{
		GetCharacterMovement()->BrakingFrictionFactor = bHasMovementInput ? 0.5f : 3.0f;

		// After 0.5 secs, reset braking friction factor to zero
		GetWorldTimerManager().SetTimer(OnLandedFrictionResetTimer, this,
			&AGSCharacterBase::OnLandFrictionReset, 0.5f, false);
	}
}

void AGSCharacterBase::Multicast_OnLanded_Implementation()
{
	if (!IsLocallyControlled())
	{
		EventOnLanded();
	}
}

void AGSCharacterBase::EventOnJumped()
{
	// Set the new In Air Rotation to the velocity rotation if speed is greater than 100.
	InAirRotation = Speed > 100.0f ? LastVelocityRotation : GetActorRotation();

	if (MainAnimInstance)
	{
		MainAnimInstance->OnJumped();
	}
}

void AGSCharacterBase::Server_PlayMontage_Implementation(UAnimMontage* Montage, float PlayRate)
{
	if (MainAnimInstance)
	{
		MainAnimInstance->Montage_Play(Montage, PlayRate);
	}

	ForceNetUpdate();
	Multicast_PlayMontage(Montage, PlayRate);
}

void AGSCharacterBase::Multicast_PlayMontage_Implementation(UAnimMontage* Montage, float PlayRate)
{
	if (MainAnimInstance && !IsLocallyControlled())
	{
		MainAnimInstance->Montage_Play(Montage, PlayRate);
	}
}

void AGSCharacterBase::Multicast_OnJumped_Implementation()
{
	if (!IsLocallyControlled())
	{
		EventOnJumped();
	}
}

void AGSCharacterBase::Server_RagdollStart_Implementation()
{
	Multicast_RagdollStart();
}

void AGSCharacterBase::Multicast_RagdollStart_Implementation()
{
	RagdollStart();
}

void AGSCharacterBase::Server_RagdollEnd_Implementation(FVector CharacterLocation)
{
	Multicast_RagdollEnd(CharacterLocation);
}

void AGSCharacterBase::Multicast_RagdollEnd_Implementation(FVector CharacterLocation)
{
	RagdollEnd();
}

void AGSCharacterBase::SetActorLocationAndTargetRotation(FVector NewLocation, FRotator NewRotation)
{
	SetActorLocationAndRotation(NewLocation, NewRotation);
	TargetRotation = NewRotation;
}

void AGSCharacterBase::SetMovementModel()
{
	const FString ContextString = GetFullName();
	FALSMovementStateSettings* OutRow =
		MovementModel.DataTable->FindRow<FALSMovementStateSettings>(MovementModel.RowName, ContextString);
	check(OutRow);
	MovementData = *OutRow;
}

void AGSCharacterBase::ForceUpdateCharacterState()
{
	SetGait(DesiredGait, true);
	SetStance(DesiredStance, true);
	SetRotationMode(DesiredRotationMode, true);
	SetViewMode(ViewMode, true);
	SetOverlayState(OverlayState, true);
	SetMovementState(MovementState, true);
	SetMovementAction(MovementAction, true);
}

void AGSCharacterBase::SetHasMovementInput(bool bNewHasMovementInput)
{
	bHasMovementInput = bNewHasMovementInput;

	if (MainAnimInstance)
	{
		MainAnimInstance->GetCharacterInformationMutable().bHasMovementInput = bHasMovementInput;
	}
}

FALSMovementSettings AGSCharacterBase::GetTargetMovementSettings() const
{
	if (RotationMode == EALSRotationMode::VelocityDirection)
	{
		if (Stance == EALSStance::Standing)
		{
			return MovementData.VelocityDirection.Standing;
		}
		if (Stance == EALSStance::Crouching)
		{
			return MovementData.VelocityDirection.Crouching;
		}
	}
	else if (RotationMode == EALSRotationMode::LookingDirection)
	{
		if (Stance == EALSStance::Standing)
		{
			return MovementData.LookingDirection.Standing;
		}
		if (Stance == EALSStance::Crouching)
		{
			return MovementData.LookingDirection.Crouching;
		}
	}
	else if (RotationMode == EALSRotationMode::Aiming)
	{
		if (Stance == EALSStance::Standing)
		{
			return MovementData.Aiming.Standing;
		}
		if (Stance == EALSStance::Crouching)
		{
			return MovementData.Aiming.Crouching;
		}
	}

	// Default to velocity dir standing
	return MovementData.VelocityDirection.Standing;
}

bool AGSCharacterBase::CanSprint() const
{
	// Determine if the character is currently able to sprint based on the Rotation mode and current acceleration
	// (input) rotation. If the character is in the Looking Rotation mode, only allow sprinting if there is full
	// movement input and it is faced forward relative to the camera + or - 50 degrees.

	if (!bHasMovementInput || RotationMode == EALSRotationMode::Aiming)
	{
		return false;
	}

	const bool bValidInputAmount = MovementInputAmount > 0.9f;

	if (RotationMode == EALSRotationMode::VelocityDirection)
	{
		return bValidInputAmount;
	}

	if (RotationMode == EALSRotationMode::LookingDirection)
	{
		const FRotator AccRot = ReplicatedCurrentAcceleration.ToOrientationRotator();
		FRotator Delta = AccRot - AimingRotation;
		Delta.Normalize();

		return bValidInputAmount && FMath::Abs(Delta.Yaw) < 50.0f;
	}

	return false;
}

void AGSCharacterBase::SetIsMoving(bool bNewIsMoving)
{
	bIsMoving = bNewIsMoving;

	if (MainAnimInstance)
	{
		MainAnimInstance->GetCharacterInformationMutable().bIsMoving = bIsMoving;
	}
}

FVector AGSCharacterBase::GetMovementInput() const
{
	return ReplicatedCurrentAcceleration;
}

void AGSCharacterBase::SetMovementInputAmount(float NewMovementInputAmount)
{
	MovementInputAmount = NewMovementInputAmount;

	if (MainAnimInstance)
	{
		MainAnimInstance->GetCharacterInformationMutable().MovementInputAmount = MovementInputAmount;
	}
}

void AGSCharacterBase::SetSpeed(float NewSpeed)
{
	Speed = NewSpeed;

	if (MainAnimInstance)
	{
		MainAnimInstance->GetCharacterInformationMutable().Speed = Speed;
	}
}

float AGSCharacterBase::GetAnimCurveValue(FName CurveName) const
{
	if (MainAnimInstance)
	{
		return MainAnimInstance->GetCurveValue(CurveName);
	}

	return 0.0f;
}

void AGSCharacterBase::SetVisibleMesh(USkeletalMesh* NewVisibleMesh)
{
	if (VisibleMesh != NewVisibleMesh)
	{
		const USkeletalMesh* Prev = VisibleMesh;
		VisibleMesh = NewVisibleMesh;
		OnVisibleMeshChanged(Prev);

		if (GetLocalRole() != ROLE_Authority)
		{
			Server_SetVisibleMesh(NewVisibleMesh);
		}
	}
}

void AGSCharacterBase::Server_SetVisibleMesh_Implementation(USkeletalMesh* NewVisibleMesh)
{
	SetVisibleMesh(NewVisibleMesh);
}

void AGSCharacterBase::SetRightShoulder(bool bNewRightShoulder)
{
	bRightShoulder = bNewRightShoulder;
	if (CameraBehavior)
	{
		CameraBehavior->bRightShoulder = bRightShoulder;
	}
}

ECollisionChannel AGSCharacterBase::GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius)
{
	TraceOrigin = GetActorLocation();
	TraceRadius = 10.0f;
	return ECC_Visibility;
}

FTransform AGSCharacterBase::GetThirdPersonPivotTarget()
{
	return GetActorTransform();
}

FVector AGSCharacterBase::GetFirstPersonCameraTarget()
{
	return GetMesh()->GetSocketLocation(NAME_FP_Camera);
}

void AGSCharacterBase::GetCameraParameters(float& TPFOVOut, float& FPFOVOut, bool& bRightShoulderOut) const
{
	TPFOVOut = ThirdPersonFOV;
	FPFOVOut = FirstPersonFOV;
	bRightShoulderOut = bRightShoulder;
}

void AGSCharacterBase::SetAcceleration(const FVector& NewAcceleration)
{
	Acceleration = (NewAcceleration != FVector::ZeroVector || IsLocallyControlled())
		? NewAcceleration
		: Acceleration / 2;

	if (MainAnimInstance)
	{
		MainAnimInstance->GetCharacterInformationMutable().Acceleration = Acceleration;
	}
}

void AGSCharacterBase::RagdollUpdate(float DeltaTime)
{
	// Set the Last Ragdoll Velocity.
	const FVector NewRagdollVel = GetMesh()->GetPhysicsLinearVelocity(NAME_root);
	LastRagdollVelocity = (NewRagdollVel != FVector::ZeroVector || IsLocallyControlled())
		? NewRagdollVel
		: LastRagdollVelocity / 2;

	// Use the Ragdoll Velocity to scale the ragdoll's joint strength for physical animation.
	const float SpringValue = FMath::GetMappedRangeValueClamped({ 0.0f, 1000.0f }, { 0.0f, 25000.0f },
		LastRagdollVelocity.Size());
	GetMesh()->SetAllMotorsAngularDriveParams(SpringValue, 0.0f, 0.0f, false);

	// Disable Gravity if falling faster than -4000 to prevent continual acceleration.
	// This also prevents the ragdoll from going through the floor.
	const bool bEnableGrav = LastRagdollVelocity.Z > -4000.0f;
	GetMesh()->SetEnableGravity(bEnableGrav);

	// Update the Actor location to follow the ragdoll.
	SetActorLocationDuringRagdoll(DeltaTime);
}

void AGSCharacterBase::SetActorLocationDuringRagdoll(float DeltaTime)
{
	if (IsLocallyControlled())
	{
		// Set the pelvis as the target location.
		TargetRagdollLocation = GetMesh()->GetSocketLocation(NAME_Pelvis);
		if (!HasAuthority())
		{
			Server_SetMeshLocationDuringRagdoll(TargetRagdollLocation);
		}
	}

	// Determine wether the ragdoll is facing up or down and set the target rotation accordingly.
	const FRotator PelvisRot = GetMesh()->GetSocketRotation(NAME_Pelvis);

	if (bReversedPelvis) {
		bRagdollFaceUp = PelvisRot.Roll > 0.0f;
	}
	else
	{
		bRagdollFaceUp = PelvisRot.Roll < 0.0f;
	}


	const FRotator TargetRagdollRotation(0.0f, bRagdollFaceUp ? PelvisRot.Yaw - 180.0f : PelvisRot.Yaw, 0.0f);

	// Trace downward from the target location to offset the target location,
	// preventing the lower half of the capsule from going through the floor when the ragdoll is laying on the ground.
	const FVector TraceVect(TargetRagdollLocation.X, TargetRagdollLocation.Y,
		TargetRagdollLocation.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	UWorld* World = GetWorld();
	check(World);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, TargetRagdollLocation, TraceVect,
		ECC_Visibility, Params);

	if (ALSDebugComponent && ALSDebugComponent->GetShowTraces())
	{
		UALSDebugComponent::DrawDebugLineTraceSingle(World,
			TargetRagdollLocation,
			TraceVect,
			EDrawDebugTrace::Type::ForOneFrame,
			bHit,
			HitResult,
			FLinearColor::Red,
			FLinearColor::Green,
			1.0f);
	}

	bRagdollOnGround = HitResult.IsValidBlockingHit();
	FVector NewRagdollLoc = TargetRagdollLocation;

	if (bRagdollOnGround)
	{
		const float ImpactDistZ = FMath::Abs(HitResult.ImpactPoint.Z - HitResult.TraceStart.Z);
		NewRagdollLoc.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - ImpactDistZ + 2.0f;
	}
	if (!IsLocallyControlled())
	{
		ServerRagdollPull = FMath::FInterpTo(ServerRagdollPull, 750.0f, DeltaTime, 0.6f);
		float RagdollSpeed = FVector(LastRagdollVelocity.X, LastRagdollVelocity.Y, 0).Size();
		FName RagdollSocketPullName = RagdollSpeed > 300 ? NAME_spine_03 : NAME_pelvis;
		GetMesh()->AddForce(
			(TargetRagdollLocation - GetMesh()->GetSocketLocation(RagdollSocketPullName)) * ServerRagdollPull,
			RagdollSocketPullName, true);
	}
	SetActorLocationAndTargetRotation(bRagdollOnGround ? NewRagdollLoc : TargetRagdollLocation, TargetRagdollRotation);
}

void AGSCharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// Use the Character Movement Mode changes to set the Movement States to the right values. This allows you to have
	// a custom set of movement states but still use the functionality of the default character movement component.

	if (GetCharacterMovement()->MovementMode == MOVE_Walking ||
		GetCharacterMovement()->MovementMode == MOVE_NavWalking)
	{
		SetMovementState(EALSMovementState::Grounded);
	}
	else if (GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		SetMovementState(EALSMovementState::InAir);
	}
}

void AGSCharacterBase::OnMovementStateChanged(const EALSMovementState PreviousState)
{
	if (MainAnimInstance)
	{
		FALSAnimCharacterInformation& AnimData = MainAnimInstance->GetCharacterInformationMutable();
		AnimData.PrevMovementState = PrevMovementState;
		MainAnimInstance->MovementState = MovementState;
	}

	if (MovementState == EALSMovementState::InAir)
	{
		if (MovementAction == EALSMovementAction::None)
		{
			// If the character enters the air, set the In Air Rotation and uncrouch if crouched.
			InAirRotation = GetActorRotation();
			if (Stance == EALSStance::Crouching)
			{
				UnCrouch();
			}
		}
		else if (MovementAction == EALSMovementAction::Rolling)
		{
			// If the character is currently rolling, enable the ragdoll.
			ReplicatedRagdollStart();
		}
	}

	if (CameraBehavior)
	{
		CameraBehavior->MovementState = MovementState;
	}
}

void AGSCharacterBase::OnMovementActionChanged(const EALSMovementAction PreviousAction)
{
	if (MainAnimInstance)
	{
		MainAnimInstance->MovementAction = MovementAction;
	}

	// Make the character crouch if performing a roll.
	if (MovementAction == EALSMovementAction::Rolling)
	{
		Crouch();
	}

	if (PreviousAction == EALSMovementAction::Rolling)
	{
		if (DesiredStance == EALSStance::Standing)
		{
			UnCrouch();
		}
		else if (DesiredStance == EALSStance::Crouching)
		{
			Crouch();
		}
	}

	if (CameraBehavior)
	{
		CameraBehavior->MovementAction = MovementAction;
	}
}

void AGSCharacterBase::OnStanceChanged(const EALSStance PreviousStance)
{
	if (MainAnimInstance)
	{
		MainAnimInstance->Stance = Stance;
	}

	if (CameraBehavior)
	{
		CameraBehavior->Stance = Stance;
	}

	MyCharacterMovementComponent->SetMovementSettings(GetTargetMovementSettings());
}

void AGSCharacterBase::OnRotationModeChanged(EALSRotationMode PreviousRotationMode)
{
	if (MainAnimInstance)
	{
		MainAnimInstance->RotationMode = RotationMode;
	}

	if (RotationMode == EALSRotationMode::VelocityDirection && ViewMode == EALSViewMode::FirstPerson)
	{
		// If the new rotation mode is Velocity Direction and the character is in First Person,
		// set the viewmode to Third Person.
		SetViewMode(EALSViewMode::ThirdPerson);
	}

	if (CameraBehavior)
	{
		CameraBehavior->SetRotationMode(RotationMode);
	}

	MyCharacterMovementComponent->SetMovementSettings(GetTargetMovementSettings());
}

void AGSCharacterBase::OnGaitChanged(const EALSGait PreviousGait)
{
	if (MainAnimInstance)
	{
		MainAnimInstance->Gait = Gait;
	}

	if (CameraBehavior)
	{
		CameraBehavior->Gait = Gait;
	}
}

void AGSCharacterBase::OnViewModeChanged(const EALSViewMode PreviousViewMode)
{
	if (MainAnimInstance)
	{
		MainAnimInstance->GetCharacterInformationMutable().ViewMode = ViewMode;
	}

	if (ViewMode == EALSViewMode::ThirdPerson)
	{
		if (RotationMode == EALSRotationMode::VelocityDirection || RotationMode == EALSRotationMode::LookingDirection)
		{
			// If Third Person, set the rotation mode back to the desired mode.
			SetRotationMode(DesiredRotationMode);
		}
	}
	else if (ViewMode == EALSViewMode::FirstPerson && RotationMode == EALSRotationMode::VelocityDirection)
	{
		// If First Person, set the rotation mode to looking direction if currently in the velocity direction mode.
		SetRotationMode(EALSRotationMode::LookingDirection);
	}

	if (CameraBehavior)
	{
		CameraBehavior->ViewMode = ViewMode;
	}
}

void AGSCharacterBase::OnOverlayStateChanged(const EALSOverlayState PreviousState)
{
	if (MainAnimInstance)
	{
		MainAnimInstance->OverlayState = OverlayState;
	}
}

void AGSCharacterBase::OnVisibleMeshChanged(const USkeletalMesh* PrevVisibleMesh)
{
	// Update the Skeletal Mesh before we update materials and anim bp variables
	GetMesh()->SetSkeletalMesh(VisibleMesh);

	// Reset materials to their new mesh defaults
	if (GetMesh() != nullptr)
	{
		for (int32 MaterialIndex = 0; MaterialIndex < GetMesh()->GetNumMaterials(); ++MaterialIndex)
		{
			GetMesh()->SetMaterial(MaterialIndex, nullptr);
		}
	}

	// Force set variables. This ensures anim instance & character stay synchronized on mesh changes
	ForceUpdateCharacterState();
}

void AGSCharacterBase::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SetStance(EALSStance::Crouching);
}

void AGSCharacterBase::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SetStance(EALSStance::Standing);
}

void AGSCharacterBase::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	if (IsLocallyControlled())
	{
		EventOnJumped();
	}
	if (HasAuthority())
	{
		Multicast_OnJumped();
	}
}

void AGSCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (IsLocallyControlled())
	{
		EventOnLanded();
	}
	if (HasAuthority())
	{
		Multicast_OnLanded();
	}
}

void AGSCharacterBase::OnLandFrictionReset()
{
	// Reset the braking friction
	GetCharacterMovement()->BrakingFrictionFactor = 0.0f;
}

void AGSCharacterBase::SetEssentialValues(float DeltaTime)
{
	if (GetLocalRole() != ROLE_SimulatedProxy)
	{
		ReplicatedCurrentAcceleration = GetCharacterMovement()->GetCurrentAcceleration();
		ReplicatedControlRotation = GetControlRotation();
		EasedMaxAcceleration = GetCharacterMovement()->GetMaxAcceleration();
	}

	else
	{
		EasedMaxAcceleration = GetCharacterMovement()->GetMaxAcceleration() != 0
			? GetCharacterMovement()->GetMaxAcceleration()
			: EasedMaxAcceleration / 2;
	}

	// Interp AimingRotation to current control rotation for smooth character rotation movement. Decrease InterpSpeed
	// for slower but smoother movement.
	AimingRotation = FMath::RInterpTo(AimingRotation, ReplicatedControlRotation, DeltaTime, 30);

	// These values represent how the capsule is moving as well as how it wants to move, and therefore are essential
	// for any data driven animation system. They are also used throughout the system for various functions,
	// so I found it is easiest to manage them all in one place.

	const FVector CurrentVel = GetVelocity();

	// Set the amount of Acceleration.
	SetAcceleration((CurrentVel - PreviousVelocity) / DeltaTime);

	// Determine if the character is moving by getting it's speed. The Speed equals the length of the horizontal (x y)
	// velocity, so it does not take vertical movement into account. If the character is moving, update the last
	// velocity rotation. This value is saved because it might be useful to know the last orientation of movement
	// even after the character has stopped.
	SetSpeed(CurrentVel.Size2D());
	SetIsMoving(Speed > 1.0f);
	if (bIsMoving)
	{
		LastVelocityRotation = CurrentVel.ToOrientationRotator();
	}

	// Determine if the character has movement input by getting its movement input amount.
	// The Movement Input Amount is equal to the current acceleration divided by the max acceleration so that
	// it has a range of 0-1, 1 being the maximum possible amount of input, and 0 being none.
	// If the character has movement input, update the Last Movement Input Rotation.
	SetMovementInputAmount(ReplicatedCurrentAcceleration.Size() / EasedMaxAcceleration);
	SetHasMovementInput(MovementInputAmount > 0.0f);
	if (bHasMovementInput)
	{
		LastMovementInputRotation = ReplicatedCurrentAcceleration.ToOrientationRotator();
	}

	// Set the Aim Yaw rate by comparing the current and previous Aim Yaw value, divided by Delta Seconds.
	// This represents the speed the camera is rotating left to right.
	SetAimYawRate(FMath::Abs((AimingRotation.Yaw - PreviousAimYaw) / DeltaTime));
}

void AGSCharacterBase::UpdateCharacterMovement()
{
	// Set the Allowed Gait
	const EALSGait AllowedGait = GetAllowedGait();

	// Determine the Actual Gait. If it is different from the current Gait, Set the new Gait Event.
	const EALSGait ActualGait = GetActualGait(AllowedGait);

	if (ActualGait != Gait)
	{
		SetGait(ActualGait);
	}

	// Update the Character Max Walk Speed to the configured speeds based on the currently Allowed Gait.
	MyCharacterMovementComponent->SetAllowedGait(AllowedGait);
}

void AGSCharacterBase::UpdateGroundedRotation(float DeltaTime)
{
	if (MovementAction == EALSMovementAction::None)
	{
		const bool bCanUpdateMovingRot = ((bIsMoving && bHasMovementInput) || Speed > 150.0f) && !HasAnyRootMotion();
		if (bCanUpdateMovingRot)
		{
			const float GroundedRotationRate = CalculateGroundedRotationRate();
			if (RotationMode == EALSRotationMode::VelocityDirection)
			{
				// Velocity Direction Rotation
				SmoothCharacterRotation({ 0.0f, LastVelocityRotation.Yaw, 0.0f }, 800.0f, GroundedRotationRate,
					DeltaTime);
			}
			else if (RotationMode == EALSRotationMode::LookingDirection)
			{
				// Looking Direction Rotation
				float YawValue;
				if (Gait == EALSGait::Sprinting)
				{
					YawValue = LastVelocityRotation.Yaw;
				}
				else
				{
					// Walking or Running..
					const float YawOffsetCurveVal = MainAnimInstance ? MainAnimInstance->GetCurveValue(NAME_YawOffset) : 0.f;
					YawValue = AimingRotation.Yaw + YawOffsetCurveVal;
				}
				SmoothCharacterRotation({ 0.0f, YawValue, 0.0f }, 500.0f, GroundedRotationRate, DeltaTime);
			}
			else if (RotationMode == EALSRotationMode::Aiming)
			{
				const float ControlYaw = AimingRotation.Yaw;
				SmoothCharacterRotation({ 0.0f, ControlYaw, 0.0f }, 1000.0f, 20.0f, DeltaTime);
			}
		}
		else
		{
			// Not Moving

			if ((ViewMode == EALSViewMode::ThirdPerson && RotationMode == EALSRotationMode::Aiming) ||
				ViewMode == EALSViewMode::FirstPerson)
			{
				LimitRotation(-100.0f, 100.0f, 20.0f, DeltaTime);
			}

			// Apply the RotationAmount curve from Turn In Place Animations.
			// The Rotation Amount curve defines how much rotation should be applied each frame,
			// and is calculated for animations that are animated at 30fps.

			const float RotAmountCurve = MainAnimInstance ? MainAnimInstance->GetCurveValue(NAME_RotationAmount) : 0.f;

			if (FMath::Abs(RotAmountCurve) > 0.001f)
			{
				if (GetLocalRole() == ROLE_AutonomousProxy)
				{
					TargetRotation.Yaw = UKismetMathLibrary::NormalizeAxis(
						TargetRotation.Yaw + (RotAmountCurve * (DeltaTime / (1.0f / 30.0f))));
					SetActorRotation(TargetRotation);
				}
				else
				{
					AddActorWorldRotation({ 0, RotAmountCurve * (DeltaTime / (1.0f / 30.0f)), 0 });
				}
				TargetRotation = GetActorRotation();
			}
		}
	}
	else if (MovementAction == EALSMovementAction::Rolling)
	{
		// Rolling Rotation (Not allowed on networked games)
		if (!bEnableNetworkOptimizations && bHasMovementInput)
		{
			SmoothCharacterRotation({ 0.0f, LastMovementInputRotation.Yaw, 0.0f }, 0.0f, 2.0f, DeltaTime);
		}
	}

	// Other actions are ignored...
}

void AGSCharacterBase::UpdateInAirRotation(float DeltaTime)
{
	if (RotationMode == EALSRotationMode::VelocityDirection || RotationMode == EALSRotationMode::LookingDirection)
	{
		// Velocity / Looking Direction Rotation
		SmoothCharacterRotation({ 0.0f, InAirRotation.Yaw, 0.0f }, 0.0f, 5.0f, DeltaTime);
	}
	else if (RotationMode == EALSRotationMode::Aiming)
	{
		// Aiming Rotation
		SmoothCharacterRotation({ 0.0f, AimingRotation.Yaw, 0.0f }, 0.0f, 15.0f, DeltaTime);
		InAirRotation = GetActorRotation();
	}
}

EALSGait AGSCharacterBase::GetAllowedGait() const
{
	// Calculate the Allowed Gait. This represents the maximum Gait the character is currently allowed to be in,
	// and can be determined by the desired gait, the rotation mode, the stance, etc. For example,
	// if you wanted to force the character into a walking state while indoors, this could be done here.

	if (Stance == EALSStance::Standing)
	{
		if (RotationMode != EALSRotationMode::Aiming)
		{
			if (DesiredGait == EALSGait::Sprinting)
			{
				return CanSprint() ? EALSGait::Sprinting : EALSGait::Running;
			}
			return DesiredGait;
		}
	}

	// Crouching stance & Aiming rot mode has same behaviour

	if (DesiredGait == EALSGait::Sprinting)
	{
		return EALSGait::Running;
	}

	return DesiredGait;
}

EALSGait AGSCharacterBase::GetActualGait(EALSGait AllowedGait) const
{
	// Get the Actual Gait. This is calculated by the actual movement of the character,  and so it can be different
	// from the desired gait or allowed gait. For instance, if the Allowed Gait becomes walking,
	// the Actual gait will still be running untill the character decelerates to the walking speed.

	const float LocWalkSpeed = MyCharacterMovementComponent->CurrentMovementSettings.WalkSpeed;
	const float LocRunSpeed = MyCharacterMovementComponent->CurrentMovementSettings.RunSpeed;

	if (Speed > LocRunSpeed + 10.0f)
	{
		if (AllowedGait == EALSGait::Sprinting)
		{
			return EALSGait::Sprinting;
		}
		return EALSGait::Running;
	}

	if (Speed >= LocWalkSpeed + 10.0f)
	{
		return EALSGait::Running;
	}

	return EALSGait::Walking;
}

void AGSCharacterBase::SmoothCharacterRotation(FRotator Target, float TargetInterpSpeed, float ActorInterpSpeed,
	float DeltaTime)
{
	// Interpolate the Target Rotation for extra smooth rotation behavior
	TargetRotation =
		FMath::RInterpConstantTo(TargetRotation, Target, DeltaTime, TargetInterpSpeed);
	SetActorRotation(
		FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, ActorInterpSpeed));
}

float AGSCharacterBase::CalculateGroundedRotationRate() const
{
	// Calculate the rotation rate by using the current Rotation Rate Curve in the Movement Settings.
	// Using the curve in conjunction with the mapped speed gives you a high level of control over the rotation
	// rates for each speed. Increase the speed if the camera is rotating quickly for more responsive rotation.

	const float MappedSpeedVal = MyCharacterMovementComponent->GetMappedSpeed();
	const float CurveVal =
		MyCharacterMovementComponent->CurrentMovementSettings.RotationRateCurve->GetFloatValue(MappedSpeedVal);
	const float ClampedAimYawRate = FMath::GetMappedRangeValueClamped({ 0.0f, 300.0f }, { 1.0f, 3.0f }, AimYawRate);
	return CurveVal * ClampedAimYawRate;
}

void AGSCharacterBase::LimitRotation(float AimYawMin, float AimYawMax, float InterpSpeed, float DeltaTime)
{
	// Prevent the character from rotating past a certain angle.
	FRotator Delta = AimingRotation - GetActorRotation();
	Delta.Normalize();
	const float RangeVal = Delta.Yaw;

	if (RangeVal < AimYawMin || RangeVal > AimYawMax)
	{
		const float ControlRotYaw = AimingRotation.Yaw;
		const float TargetYaw = ControlRotYaw + (RangeVal > 0.0f ? AimYawMin : AimYawMax);
		SmoothCharacterRotation({ 0.0f, TargetYaw, 0.0f }, 0.0f, InterpSpeed, DeltaTime);
	}
}

void AGSCharacterBase::GetControlForwardRightVector(FVector& Forward, FVector& Right) const
{
	const FRotator ControlRot(0.0f, AimingRotation.Yaw, 0.0f);
	Forward = GetInputAxisValue("MoveForward") * UKismetMathLibrary::GetForwardVector(ControlRot);
	Right = GetInputAxisValue("MoveRight") * UKismetMathLibrary::GetRightVector(ControlRot);
}

FVector AGSCharacterBase::GetPlayerMovementInput() const
{
	FVector Forward = FVector::ZeroVector;
	FVector Right = FVector::ZeroVector;
	GetControlForwardRightVector(Forward, Right);
	return (Forward + Right).GetSafeNormal();
}

void AGSCharacterBase::PlayerForwardMovementInput(float Value)
{
	if (MovementState == EALSMovementState::Grounded || MovementState == EALSMovementState::InAir)
	{
		// Default camera relative movement behavior
		const float Scale = UALSMathLibrary::FixDiagonalGamepadValues(Value, GetInputAxisValue("MoveRight")).Key;
		const FRotator DirRotator(0.0f, AimingRotation.Yaw, 0.0f);
		AddMovementInput(UKismetMathLibrary::GetForwardVector(DirRotator), Scale);
	}
}

void AGSCharacterBase::PlayerRightMovementInput(float Value)
{
	if (MovementState == EALSMovementState::Grounded || MovementState == EALSMovementState::InAir)
	{
		// Default camera relative movement behavior
		const float Scale = UALSMathLibrary::FixDiagonalGamepadValues(GetInputAxisValue("MoveForward"), Value)
			.Value;
		const FRotator DirRotator(0.0f, AimingRotation.Yaw, 0.0f);
		AddMovementInput(UKismetMathLibrary::GetRightVector(DirRotator), Scale);
	}
}

void AGSCharacterBase::PlayerCameraUpInput(float Value)
{
	AddControllerPitchInput(LookUpDownRate * Value);
}

void AGSCharacterBase::PlayerCameraRightInput(float Value)
{
	AddControllerYawInput(LookLeftRightRate * Value);
}

void AGSCharacterBase::JumpPressedAction()
{
	// Jump Action: Press "Jump Action" to end the ragdoll if ragdolling, stand up if crouching, or jump if standing.

	if (JumpPressedDelegate.IsBound())
	{
		JumpPressedDelegate.Broadcast();
	}

	if (MovementAction == EALSMovementAction::None)
	{
		if (MovementState == EALSMovementState::Grounded)
		{
			if (Stance == EALSStance::Standing)
			{
				Jump();
			}
			else if (Stance == EALSStance::Crouching)
			{
				UnCrouch();
			}
		}
		else if (MovementState == EALSMovementState::Ragdoll)
		{
			ReplicatedRagdollEnd();
		}
	}
}

void AGSCharacterBase::JumpReleasedAction()
{
	StopJumping();
}

void AGSCharacterBase::SprintPressedAction()
{
	SetDesiredGait(EALSGait::Sprinting);
}

void AGSCharacterBase::SprintReleasedAction()
{
	SetDesiredGait(EALSGait::Running);
}

void AGSCharacterBase::AimPressedAction()
{
	// AimAction: Hold "AimAction" to enter the aiming mode, release to revert back the desired rotation mode.
	SetRotationMode(EALSRotationMode::Aiming);
}

void AGSCharacterBase::AimReleasedAction()
{
	if (ViewMode == EALSViewMode::ThirdPerson)
	{
		SetRotationMode(DesiredRotationMode);
	}
	else if (ViewMode == EALSViewMode::FirstPerson)
	{
		SetRotationMode(EALSRotationMode::LookingDirection);
	}
}

void AGSCharacterBase::CameraPressedAction()
{
	UWorld* World = GetWorld();
	check(World);
	CameraActionPressedTime = World->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(OnCameraModeSwapTimer, this,
		&AGSCharacterBase::OnSwitchCameraMode, ViewModeSwitchHoldTime, false);
}

void AGSCharacterBase::CameraReleasedAction()
{
	if (ViewMode == EALSViewMode::FirstPerson)
	{
		// Don't swap shoulders on first person mode
		return;
	}

	UWorld* World = GetWorld();
	check(World);
	if (World->GetTimeSeconds() - CameraActionPressedTime < ViewModeSwitchHoldTime)
	{
		// Switch shoulders
		SetRightShoulder(!bRightShoulder);
		GetWorldTimerManager().ClearTimer(OnCameraModeSwapTimer); // Prevent mode change
	}
}

void AGSCharacterBase::OnSwitchCameraMode()
{
	// Switch camera mode
	if (ViewMode == EALSViewMode::FirstPerson)
	{
		SetViewMode(EALSViewMode::ThirdPerson);
	}
	else if (ViewMode == EALSViewMode::ThirdPerson)
	{
		SetViewMode(EALSViewMode::FirstPerson);
	}
}


void AGSCharacterBase::StancePressedAction()
{
	// Stance Action: Press "Stance Action" to toggle Standing / Crouching, double tap to Roll.

	if (MovementAction != EALSMovementAction::None)
	{
		return;
	}

	UWorld* World = GetWorld();
	check(World);

	const float PrevStanceInputTime = LastStanceInputTime;
	LastStanceInputTime = World->GetTimeSeconds();

	if (LastStanceInputTime - PrevStanceInputTime <= RollDoubleTapTimeout)
	{
		// Roll
		Replicated_PlayMontage(GetRollAnimation(), 1.15f);

		if (Stance == EALSStance::Standing)
		{
			SetDesiredStance(EALSStance::Crouching);
		}
		else if (Stance == EALSStance::Crouching)
		{
			SetDesiredStance(EALSStance::Standing);
		}
		return;
	}

	if (MovementState == EALSMovementState::Grounded)
	{
		if (Stance == EALSStance::Standing)
		{
			SetDesiredStance(EALSStance::Crouching);
			Crouch();
		}
		else if (Stance == EALSStance::Crouching)
		{
			SetDesiredStance(EALSStance::Standing);
			UnCrouch();
		}
	}

	// Notice: MovementState == EALSMovementState::InAir case is removed
}

void AGSCharacterBase::WalkPressedAction()
{
	if (DesiredGait == EALSGait::Walking)
	{
		SetDesiredGait(EALSGait::Running);
	}
	else if (DesiredGait == EALSGait::Running)
	{
		SetDesiredGait(EALSGait::Walking);
	}
}

void AGSCharacterBase::RagdollPressedAction()
{
	// Ragdoll Action: Press "Ragdoll Action" to toggle the ragdoll state on or off.

	if (GetMovementState() == EALSMovementState::Ragdoll)
	{
		ReplicatedRagdollEnd();
	}
	else
	{
		ReplicatedRagdollStart();
	}
}

void AGSCharacterBase::VelocityDirectionPressedAction()
{
	// Select Rotation Mode: Switch the desired (default) rotation mode to Velocity or Looking Direction.
	// This will be the mode the character reverts back to when un-aiming
	SetDesiredRotationMode(EALSRotationMode::VelocityDirection);
	SetRotationMode(EALSRotationMode::VelocityDirection);
}

void AGSCharacterBase::LookingDirectionPressedAction()
{
	SetDesiredRotationMode(EALSRotationMode::LookingDirection);
	SetRotationMode(EALSRotationMode::LookingDirection);
}

void AGSCharacterBase::ReplicatedRagdollStart()
{
	if (HasAuthority())
	{
		Multicast_RagdollStart();
	}
	else
	{
		Server_RagdollStart();
	}
}

void AGSCharacterBase::ReplicatedRagdollEnd()
{
	if (HasAuthority())
	{
		Multicast_RagdollEnd(GetActorLocation());
	}
	else
	{
		Server_RagdollEnd(GetActorLocation());
	}
}

void AGSCharacterBase::OnRep_RotationMode(EALSRotationMode PrevRotMode)
{
	OnRotationModeChanged(PrevRotMode);
}

void AGSCharacterBase::OnRep_ViewMode(EALSViewMode PrevViewMode)
{
	OnViewModeChanged(PrevViewMode);
}

void AGSCharacterBase::OnRep_OverlayState(EALSOverlayState PrevOverlayState)
{
	OnOverlayStateChanged(PrevOverlayState);
}

void AGSCharacterBase::OnRep_VisibleMesh(USkeletalMesh* NewVisibleMesh)
{
	OnVisibleMeshChanged(NewVisibleMesh);
}

float AGSCharacterBase::GetFOV()
{
	return ViewMode == EALSViewMode::ThirdPerson ? ThirdPersonFOV : FirstPersonFOV;
}

void AGSCharacterBase::SetFOV(float FOV)
{
	if (ViewMode == EALSViewMode::ThirdPerson)
	{
		ThirdPersonFOV = FOV;
	}
	else
	{
		FirstPersonFOV = FOV;
	}
}

/// /// /////////////////////////////////////////////////////////////////////////
/// ALS ends here
/// /////////////////////////////////////////////////////////////////////////
