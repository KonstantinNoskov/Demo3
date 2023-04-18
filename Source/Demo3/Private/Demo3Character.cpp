#include "Demo3Character.h"

#include "Demo3MovementComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"


//////////////////////////////////////////////////////////////////////////
// ADemo3Character


// Constructor
#pragma region Consctructor

ADemo3Character::ADemo3Character(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDemo3MovementComponent>(ACharacter::CharacterMovementComponentName))
{
	Demo3MovementComponent = Cast<UDemo3MovementComponent>(GetCharacterMovement());
	Demo3MovementComponent->SetIsReplicated(true);

	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	

	// Configure character movement
	Demo3MovementComponent->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	Demo3MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	Demo3MovementComponent->JumpZVelocity = 600.f;
	Demo3MovementComponent->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

#pragma endregion 

void ADemo3Character::BeginPlay()
{
	Super::BeginPlay();
}

// Input
#pragma  region  Input
void ADemo3Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Walk", IE_Pressed, Demo3MovementComponent, &UDemo3MovementComponent::WalkToggle);
	
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, Demo3MovementComponent, &UDemo3MovementComponent::SprintPressed);
	PlayerInputComponent->BindAction("Sprint", IE_Released, Demo3MovementComponent, &UDemo3MovementComponent::SprintReleased);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, Demo3MovementComponent, &UDemo3MovementComponent::CrouchToggle);

	PlayerInputComponent->BindAction("Dash", IE_Pressed, Demo3MovementComponent, &UDemo3MovementComponent::DashPressed);
	PlayerInputComponent->BindAction("Dash", IE_Released, Demo3MovementComponent, &UDemo3MovementComponent::DashReleased);

	PlayerInputComponent->BindAction("Slide", IE_Pressed, Demo3MovementComponent, &UDemo3MovementComponent::SlidePressed);
	PlayerInputComponent->BindAction("Slide", IE_Released, Demo3MovementComponent, &UDemo3MovementComponent::SlideReleased);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ADemo3Character::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADemo3Character::MoveRight);
	

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ADemo3Character::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ADemo3Character::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ADemo3Character::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ADemo3Character::TouchStopped);
}

/*void ADemo3Character::Jump()
{
	Super::Jump();

	bPressedDemo3Jump = true;
	bPressedJump = false;
}

void ADemo3Character::StopJumping()
{
	Super::StopJumping();

	bPressedDemo3Jump = false;
}*/

FCollisionQueryParams ADemo3Character::GetIgnoreCharacterParams() const
{
	FCollisionQueryParams Params;

	TArray<AActor*> CharacterChildren;
	GetAllChildActors(CharacterChildren);
	Params.AddIgnoredActors(CharacterChildren);
	Params.AddIgnoredActor(this);

	return Params;
}

#pragma endregion

// Jump 
#pragma region Jump

void ADemo3Character::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}
void ADemo3Character::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

#pragma endregion

// Look
#pragma region Look

void ADemo3Character::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}
void ADemo3Character::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

#pragma endregion

// Move
#pragma region Move

void ADemo3Character::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);

		
	}
}
void ADemo3Character::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

#pragma endregion

#pragma endregion 