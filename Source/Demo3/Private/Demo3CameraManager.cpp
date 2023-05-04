// Fill out your copyright notice in the Description page of Project Settings.


#include "Demo3CameraManager.h"

#include "Demo3Character.h"
#include "Demo3MovementComponent.h"
#include "Components/CapsuleComponent.h"

ADemo3CameraManager::ADemo3CameraManager()
{
}

void ADemo3CameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);
	
	// Crouch Camera
	if (ADemo3Character* Demo3Character = Cast<ADemo3Character>(GetOwningPlayerController()->GetPawn()))
	{
		UDemo3MovementComponent* DMC = Demo3Character->GetDemo3MovementComponent();
		FVector TargetCrouchOffset = FVector(
			0,
			0,
			DMC->GetCrouchedHalfHeight() - Demo3Character->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

		FVector Offset = FMath::Lerp(FVector::ZeroVector, TargetCrouchOffset, FMath::Clamp(CrouchBlendTime/CrouchBlendDuration, 0.f, 1.f));

		if (DMC->IsCrouching())
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime + DeltaTime, 0.f, CrouchBlendDuration);
			Offset -= TargetCrouchOffset;
		}
		else
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime - DeltaTime, 0.f, CrouchBlendDuration);
		}

		if (DMC->IsMovingOnGround())
		{
			OutVT.POV.Location += Offset;
		}
	}
}
