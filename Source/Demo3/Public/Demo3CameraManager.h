// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "Demo3CameraManager.generated.h"

class UDemo3MovementComponent;

UCLASS()
class DEMO3_API ADemo3CameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:
	ADemo3CameraManager();

	void Zoom();
	
	UPROPERTY(EditDefaultsOnly) float CrouchBlendDuration;
	float CrouchBlendTime;

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
	
};
