
#include "Demo3MovementComponent.h"

#include "Demo3Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

UDemo3MovementComponent::UDemo3MovementComponent()
{
	Base_MaxWalkSpeed = MaxWalkSpeed;
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

	if (Saved_bWantsToSprint != NewDemo3Move->Saved_bWantsToSprint)
	{
		return false;
	}
	
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UDemo3MovementComponent::FSavedMove_Demo3::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bWantsToSprint = 0;
}

uint8 UDemo3MovementComponent::FSavedMove_Demo3::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (Saved_bWantsToSprint) Result |= FLAG_Custom_0;

	return Result;
}

void UDemo3MovementComponent::FSavedMove_Demo3::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	UDemo3MovementComponent* Demo3MovementComponent = Cast<UDemo3MovementComponent>(C->GetCharacterMovement());

	Saved_bWantsToSprint = Demo3MovementComponent->Safe_bWantsToSprint;
	//Saved_bPrevWantsToCrouch = Demo3MovementComponent->Safe_bPrevWantsToCrouch;
	Saved_bWantsToWalk = Demo3MovementComponent->Safe_bWantsToWalk;
}

void UDemo3MovementComponent::FSavedMove_Demo3::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	UDemo3MovementComponent* Demo3MovementComponent = Cast<UDemo3MovementComponent>(C->GetCharacterMovement());

	Demo3MovementComponent->Safe_bWantsToSprint = Saved_bWantsToSprint;
	//Demo3MovementComponent->Safe_bPrevWantsToCrouch = Saved_bPrevWantsToCrouch;
	Demo3MovementComponent->Safe_bWantsToWalk = Saved_bWantsToWalk;
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

	Safe_bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

#pragma endregion

// General function's
#pragma region General 
void UDemo3MovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (MovementMode == MOVE_Walking)
	{
		if (Safe_bWantsToSprint)
		{
			
			MaxWalkSpeed = Sprint_MaxWalkSpeed;
		}

		else if (Safe_bWantsToWalk)
		{
			MaxWalkSpeed = Walk_MaxWalkSpeed;
		}
		
		else
		{
			MaxWalkSpeed = Base_MaxWalkSpeed;
		}
	}
	
	//Safe_bPrevWantsToCrouch = bWantsToCrouch;
}

/*void UDemo3MovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if (MovementMode == MOVE_Walking && !bWantsToCrouch && Safe_bPrevWantsToCrouch)
	{
		FHitResult PotentialSlideSurface;
		if (Velocity.SizeSquared() > pow(Slide_MinSpeed, 2) && GetSlideSurface(PotentialSlideSurface))
		{
			EnterSlide();
		}
	}

	if (IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch)
	{
		ExitSlide();
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}*/


// Вызываем актуальную физику персонажа в зависимсоти от CMM
void UDemo3MovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime, Iterations);
		UE_LOG(LogTemp, Warning, TEXT("Sliding!"))
		break;
		
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
	}
}

// При слайде персонаж также передвигается по земле
bool UDemo3MovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide);
}

// Опиционально. Персонаж не может присядать в воздухе 
bool UDemo3MovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() && IsMovingOnGround();
}

#pragma endregion

//Walk
#pragma region Walk

void UDemo3MovementComponent::WalkPressed()
{
	Safe_bWantsToWalk = !Safe_bWantsToWalk;
}

#pragma endregion 

// Sprint
#pragma region Sprint

void UDemo3MovementComponent::SprintPressed()
{

	Safe_bWantsToSprint = true; 	
	UE_LOG(LogTemp,Warning, TEXT("Sprint!"))
}
void UDemo3MovementComponent::SprintReleased()
{	
	Safe_bWantsToSprint = false;
}

#pragma endregion 

// Crouch
#pragma region Crouch

void UDemo3MovementComponent::CrouchPressed()
{
	bWantsToCrouch = !bWantsToCrouch;
}

#pragma endregion

// Slide
#pragma region Slide

bool UDemo3MovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}

void UDemo3MovementComponent::EnterSlide()
{
	if (Slide_CurrentCount && IsMovingOnGround())
	{
		bWantsToCrouch = true;
		Velocity += Velocity.GetSafeNormal2D() * Slide_EnterImpulse;
		SetMovementMode(MOVE_Custom, CMOVE_Slide);
		UE_LOG(LogTemp, Warning, TEXT("Sliding!"));
		Slide_CurrentCount--; 
	}
}

void UDemo3MovementComponent::ExitSlide()
{
	bWantsToCrouch = false;

	FQuat NewRotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, true, Hit);
	SetMovementMode(MOVE_Walking);
	Slide_CurrentCount = Slide_MaxCount;
}

// Phys Implementation
void UDemo3MovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();
	
	FHitResult SurfaceHit;
	// Если под ногами нет земли или скорость ниже мин.скорости слайда...
	if (!GetSlideSurface(SurfaceHit) || Velocity.SizeSquared() < pow(Slide_MinSpeed, 2))
	{
		// ... выходим из состояния слайда ...
		ExitSlide();
		// ... и применяем на персонажа актуальную для текущего состояния персонажа физику
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	// Surface Gravity
	
	// Гравитация будет тянуть нашего персонажа вниз
	Velocity += Slide_GravityForce * FVector::DownVector * deltaTime;

	// Strafe

	// Ограничивает угол стрейфа ~45 градусами перед собой, чтобы персонаж не мог стрейфиться вверх по склону или строго вправо/влево...
	if (FMath::Abs(FVector::DotProduct(Acceleration.GetSafeNormal(), UpdatedComponent->GetRightVector())) > .5)
	{
		// ... если пытается стрейфиться в этом диапазоне, то применяем ускорение.  
		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector() * 2.f);
	}
	else
	{
		// Если игрок пытается катиться/стрейфиться в сторону обратную спуску его ускоренние будет равно 0, т.е. он не сможет этого сделать
		Acceleration = FVector::ZeroVector;
	}

	// Calc Velocity
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(deltaTime, Slide_Friction, true, GetMaxBrakingDeceleration());
	}
	
	ApplyRootMotionToVelocity(deltaTime);
	
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

	// Если персонаж врезался в стену во время слайда он продолжит движение в параллельно препятствию
	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	FHitResult NewSurfaceHit;
	if (!GetSlideSurface(NewSurfaceHit) || Velocity.SizeSquared() < pow(Slide_MinSpeed, 2))
	{
		ExitSlide();
	}

	// Update Outgoing Velocity & Acceleration
	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
}

bool UDemo3MovementComponent::GetSlideSurface(FHitResult& Hit) const
{
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");
	
	return GetWorld()->LineTraceSingleByProfile(Hit, Start, End, ProfileName, Demo3CharacterOwner->GetIgnoreCharacterParams());
}


#pragma endregion 
