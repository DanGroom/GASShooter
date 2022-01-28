// Copyright 2020 Dan Kestranek.
// modified by Dan Groom

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "Library/ALSCharacterStructLibrary.h"
#include "GSCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class GASSHOOTERALS_API UGSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
	class FGSSavedMove : public FSavedMove_Character
	{
	public:

		typedef FSavedMove_Character Super;

		///@brief Resets all saved variables.
		virtual void Clear() override;

		///@brief Store input commands in the compressed flags.
		virtual uint8 GetCompressedFlags() const override;

		///@brief This is used to check whether or not two moves can be combined into one.
		///Basically you just check to make sure that the saved variables are the same.
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;

		///@brief Sets up the move before sending it to the server. 
		virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
		///@brief Sets variables on character movement component before making a predictive correction.
		virtual void PrepMoveFor(class ACharacter* Character) override;

		// Sprint
		uint8 SavedRequestToStartSprinting : 1;

		// Aim Down Sights
		uint8 SavedRequestToStartADS : 1;

/// /////////////////////////////////////////////////////////////////////////
/// ALS starts here
/// /////////////////////////////////////////////////////////////////////////

		// Walk Speed Update
		uint8 bSavedRequestMovementSettingsChange : 1;
		EALSGait SavedAllowedGait = EALSGait::Walking;

/// /////////////////////////////////////////////////////////////////////////
/// ALS ends here
/// /////////////////////////////////////////////////////////////////////////
	};

	class FGSNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
	{
	public:
		FGSNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		///@brief Allocates a new copy of our custom saved move
		virtual FSavedMovePtr AllocateNewMove() override;
	};

/// /////////////////////////////////////////////////////////////////////////
/// ALS starts here
/// /////////////////////////////////////////////////////////////////////////
public:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void OnMovementUpdated(float DeltaTime, const FVector& OldLocation, const FVector& OldVelocity) override;

	// Movement Settings Override
	virtual void PhysWalking(float deltaTime, int32 Iterations) override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxBrakingDeceleration() const override;

	// Movement Settings Variables
	UPROPERTY()
		uint8 bRequestMovementSettingsChange = 1;

	UPROPERTY()
		EALSGait AllowedGait = EALSGait::Walking;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Movement System")
		FALSMovementSettings CurrentMovementSettings;

	// Set Movement Curve (Called in every instance)
	float GetMappedSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Movement Settings")
		void SetMovementSettings(FALSMovementSettings NewMovementSettings);

	// Set Max Walking Speed (Called from the owning client)
	UFUNCTION(BlueprintCallable, Category = "Movement Settings")
		void SetAllowedGait(EALSGait NewAllowedGait);

	UFUNCTION(Reliable, Server, Category = "Movement Settings")
		void Server_SetAllowedGait(EALSGait NewAllowedGait);
/// 	/// /////////////////////////////////////////////////////////////////////////
/// ALS ends here
/// /////////////////////////////////////////////////////////////////////////

public:
	UGSCharacterMovementComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float SprintSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float ADSSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float KnockedDownSpeedMultiplier;

	uint8 RequestToStartSprinting : 1;
	uint8 RequestToStartADS : 1;

	FGameplayTag KnockedDownTag;
	FGameplayTag InteractingTag;
	FGameplayTag InteractingRemovalTag;

	virtual float GetMaxSpeed() const override;
//	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	//virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	// Sprint
	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void StartSprinting();
	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void StopSprinting();

	// Aim Down Sights
	UFUNCTION(BlueprintCallable, Category = "Aim Down Sights")
	void StartAimDownSights();
	UFUNCTION(BlueprintCallable, Category = "Aim Down Sights")
	void StopAimDownSights();
};
