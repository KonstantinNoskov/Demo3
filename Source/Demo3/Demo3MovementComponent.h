#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Demo3MovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None		UMETA(Hidden),
	CMOVE_Slide		UMETA(DisplayName = "Slide"),
	CMOVE_MAX		UMETA(Hidden)
};

class ADemo3Character;

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

		// This is a Flag...
		uint8 Saved_bWantsToSprint:1;
		uint8 Saved_bWantsToWalk:1;
		
		// ... and this is not
		uint8 Saved_bPrevWantsToCrouch:1;
			
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
	
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

		
#pragma endregion

// Walk
#pragma region Walk
public:
	
	void WalkPressed();

	UPROPERTY(EditDefaultsOnly) float Walk_MaxWalkSpeed;
	bool Safe_bWantsToWalk;

#pragma endregion
	
// Sprint
#pragma region Sprint

	bool Safe_bWantsToSprint;
	
public:
	void SprintPressed();
	void SprintReleased();

	UPROPERTY(EditDefaultsOnly) float Sprint_MaxWalkSpeed;
	float Base_MaxWalkSpeed;

#pragma endregion

//Crouch
#pragma region Crouch
	
public:
	void CrouchPressed();

#pragma endregion
	
// Slide
#pragma region Slide

private:
	// Transient
	UPROPERTY(Transient)
	
	ADemo3Character* Demo3CharacterOwner;
	
	//bool Safe_bPrevWantsToCrouch;

protected:
	virtual void InitializeComponent() override;

	UFUNCTION(BlueprintPure) bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;

	//Params
	UPROPERTY(EditDefaultsOnly) float Slide_MinSpeed = 350;
	UPROPERTY(EditDefaultsOnly) float Slide_EnterImpulse = 500;
	UPROPERTY(EditDefaultsOnly) float Slide_GravityForce = 5000;
	UPROPERTY(EditDefaultsOnly) float Slide_Friction = 1.3;
	UPROPERTY(EditDefaultsOnly) int Slide_MaxCount = 1;
	int Slide_CurrentCount = Slide_MaxCount;

private:
	
	void ExitSlide();
	void PhysSlide(float deltaTime, int32 Iterations);
	bool GetSlideSurface(FHitResult& Hit) const;
	

public:
	void EnterSlide();

#pragma endregion 


};


