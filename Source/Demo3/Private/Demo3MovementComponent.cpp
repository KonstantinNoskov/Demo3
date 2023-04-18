
#include "Demo3MovementComponent.h"

#include "Demo3Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UDemo3MovementComponent::UDemo3MovementComponent()
{
	NavAgentProps.bCanCrouch = true;
	DashStartDelegate.BindUObject(this, &UDemo3MovementComponent::PerformDash);
}

void UDemo3MovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	Demo3CharacterOwner = Cast<ADemo3Character>(GetOwner());
};

// Client/Server setups
#pragma region Client/Server

bool UDemo3MovementComponent::FSavedMove_Demo3::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
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

	Saved_bWantsToWalk		 = 0;
	Saved_bWantsToSprint	 = 0;
	Saved_bWantsToDash		 = 0;
	//Saved_bPrevWantsToCrouch = 0;
}
uint8 UDemo3MovementComponent::FSavedMove_Demo3::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (Saved_bWantsToWalk )  Result	|= FLAG_Walk;
	if (Saved_bWantsToSprint) Result	|= FLAG_Sprint;
	if (Saved_bWantsToDash )  Result	|= FLAG_Dash;
	
	return Result;
}
void UDemo3MovementComponent::FSavedMove_Demo3::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
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
UDemo3MovementComponent::FNetworkPredictionData_Client_Demo3::FNetworkPredictionData_Client_Demo3(const UCharacterMovementComponent& ClientMovement): Super(ClientMovement)
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

#pragma endregion

// General function's
#pragma region General

// Getters / Helpers
bool UDemo3MovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide);
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
	if (MovementMode != MOVE_Custom) return Super::GetMaxSpeed();
	
	switch (CustomMovementMode)
	{
	case CMOVE_Sprint:
		return Sprint_MaxSpeed;
	case CMOVE_Walk:
		return Walk_MaxSpeed;
	default:
		return -1.f;
	}
}

// Movement PipeLine
// Переворачиваем флаги наших кастомных MovementMode
void UDemo3MovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToWalk = (Flags & FSavedMove_Demo3::FLAG_Walk)		!= 0;
	Safe_bWantsToSprint = (Flags & FSavedMove_Demo3::FLAG_Sprint)	!= 0;
	Safe_bWantsToDash = (Flags & FSavedMove_Demo3::FLAG_Dash)		!= 0;
}
// Блок срабатывает каждый фрейм перед обновлением движения
void UDemo3MovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Slide
	if (MovementMode == MOVE_Walking)
	{
		FHitResult PotentialSurface;
		if (Velocity.SizeSquared() > pow(SlideMinSpeed, 2) && GetSlideSurface(PotentialSurface))
		{
			EnterSlide();
		}
	}
	if (IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch)
	{
		ExitSlide();
	}

	// Dash
	bool bAuthProxy = CharacterOwner->HasAuthority() && !CharacterOwner->IsLocallyControlled();
	if (Safe_bWantsToDash && CanDash())
	{
		if (!bAuthProxy || GetWorld()->GetTimeSeconds() - DashStartTime > AuthDashCooldownDuration)
		{
			PerformDash();
			Safe_bWantsToDash = false;
			bIsDashing = true;
			Proxy_bDashStart = true;
		}
	}
	else
	{
		Proxy_bDashStart = false;
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}
// Блок срабатывает каждый фрейм после обновления движения
void UDemo3MovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, 
	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	
	if (MovementMode == MOVE_Walking)
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
			MaxWalkSpeed = BaseSpeed;
		}
	}
	if(!CharacterOwner->IsPlayingRootMotion())
	{
		bIsDashing = false;
	}
	
}
// Блок срабатывает каждый фрейм. Используем для представление физики того или иного MovementMode
void UDemo3MovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime,Iterations);
		Acceleration = FVector::ZeroVector; // <-- Костыль устраняет глич анимации при ускорении в разных направлениях во время слайда 
		break;
	default:
		UE_LOG(LogClass, Display, TEXT("ok"));		
	}
} 

// Movement Event
// Блок срабатывает когда меняется MovementMode
void UDemo3MovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (IsMovingOnGround())
	{
		DashCurrentCount = DashMaxCount;
	}
	
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
	return DashCurrentCount && ((IsWalking() && !IsCrouching()) || IsFalling());
}
void UDemo3MovementComponent::PerformDash()
{
	//bIsDashing = true;
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

// Slide
#pragma region Slide

bool UDemo3MovementComponent::GetSlideSurface(FHitResult& Hit) const
{
	FVector LineTraceStart = UpdatedComponent->GetComponentLocation();
	FVector LineTraceEnd = LineTraceStart + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 3.f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");

	return GetWorld()->LineTraceSingleByProfile(Hit, LineTraceStart, LineTraceEnd, ProfileName, Demo3CharacterOwner->GetIgnoreCharacterParams());
}

void UDemo3MovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{ 
		return;
	}

	RestorePreAdditiveRootMotionVelocity();
	
	FHitResult SurfaceHit;
	 
	if (!GetSlideSurface(SurfaceHit) || Velocity.SizeSquared() < pow(SlideMinSpeed, 2))
	{
		ExitSlide();
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	// Surface Gravity
	Velocity += SlideGravityForce * FVector::DownVector * deltaTime;

	// Strafe
	if (FMath::Abs(FVector::DotProduct(Acceleration.GetSafeNormal(), UpdatedComponent->GetRightVector())) > .5)
	{
		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector());
	}
	else
	{
		Acceleration = FVector::ZeroVector;
	}

	// Calc Velocity
	if(!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(deltaTime, SlideFriction, true, GetMaxBrakingDeceleration());
	}

	// Perform Move
	Iterations++;
	bJustTeleported = false;

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FQuat OldRotation = UpdatedComponent->GetComponentRotation().Quaternion();
	FHitResult Hit(1.f);
	FVector Adjusted = Velocity * deltaTime;
	FVector VelPlaneDir = FVector::VectorPlaneProject(Velocity, SurfaceHit.Normal).GetSafeNormal();
	FQuat NewRotation = FRotationMatrix::MakeFromXZ(VelPlaneDir, SurfaceHit.Normal).ToQuat();
	SafeMoveUpdatedComponent(Adjusted, NewRotation, true, Hit);
	
	// Скользим вдоль стенки
	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted,(1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	FHitResult NewSurfaceHit;
	if (!GetSlideSurface(NewSurfaceHit) || Velocity.SizeSquared() < pow(SlideMinSpeed, 2))
	{
		ExitSlide();
	}

	// Update Outgoing Velocity & Acceleration
	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime; //v = ds / dt
	}
}
void UDemo3MovementComponent::EnterSlide()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	if (IsMovingOnGround() && Safe_bWantsToSprint && !bWantsToCrouch && CurrentTime - SlideEndTime >= SlideCooldownDuration && Safe_bWantsToSlide)
	{
		bWantsToCrouch = true;
		Velocity = Velocity.GetSafeNormal2D() * SlideImpulse;
		SetMovementMode(MOVE_Custom, CMOVE_Slide);	
	}
}
void UDemo3MovementComponent::ExitSlide()
{
	bWantsToCrouch = false;

	// Поворачиваем капсуль прямо перед собой по завершении слайда
	FQuat NewRotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), FVector::UpVector).ToQuat();
	FHitResult Hit;

	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, true, Hit);
	SetMovementMode(MOVE_Walking);
	
	SlideEndTime = GetWorld()->GetTimeSeconds();
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
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("Sprint: %hhd"),Safe_bWantsToSprint),
			true, FVector2D::UnitVector * 1.5);
	}
	
	UE_LOG(LogTemp, Display, TEXT("Is Sprinting!"));
}
void UDemo3MovementComponent::SprintReleased()
{	
	Safe_bWantsToSprint = false;
}

void UDemo3MovementComponent::CrouchToggle()
{
	if (!IsCustomMovementMode(CMOVE_Slide))
	{
		bWantsToCrouch = !bWantsToCrouch;
	}
	
}

void UDemo3MovementComponent::SlidePressed()
{
	Safe_bWantsToSlide = true;
}
void UDemo3MovementComponent::SlideReleased()
{
	Safe_bWantsToSlide = false;
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
	}
}
void UDemo3MovementComponent::DashReleased()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashCooldown);
	Safe_bWantsToDash = false;
}

#pragma endregion	

// Replication
#pragma region Replication

void UDemo3MovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UDemo3MovementComponent, Proxy_bDashStart, COND_SkipOwner)
}

void UDemo3MovementComponent::OnRep_DashStart()
{
	if (Proxy_bDashStart)
	{
		DashStartDelegate.ExecuteIfBound();
		//CharacterOwner->PlayAnimMontage(DashMontage);
	}
}

#pragma endregion 
