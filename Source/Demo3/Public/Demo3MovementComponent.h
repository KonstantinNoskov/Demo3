#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Demo3MovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDashStartDelegate);


UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None		UMETA(Hidden),
	CMOVE_Walk		UMETA(DisplayName = "Walk"),
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

// Parameters
#pragma region Params
	
protected:
	virtual void InitializeComponent() override;

	// Walk
	UPROPERTY(EditDefaultsOnly, Category=Sprint) float Walk_MaxSpeed = 200.f;
	float BaseSpeed = GetMaxSpeed();
	
	// Sprint
	UPROPERTY(EditDefaultsOnly, Category=Sprint) float Sprint_MaxSpeed = 750.f;
	float Base_MaxWalkSpeed = MaxWalkSpeed;

	// Dash
	UPROPERTY(EditDefaultsOnly, Category=Dash) float DashImpulse = 1000.f;
	UPROPERTY(EditDefaultsOnly, Category=Dash) float DashCooldownDuration = 1.f;
	
	// Safe Variables
	bool Safe_bWantsToSprint;
	bool Safe_bWantsToWalk;
	bool Safe_bWantsToDash;

	// Working Variables
	float DashStartTime;
	
	// Transient
	UPROPERTY(Transient) ADemo3Character* Demo3CharacterOwner;

	// Delegates
	FDashStartDelegate DashStartDelegate;

	// Timers
	FTimerHandle TimerHandle_DashCooldown;


#pragma endregion
	
	// Client/Server setups
	#pragma region Client/Server
	
	class FSavedMove_Demo3 : public FSavedMove_Character
	{
	public:
		enum CompressedFlags
		{
			FLAG_Walk		= 0x10,
			FLAG_Sprint		= 0x20,
			FLAG_Dash		= 0x40,
			FLAG_Custom_3	= 0x80
		};
		
		typedef FSavedMove_Character Super;
	
		// Server's Flags
		uint8 Saved_bWantsToWalk:1;
		uint8 Saved_bWantsToSprint:1;
		uint8 Saved_bWantsToDash:1;
		
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

#pragma endregion 

// Helping functions
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
	UFUNCTION(BlueprintPure)bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;


#pragma endregion

// Input Interface
#pragma region Sprint

public:
	UFUNCTION(BlueprintCallable) void WalkToggle();
	
	UFUNCTION(BlueprintCallable) void SprintPressed();
	UFUNCTION(BlueprintCallable) void SprintReleased();

	UFUNCTION(BlueprintCallable) void CrouchPressed();

	UFUNCTION(BlueprintCallable) void DashPressed();
	UFUNCTION(BlueprintCallable) void DashReleased();

#pragma endregion
	
// Sprint
#pragma region Sprint

#pragma endregion

// Crouch
#pragma region Crouch
	

#pragma endregion

// Dash
#pragma region Dash
	
private:
	void OnDashCooldown();
	bool CanDash() const;
	void PerformDash();

#pragma endregion 
	 
};


