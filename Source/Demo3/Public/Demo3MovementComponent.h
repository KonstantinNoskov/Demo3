#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Demo3MovementComponent.generated.h"

DECLARE_DELEGATE(FDashStartDelegate);

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None		UMETA(Hidden),
	CMOVE_Walk		UMETA(DisplayName = "Walk"),
	CMOVE_Sprint	UMETA(DisplayName = "Sprint"),
	CMOVE_Dash		UMETA(DisplayName = "Dash"),
	CMOVE_Slide		UMETA(DisplayName = "Slide"),
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

	// Walk / Sprint
	UPROPERTY(EditDefaultsOnly, Category=Walk) float Walk_MaxSpeed = 200.f;
	UPROPERTY(EditDefaultsOnly, Category=Sprint) float Sprint_MaxSpeed = 750.f;

	float BaseSpeed = MaxWalkSpeed;

	// Dash
	UPROPERTY(EditDefaultsOnly, Category=Dash) float DashImpulse = 1000.f;
	UPROPERTY(EditDefaultsOnly, Category=Dash) float DashCooldownDuration = 1.f;
	UPROPERTY(EditDefaultsOnly, Category=Dash) float AuthDashCooldownDuration=.9f;
	UPROPERTY(EditDefaultsOnly, Category=Dash) UAnimMontage* DashMontage;
	UPROPERTY(EditDefaultsOnly, Category=Dash) uint8 DashMaxCount = 1;

	uint8 DashCurrentCount = DashMaxCount;
	float DashStartTime;
	bool bIsDashing;
	
	// Slide
	UPROPERTY(EditDefaultsOnly, Category=Slide) float SlideImpulse = 500.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float SlideMinSpeed = 300.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float SlideMaxSpeed = 900.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float SlideGravityForce = 4000.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float SlideFriction = 1.3f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float SlideCooldownDuration = 1.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float SlideMinSurfaceAngle = 0.9f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float SlideSlopeAngle = 90;

	float SlideEndTime;
	float CurrentSurfaceAngle;
	bool bUpSlope;
	
	// Mantle
	UPROPERTY(EditDefaultsOnly, Category=Mantle) float MantleMaxDistance = 200;
	UPROPERTY(EditDefaultsOnly, Category=Mantle) float MantleReachHeight = 50;
	UPROPERTY(EditDefaultsOnly, Category=Mantle) float MinMantleDepth = 30;
	UPROPERTY(EditDefaultsOnly, Category=Mantle) float MantleMinWallSteepnessAngle = 45;
	UPROPERTY(EditDefaultsOnly, Category=Mantle) float MantleMaxAlignmentAngle = 45;
	UPROPERTY(EditDefaultsOnly, Category=Mantle) float MantleMaxSurfaceAngle = 45;
	
	UPROPERTY(EditDefaultsOnly, Category=Mantle) UAnimMontage* TallMantleMontage;
	UPROPERTY(EditDefaultsOnly, Category=Mantle) UAnimMontage* TransitionTallMantleMontage;
	
	UPROPERTY(EditDefaultsOnly, Category=Mantle) UAnimMontage* ShortMantleMontage;
	UPROPERTY(EditDefaultsOnly, Category=Mantle) UAnimMontage* TransitionShortMantleMontage;
	bool bTallMantle;

	
	bool Safe_bTransitionFinished;
	TSharedPtr<FRootMotionSource_MoveToForce> TransitionRMS;
	UPROPERTY(Transient) UAnimMontage* TransitionQueuedMontage;
	
	float TransitionQueuedMontageSpeed;
	int TransitionRMS_ID;
	
	// Safe Flags
	bool Safe_bWantsToSprint;
	bool Safe_bWantsToWalk;
	bool Safe_bWantsToSlide;
	bool Safe_bWantsToDash;

	bool Safe_bHadAnimRootMotion;

	// Timers
	FTimerHandle TimerHandle_SlideCooldown;
	FTimerHandle TimerHandle_DashCooldown;

	// Delegates;
	FDashStartDelegate DashStartDelegate;
	
	// Transient
	UPROPERTY(Transient) ADemo3Character* Demo3CharacterOwner;
	
#pragma endregion
	
// Helping functions
public:
	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual float GetMaxSpeed() const override;
	
protected:
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	
	bool IsMovementMode(EMovementMode InMovementMode) const;
	UFUNCTION(BlueprintPure)bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;

private:
	bool IsServer() const;
	float CapR() const;
	float CapHH() const;
	UCapsuleComponent* Cap() const;

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
	UFUNCTION(BlueprintPure) bool IsDashing() const;
	bool CanDash() const;
	void OnDashCooldown();
	void PerformDash();

#pragma endregion

// Slide
#pragma region Slide
public:
	void EnterSlide();

private:
	void ExitSlide();
	void PhysSlide(float deltaTime, int32 Iterations);
	bool GetSlideSurface(FHitResult& Hit);
	

#pragma endregion

// Mantle
#pragma region Mantle
public:
	UFUNCTION(BlueprintPure) bool GetMantle() const;
	
private:
	bool TryMantle() ; 
	FVector GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit, bool TallMantle) const;

public:

#pragma endregion 

// Input Interface
#pragma region Sprint

public:
	UFUNCTION(BlueprintCallable) void WalkToggle();
	
	UFUNCTION(BlueprintCallable) void SprintPressed();
	UFUNCTION(BlueprintCallable) void SprintReleased();

	UFUNCTION(BlueprintCallable) void CrouchToggle();

	UFUNCTION(BlueprintCallable) void SlidePressed();
	UFUNCTION(BlueprintCallable) void SlideReleased();
	
	UFUNCTION(BlueprintCallable) void DashPressed();
	UFUNCTION(BlueprintCallable) void DashReleased();

#pragma endregion
	
};


