﻿// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Achim Turan

//modified by Dan Groom

#pragma once

#include "CoreMinimal.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Components/ActorComponent.h"
#include "CollisionShape.h"
#include "GSALSDebugComponent.generated.h"

class AGSCharacterBase;
class USkeletalMesh;

UCLASS(Blueprintable, BlueprintType)
class GASSHOOTERALS_API UGSALSDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSALSDebugComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	/** Implemented on BP to update layering colors */
	UFUNCTION(BlueprintImplementableEvent, Category = "ALS|Debug")
	void UpdateColoringSystem();

	/** Implement on BP to draw debug spheres */
	UFUNCTION(BlueprintImplementableEvent, Category = "ALS|Debug")
	void DrawDebugSpheres();

	/** Implemented on BP to set/reset layering colors */
	UFUNCTION(BlueprintImplementableEvent, Category = "ALS|Debug")
	void SetResetColors();

	/** Implemented on BP to set dynamic color materials for debugging */
	UFUNCTION(BlueprintImplementableEvent, Category = "ALS|Debug")
	void SetDynamicMaterials();

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleGlobalTimeDilationLocal(float TimeDilation);

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleSlomo();

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleHud() { bShowHud = !bShowHud; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleDebugView();

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleDebugMesh();

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleTraces() { bShowTraces = !bShowTraces; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleDebugShapes() { bShowDebugShapes = !bShowDebugShapes; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleLayerColors() { bShowLayerColors = !bShowLayerColors; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void ToggleCharacterInfo() { bShowCharacterInfo = !bShowCharacterInfo; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	bool GetDebugView() { return bDebugView; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	bool GetShowTraces() { return bShowTraces; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	bool GetShowDebugShapes() { return bShowDebugShapes; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	bool GetShowLayerColors() { return bShowLayerColors; }

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void PreviousFocusedDebugCharacter();

	UFUNCTION(BlueprintCallable, Category = "ALS|Debug")
	void NextFocusedDebugCharacter();

	// utility functions to draw trace debug shapes,
	// which are derived from Engine/Private/KismetTraceUtils.h.
	// Sadly the functions are private, which was the reason
	// why there reimplemented here.
	static void DrawDebugLineTraceSingle(const UWorld* World,
	                                     const FVector& Start,
	                                     const FVector& End,
	                                     EDrawDebugTrace::Type DrawDebugType,
	                                     bool bHit,
	                                     const FHitResult& OutHit,
	                                     FLinearColor TraceColor,
	                                     FLinearColor TraceHitColor,
	                                     float DrawTime);

	static void DrawDebugCapsuleTraceSingle(const UWorld* World,
	                                        const FVector& Start,
	                                        const FVector& End,
	                                        const FCollisionShape& CollisionShape,
	                                        EDrawDebugTrace::Type DrawDebugType,
	                                        bool bHit,
	                                        const FHitResult& OutHit,
	                                        FLinearColor TraceColor,
	                                        FLinearColor TraceHitColor,
	                                        float DrawTime);

	static void DrawDebugSphereTraceSingle(const UWorld* World,
	                                       const FVector& Start,
	                                       const FVector& End,
	                                       const FCollisionShape& CollisionShape,
	                                       EDrawDebugTrace::Type DrawDebugType,
	                                       bool bHit,
	                                       const FHitResult& OutHit,
	                                       FLinearColor TraceColor,
	                                       FLinearColor TraceHitColor,
	                                       float DrawTime);

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "ALS|Debug")
	AGSCharacterBase* OwnerCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ALS|Debug")
	bool bSlomo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ALS|Debug")
	bool bShowHud = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ALS|Debug")
	bool bShowCharacterInfo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ALS|Debug")
	USkeletalMesh* DebugSkeletalMesh = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "ALS|Debug")
	TArray<AGSCharacterBase*> AvailableDebugCharacters;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Debug")
	AGSCharacterBase* DebugFocusCharacter = nullptr;
private:
	static bool bDebugView;

	static bool bShowTraces;

	static bool bShowDebugShapes;

	static bool bShowLayerColors;

	bool bNeedsColorReset = false;

	bool bDebugMeshVisible = false;

	UPROPERTY()
	USkeletalMesh* DefaultSkeletalMesh = nullptr;
	
	/// Stores the index, which is used to select the next focused debug ALSBaseCharacter.
	/// If no characters where found during BeginPlay the value should be set to INDEX_NONE.
	int32 FocusedDebugCharacterIndex = INDEX_NONE;
};

