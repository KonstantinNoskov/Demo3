// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Demo3MovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class DEMO3_API UDemo3MovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	UDemo3MovementComponent();
	
	// Client/Server setups
	#pragma region Client/Server
	
	class FSavedMove_Demo3 : public FSavedMove_Character
	{
		typedef FSavedMove_Character Super;
	
		uint8 Saved_bWantsToSprint:1;
	
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
		
	};

	class FNetworkPredictionData_Client_Demo3 : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Demo3(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

#pragma endregion
	
// Sprint
#pragma region Sprint

	bool Safe_bWantsToSprint;
	
public:
	UFUNCTION(BlueprintCallable) void SprintPressed();
	UFUNCTION(BlueprintCallable) void SprintReleased();

	UPROPERTY(EditDefaultsOnly) float Sprint_MaxWalkSpeed;
	float Base_MaxWalkSpeed;

protected:
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

#pragma endregion 
};
