
#include "Demo3MovementComponent.h"

#include "ApexClothingUtils.h"
#include "Demo3Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"

// Debug Helper Macro
#if 1
float MacroDuration = 2.f;
#define SLOG(x, c) GEngine->AddOnScreenDebugMessage(-1, MacroDuration ? MacroDuration : -1.f, FColor::c, x);
#define FLOG(x, c) GEngine->AddOnScreenDebugMessage(-1, MacroDuration ? MacroDuration : -1.f, FColor::c, FString::SanitizeFloat(x));
#define POINT(x, c) DrawDebugPoint(GetWorld(), x, 10, FColor::c, !MacroDuration, MacroDuration);
#define LINE(x1, x2, c) DrawDebugLine(GetWorld(), x1, x2, FColor::c, !MacroDuration, MacroDuration);
#define CAPSULE(x, c) DrawDebugCapsule(GetWorld(), x, CapHH(), CapR(), FQuat::Identity, FColor::c, !MacroDuration, MacroDuration);

#else
#define SLOG(x, c)
#define FLOG(x, c)
#define POINT(x, c)
#define LINE(x1, x2, c)
#define CAPSULE(x, c)

#endif

UDemo3MovementComponent::UDemo3MovementComponent()
{
	NavAgentProps.bCanCrouch = true;
}

void UDemo3MovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	Demo3CharacterOwner = Cast<ADemo3Character>(GetOwner());
	//DashStartDelegate.BindUObject(this, &UDemo3MovementComponent::PerformDash);
	
};

// Movement PipeLine
#pragma region Movement PipeLine

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
	case CMOVE_Slide:
		return SlideMaxSpeed;

		default:
		return -1.f;
	}
}

bool UDemo3MovementComponent::IsServer() const
{
	return CharacterOwner->HasAuthority();
}
UCapsuleComponent* UDemo3MovementComponent::Cap() const
{
	return CharacterOwner->GetCapsuleComponent();
};
float UDemo3MovementComponent::CapR() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}
float UDemo3MovementComponent::CapHH() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

// Блок срабатывает каждый фрейм перед обновлением физики
void UDemo3MovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Slide
	if (MovementMode == MOVE_Walking )
	{
		FHitResult PotentialSurface;
		
		if (Velocity.SizeSquared() > pow(SlideMinSpeed, 2) && GetSlideSurface(PotentialSurface))
		{
			// Переворачиваем флаг крутого склона. Если склон слишком крутой, то входим в слайд автоматически 
			CurrentSurfaceAngle = FVector::UpVector | PotentialSurface.Normal;
			(PotentialSurface.Normal | Velocity) < FMath::Cos(FMath::DegreesToRadians(SlideSlopeAngle)) ? bUpSlope = true : bUpSlope = false;

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
			
		}
	}

	// Try Mantle
	if (Demo3CharacterOwner->bPressedDemo3Jump)
	{
		if (TryMantle())
		{
			Demo3CharacterOwner->StopJumping();
		}
		else
		{
			SLOG("Failed Mantle, Reverting to jump", Red)
			
			Demo3CharacterOwner->bPressedDemo3Jump = false;
			CharacterOwner->bPressedJump = true;
			CharacterOwner->CheckJumpInput(DeltaSeconds);
			//bOrientRotationToMovement = true;
		}
	}

	// Transition Mantle
	if (Safe_bTransitionFinished)
	{
		SLOG("Transition Finished", Yellow)
		UE_LOG(LogTemp, Warning, TEXT("FINISHED RM"))
		
		if (IsValid(TransitionQueuedMontage))
		{
			SetMovementMode(MOVE_Flying);
			CharacterOwner->PlayAnimMontage(TransitionQueuedMontage, TransitionQueuedMontageSpeed);
			TransitionQueuedMontageSpeed = 0.f;
			TransitionQueuedMontage = nullptr;
		}
		else
		{
			SetMovementMode(MOVE_Walking);
		}
		Safe_bTransitionFinished = false;
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

// Блок срабатывает каждый фрейм. Используем для представления физики того или иного MovementMode
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

// Блок срабатывает каждый фрейм после обновления движения
void UDemo3MovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

	if (!HasAnimRootMotion() && Safe_bHadAnimRootMotion && IsMovementMode(MOVE_Flying))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ending Anim Root Motion"))
		SetMovementMode(MOVE_Walking);
		bTallMantle = false;
	}

	if (GetRootMotionSourceByID(TransitionRMS_ID) && GetRootMotionSourceByID(TransitionRMS_ID)->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{
		RemoveRootMotionSourceByID(TransitionRMS_ID);
		Safe_bTransitionFinished = true;
	}

	Safe_bHadAnimRootMotion = HasAnimRootMotion();
}

// Блок срабатывает каждый фрейм после смены MovementMode
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

// Movement Event
// Блок срабатывает когда меняется MovementMode
void UDemo3MovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Slide)
	{
		ExitSlide();	
	}
	if (IsCustomMovementMode(CMOVE_Slide))
	{
		EnterSlide();	
	}
	
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
	
	SetMovementMode(MOVE_Flying);
	CharacterOwner->PlayAnimMontage(DashMontage);
	
	UE_LOG(LogClass, Warning, TEXT("PerformDash!"));
}

#pragma endregion

// Slide
#pragma region Slide

bool UDemo3MovementComponent::GetSlideSurface(FHitResult& Hit)
{
	FVector LineTraceStart = UpdatedComponent->GetComponentLocation();
	FVector LineTraceEnd = LineTraceStart + CapHH() * 2.3f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");
	
	// LINE(LineTraceStart, LineTraceEnd, Red) 
	
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

	// Проверка, чтобы не делать слайд вверх по лестнице
	FHitResult FootHit;
	
	FVector FootHitStart = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH() + 10;
	FVector FootHitEnd = FootHitStart + Velocity;
	FName ProfileName = TEXT("BlockAll");
	bool bOnStairs = (FootHit.Normal | FootHitEnd.GetSafeNormal2D()) > -.1f ? false : true;
	
	GetWorld()->LineTraceSingleByProfile(FootHit, FootHitStart, FootHitEnd, ProfileName, Demo3CharacterOwner->GetIgnoreCharacterParams());
	
	// Debug Trace & Log
	/*LINE(Start, End, Orange); 
	FLOG(FootHit.Normal | End.GetSafeNormal2D(), Green);*/

	// Если все условия выполнены, входим в слайд
	if (IsMovingOnGround() && Safe_bWantsToSprint && !bWantsToCrouch && CurrentTime - SlideEndTime >= SlideCooldownDuration && Safe_bWantsToSlide
		&& !bUpSlope && !bOnStairs)
	{
		bWantsToCrouch = true;
		Velocity = Velocity.GetSafeNormal2D() * SlideImpulse;
		SetMovementMode(MOVE_Custom, CMOVE_Slide);	
	}

	// Если склон слишком крутой, то входим в слайд автоматически
	if (CurrentSurfaceAngle < SlideMinSurfaceAngle && !bUpSlope)
	{
		bWantsToCrouch = true;
		Velocity += Velocity.GetSafeNormal2D();
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

// Mantle
#pragma region Mantle

bool UDemo3MovementComponent::GetMantle() const
{
	return bTallMantle;
}

bool UDemo3MovementComponent::TryMantle()
{
	if (!(IsMovementMode(MOVE_Walking) && !IsCrouching()) && !IsMovementMode(MOVE_Falling)) return false;

	// Helper Variables
	FVector BaseLoc = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector Fwd = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	auto Params = Demo3CharacterOwner->GetIgnoreCharacterParams();
	float MaxHeight = CapHH() * 2 + MantleReachHeight;
	
	float CosFrontSlope = FMath::Cos(FMath::DegreesToRadians(MantleMinWallSteepnessAngle));
	float CosFrontAngle = FMath::Cos(FMath::DegreesToRadians(MantleMaxAlignmentAngle));
	float CosSurfaceAngle = FMath::Cos(FMath::DegreesToRadians(MantleMaxSurfaceAngle));
	
	SLOG("Starting Mantle Attempt", Yellow)

	// Check Front Face
	FHitResult FrontHit;
	float CheckDistance = FMath::Clamp(Velocity | Fwd, CapR() + 30, MantleMaxDistance);
	FVector FrontStart = BaseLoc + FVector::UpVector * (MaxStepHeight - 1);

	for (int i = 0; i < 6; i++)
	{
		LINE(FrontStart, FrontStart + Fwd * CheckDistance, Red)
		if (GetWorld()->LineTraceSingleByProfile(FrontHit, FrontStart, FrontStart + Fwd * CheckDistance, "BlockAll", Params)) break;
		FrontStart += FVector::UpVector * (2.f * CapHH() - (MaxStepHeight - 1)) / 5;
		
	}
	if (!FrontHit.IsValidBlockingHit()) return false;
	
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	
	if (FMath::Abs(CosWallSteepnessAngle) > CosFrontSlope || (Fwd | -FrontHit.Normal) < CosFrontAngle)
	{
		
		/*FLOG(FMath::Abs(CosWallSteepnessAngle), Red)
		SLOG("Current COS:", Red)

		FLOG(CosFrontSlope, Purple)
		SLOG("Treshold COS:", Purple)*/
		
		return false;
	}
	else
	{
		POINT(FrontHit.Location, Green)
		LINE(FrontStart, FrontStart + Fwd * CheckDistance, Green)

		/*FLOG(CosFrontSlope, Purple)
		SLOG("Treshold COS:", Purple)
		
		FLOG(CosWallSteepnessAngle, Green)
		SLOG("Current COS:", Green)*/
		
	}
	
	// Check Height
	TArray<FHitResult> HeightHits;
	FHitResult SurfaceHit;
	
	FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, FrontHit.Normal).GetSafeNormal();
	float WallCos = FVector::UpVector | FrontHit.Normal;
	float WallSin = FMath::Sqrt(1 - WallCos * WallCos);
	FVector TraceStart = FrontHit.Location + Fwd + WallUp * (MaxHeight - (MaxStepHeight - 1)) / WallSin;
	
	LINE(TraceStart, FrontHit.Location + Fwd, FColor::Orange)
	
	if (!GetWorld()->LineTraceMultiByProfile(HeightHits, TraceStart, FrontHit.Location + Fwd, "BlockAll", Params)) return false;
	for (const FHitResult& Hit : HeightHits)
	{
		if (Hit.IsValidBlockingHit())
		{
			SurfaceHit = Hit;
			break;	
		}
		
	}
	if (!SurfaceHit.IsValidBlockingHit() || (SurfaceHit.Normal | FVector::UpVector) < CosSurfaceAngle) return false;

	float Height = (SurfaceHit.Location - BaseLoc) | FVector::UpVector;
	
	SLOG(FString::Printf(TEXT("Height: %f"), Height), Orange)
	POINT(SurfaceHit.Location, Blue);
	
	if (Height > MaxHeight) return false;
	
	// Check Clearance
	float SurfaceCos = FVector::UpVector | SurfaceHit.Normal;
	float SurfaceSin = FMath::Sqrt(1 - SurfaceCos * SurfaceCos);
	
	FVector ClearCapLoc = SurfaceHit.Location + Fwd * CapR() + FVector::UpVector * (CapHH() + 1 + CapR() * 2 * SurfaceSin);
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR(), CapHH());
	
	if (GetWorld()->OverlapAnyTestByProfile(ClearCapLoc, FQuat::Identity, "BlockAll", CapShape, Params))
	{
		CAPSULE(ClearCapLoc, FColor::Red)
		return false;
	}
	else
	{
		CAPSULE(ClearCapLoc, FColor::Green)
	}
	
	SLOG("Can Mantle", Yellow)

	// Mantle Selection
	FVector ShortMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, false);
	FVector TallMantleTarget = GetMantleStartLocation(FrontHit, SurfaceHit, true);
	
	bTallMantle = false;
	if (IsMovementMode(MOVE_Walking) && Height > CapHH() * 2)
	{
		bTallMantle = true;

		SLOG("Tall Mantle", Yellow)
	}
	else if (IsMovementMode(MOVE_Falling) && (Velocity | FVector::UpVector) < 0)
	{
		if (!GetWorld()->OverlapAnyTestByProfile(TallMantleTarget, FQuat::Identity, "BlockAll", CapShape, Params))
		{
			bTallMantle = true;
			SLOG("Tall Mantle", Yellow)
		}
	}
	FVector TransitionTarget = bTallMantle ? TallMantleTarget : ShortMantleTarget;
	
	CAPSULE(TransitionTarget, FColor::Yellow)
	
	CAPSULE(UpdatedComponent->GetComponentLocation(), FColor::Red)
	SLOG("Tried Mantle",Yellow)
	
	// Perform Transition to Mantle
	CAPSULE(UpdatedComponent->GetComponentLocation(), FColor::Red)

	float UpSpeed = Velocity | FVector::UpVector;
	float TransDistance = FVector::Dist(TransitionTarget, UpdatedComponent->GetComponentLocation());

	TransitionQueuedMontageSpeed = FMath::GetMappedRangeValueClamped(FVector2D(-500, 750), FVector2D(.9f, 1.2f), UpSpeed);
	TransitionRMS.Reset();
	TransitionRMS = MakeShared<FRootMotionSource_MoveToForce>();
	TransitionRMS->AccumulateMode = ERootMotionAccumulateMode::Override;
	
	TransitionRMS->Duration = FMath::Clamp(TransDistance / 500.f, .1f, .25f);
	SLOG(FString::Printf(TEXT("Duration: %f"), TransitionRMS->Duration), Yellow)
	
	TransitionRMS->StartLocation = UpdatedComponent->GetComponentLocation();
	TransitionRMS->TargetLocation = TransitionTarget;

	// Apply Transition Root Motion Source
	Velocity = FVector::ZeroVector;
	SetMovementMode(MOVE_Flying);
	TransitionRMS_ID = ApplyRootMotionSource(TransitionRMS);

	// Animations
	if (bTallMantle)
	{
		TransitionQueuedMontage = TallMantleMontage;
		CharacterOwner->PlayAnimMontage(TransitionTallMantleMontage, 1 / TransitionRMS->Duration);
	}
	else
	{
		TransitionQueuedMontage = ShortMantleMontage;
		CharacterOwner->PlayAnimMontage(TransitionShortMantleMontage, 1 / TransitionRMS->Duration);
	}
	
	return true;
}

FVector UDemo3MovementComponent::GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit,
	bool TallMantle) const
{
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	float DownDistance = TallMantle ? CapHH() * 2.f : MaxStepHeight - 1;
	FVector EdgeTangent = FVector::CrossProduct(SurfaceHit.Normal, FrontHit.Normal).GetSafeNormal();

	FVector MantleStart = SurfaceHit.Location;
	MantleStart += FrontHit.Normal.GetSafeNormal2D() * (2.f + CapR());
	MantleStart += UpdatedComponent->GetForwardVector().GetSafeNormal2D().ProjectOnTo(EdgeTangent) * CapR() * .3f;
	MantleStart += FVector::UpVector * CapHH();
	MantleStart += FVector::DownVector * DownDistance;
	MantleStart += FrontHit.Normal.GetSafeNormal2D() * CosWallSteepnessAngle * DownDistance;

	return MantleStart;
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

#pragma endregion 
