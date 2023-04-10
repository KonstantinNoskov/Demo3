
#include "Demo3MovementComponent.h"

#include "Demo3Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

UDemo3MovementComponent::UDemo3MovementComponent()
{
	NavAgentProps.bCanCrouch = true;
}

void UDemo3MovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	Demo3CharacterOwner = Cast<ADemo3Character>(GetOwner());
};

// Client/Server setups
#pragma region Client/Server

bool UDemo3MovementComponent::FSavedMove_Demo3::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter,
	float MaxDelta) const
{
	FSavedMove_Demo3* NewDemo3Move = static_cast<FSavedMove_Demo3*>(NewMove.Get());

	if (Saved_bWantsToWalk != NewDemo3Move->Saved_bWantsToWalk)		return false;
	if (Saved_bWantsToSprint != NewDemo3Move->Saved_bWantsToSprint) return false;
	if (Saved_bWantsToDash != NewDemo3Move->Saved_bWantsToDash)		return false;
	
	
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UDemo3MovementComponent::FSavedMove_Demo3::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bWantsToWalk		= 0;
	Saved_bWantsToSprint	= 0;
	Saved_bWantsToDash		= 0;
	
}

uint8 UDemo3MovementComponent::FSavedMove_Demo3::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (Saved_bWantsToDash ) Result		|= FLAG_Walk;
	if (Saved_bWantsToSprint) Result	|= FLAG_Sprint;
	if (Saved_bWantsToDash ) Result		|= FLAG_Dash;
	
	return Result;
}

void UDemo3MovementComponent::FSavedMove_Demo3::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	UDemo3MovementComponent* Demo3MovementComponent = Cast<UDemo3MovementComponent>(C->GetCharacterMovement());

	Saved_bWantsToWalk = Demo3MovementComponent->Safe_bWantsToWalk;
	Saved_bWantsToSprint = Demo3MovementComponent->Safe_bWantsToSprint;
	Saved_bWantsToDash = Demo3MovementComponent->Safe_bWantsToDash;
}

void UDemo3MovementComponent::FSavedMove_Demo3::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UDemo3MovementComponent* Demo3MovementComponent = Cast<UDemo3MovementComponent>(C->GetCharacterMovement());

	Demo3MovementComponent->Safe_bWantsToWalk = Saved_bWantsToWalk;
	Demo3MovementComponent->Safe_bWantsToSprint = Saved_bWantsToSprint;
	Demo3MovementComponent->Safe_bWantsToDash = Saved_bWantsToDash;
}

UDemo3MovementComponent::FNetworkPredictionData_Client_Demo3::FNetworkPredictionData_Client_Demo3(
	const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
}

FSavedMovePtr UDemo3MovementComponent::FNetworkPredictionData_Client_Demo3::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Demo3());
}

FNetworkPredictionData_Client* UDemo3MovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if (ClientPredictionData == nullptr)
	{
		UDemo3MovementComponent* MutableThis = const_cast<UDemo3MovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Demo3(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}
	return ClientPredictionData;
}

void UDemo3MovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToWalk = (Flags & FSavedMove_Demo3::FLAG_Walk)		!= 0;
	Safe_bWantsToSprint = (Flags & FSavedMove_Demo3::FLAG_Sprint)	!= 0;
	Safe_bWantsToDash = (Flags & FSavedMove_Demo3::FLAG_Dash)		!= 0;
}

#pragma endregion

// General function's
#pragma region General

// Getters / Helpers
bool UDemo3MovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround();
}
bool UDemo3MovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() && IsMovingOnGround();
}
bool UDemo3MovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}
bool UDemo3MovementComponent::IsMovementMode(EMovementMode InMovementMode) const
{
	return MovementMode == InMovementMode;
}
float UDemo3MovementComponent::GetMaxSpeed() const
{
	if (IsMovementMode(MOVE_Walking) && Safe_bWantsToSprint && !IsCrouching()) return Sprint_MaxSpeed;
	if (IsMovementMode(MOVE_Walking) && Safe_bWantsToWalk && !IsCrouching()) return Walk_MaxSpeed;

	if (MovementMode != MOVE_Custom) return Super::GetMaxSpeed();

	switch (CustomMovementMode)
	{
	case CMOVE_Sprint:
		return Sprint_MaxSpeed;
	case CMOVE_Walk:
		return Walk_MaxSpeed;
		
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
		return -1.f;
	}
}

// Movement PipeLine
void UDemo3MovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if (Safe_bWantsToDash && CanDash())
	{
		PerformDash();
		Safe_bWantsToDash = false;
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}
void UDemo3MovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (IsCustomMovementMode(CMOVE_Sprint))
	{
		if (Safe_bWantsToSprint)
		{
			MaxWalkSpeed = Sprint_MaxSpeed;
		}
		else if (Safe_bWantsToWalk)
		{
			MaxWalkSpeed = Walk_MaxSpeed;
		}
		else 
		{
			MaxWalkSpeed = Base_MaxWalkSpeed;
		}
	}
	/*if (IsMovementMode(MOVE_Flying) && !HasRootMotionSources())
	{
		SetMovementMode(MOVE_Walking);
	}*/
	if(!CharacterOwner->IsPlayingRootMotion())
	{
		bIsDashing = false;
	}
	
}
void UDemo3MovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);
	
}

// Movement Event
void UDemo3MovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (IsMovingOnGround())
	{
		DashCurrentCount = DashMaxCount;
		if (DashCurrentCount > DashMaxCount)
		{
			DashCurrentCount = 0;
		}
	}
}

#pragma endregion 

// Input Interface
#pragma region Input Interface

void UDemo3MovementComponent::WalkToggle()
{
	Safe_bWantsToWalk = !Safe_bWantsToWalk;
}

void UDemo3MovementComponent::SprintPressed()
{
	Safe_bWantsToSprint = true; 	
	UE_LOG(LogTemp, Display, TEXT("Is Sprinting!"));
}
void UDemo3MovementComponent::SprintReleased()
{	
	Safe_bWantsToSprint = false;
}

void UDemo3MovementComponent::CrouchToggle()
{
	bWantsToCrouch = !bWantsToCrouch;
}

void UDemo3MovementComponent::DashPressed()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - DashStartTime >= DashCooldownDuration)
	{
		Safe_bWantsToDash = true; 
	}
	else 
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_DashCooldown, this, &UDemo3MovementComponent::OnDashCooldown,
		DashCooldownDuration - (CurrentTime - DashStartTime));
		UE_LOG(LogClass, Warning, TEXT("Timer is Set!"));
	}
}
void UDemo3MovementComponent::DashReleased()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashCooldown);
	Safe_bWantsToDash = false;
}

#pragma endregion

// Sprint
#pragma region Sprint


#pragma endregion 
 
// Crouch
#pragma region Crouch


#pragma endregion

// Dash
#pragma region Dash

void UDemo3MovementComponent::OnDashCooldown()
{
	Safe_bWantsToDash = true;
	
}
bool UDemo3MovementComponent::IsDashing() const
{
	return bIsDashing;
}

bool UDemo3MovementComponent::CanDash() const
{
	return DashCurrentCount && (IsWalking() && !IsCrouching() || IsFalling());
}
void UDemo3MovementComponent::PerformDash()
{
	bIsDashing = true;
	DashCurrentCount--;
	
	DashStartTime = GetWorld()->GetTimeSeconds();
	
	//Phys implemintation for non-rootmotion only.
	/*FVector DashDirection = (Acceleration.IsNearlyZero() ? UpdatedComponent->GetForwardVector() : Acceleration).GetSafeNormal2D();
	Velocity += DashImpulse * (DashDirection + FVector::UpVector * .1f);

	FQuat NewRotation = FRotationMatrix::MakeFromXZ(DashDirection, FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, false, Hit);*/

	// -------------------------------------------------
	
	SetMovementMode(MOVE_Falling);
	
	CharacterOwner->PlayAnimMontage(DashMontage);
	
	UE_LOG(LogClass, Warning, TEXT("PerformDash!"));
}

#pragma endregion 