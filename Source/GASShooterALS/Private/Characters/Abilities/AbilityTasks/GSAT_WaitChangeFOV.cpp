// Copyright 2020 Dan Kestranek.
//modified by Dan Groom

#include "Characters/Abilities/AbilityTasks/GSAT_WaitChangeFOV.h"
#include "Characters/GSCharacterBase.h"
#include "Curves/CurveFloat.h"
#include "GASShooterALS/GASShooterALS.h"

UGSAT_WaitChangeFOV::UGSAT_WaitChangeFOV(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
	bIsFinished = false;
}

UGSAT_WaitChangeFOV* UGSAT_WaitChangeFOV::WaitChangeFOV(UGameplayAbility* OwningAbility, FName TaskInstanceName, class AGSCharacterBase* AffectedCharacter, float TargetFOV, float Duration, UCurveFloat* OptionalInterpolationCurve)
{
	UGSAT_WaitChangeFOV* MyObj = NewAbilityTask<UGSAT_WaitChangeFOV>(OwningAbility, TaskInstanceName);

	MyObj->Character = AffectedCharacter;
	if (AffectedCharacter != nullptr)
	{
		MyObj->StartFOV = MyObj->Character->GetFOV();
	}

	MyObj->TargetFOV = TargetFOV;
	MyObj->Duration = FMath::Max(Duration, 0.001f);		// Avoid negative or divide-by-zero cases
	MyObj->TimeChangeStarted = MyObj->GetWorld()->GetTimeSeconds();
	MyObj->TimeChangeWillEnd = MyObj->TimeChangeStarted + MyObj->Duration;
	MyObj->LerpCurve = OptionalInterpolationCurve;

	return MyObj;
}

void UGSAT_WaitChangeFOV::Activate()
{
}

void UGSAT_WaitChangeFOV::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);

	if (Character)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentTime >= TimeChangeWillEnd)
		{
			bIsFinished = true;

			Character->SetFOV(TargetFOV);
			
			if (ShouldBroadcastAbilityTaskDelegates())
			{
				OnTargetFOVReached.Broadcast();
			}
			EndTask();
		}
		else
		{
			float NewFOV;

			float MoveFraction = (CurrentTime - TimeChangeStarted) / Duration;
			if (LerpCurve)
			{
				MoveFraction = LerpCurve->GetFloatValue(MoveFraction);
			}

			NewFOV = FMath::Lerp<float, float>(StartFOV, TargetFOV, MoveFraction);

			Character->SetFOV(NewFOV);
		}
	}
	else
	{
		bIsFinished = true;
		EndTask();
	}
}

void UGSAT_WaitChangeFOV::OnDestroy(bool AbilityIsEnding)
{
	Super::OnDestroy(AbilityIsEnding);
}
