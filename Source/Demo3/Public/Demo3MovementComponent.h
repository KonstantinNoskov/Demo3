#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Demo3MovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None		UMETA(Hidden),
	CMOVE_Sprint	UMETA(DisplayName = "Sprint"),
	CMOVE_Dash		UMETA(DisplayName = "Dash"),
	CMOVE_MAX		UMETA(Hidden)
};

class ADemo3Character;

UCLASS()
class DEMO3_API UDemo3MovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	UDemo3MovementComponent();

#pragma region Params
	
protected:
	virtual void InitializeComponent() override;

	// Parameters
	UPROPERTY(EditDefaultsOnly) float Sprint_MaxSpeed = 750.f;
	float Base_MaxWalkSpeed = MaxWalkSpeed;

	bool Safe_bWantsToSprint;
	bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;
	
	// Transient
	UPROPERTY(Transient) ADemo3Character* Demo3CharacterOwner;

#pragma endregion
	
	// Client/Server setups
	#pragma region Client/Server
	
	class FSavedMove_Demo3 : public FSavedMove_Character
	{
	public:
		enum CompressedFlags
		{
			FLAG_Sprint		= 0x10,
			FLAG_Dash		= 0x20,
			FLAG_Custom_2	= 0x40,
			FLAG_Custom_3	= 0x80
		};
		
		typedef FSavedMove_Character Super;
	
		// This is a Flag...
		uint8 Saved_bWantsToSprint:1;
		
		// Other Variables
		
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


// General helping function's
public:
	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual float GetMaxSpeed() const override;
	
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	bool IsMovementMode(EMovementMode InMovementMode) const;

#pragma endregion
	
// Sprint
#pragma region Sprint
	
public:
	void SprintPressed();
	void SprintReleased();

#pragma endregion

//Crouch
#pragma region Crouch
	
public:
	UFUNCTION(BlueprintCallable) void CrouchPressed();

#pragma endregion

#pragma endregion
};


