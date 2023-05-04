#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Demo3Character.generated.h"

class UDemo3MovementComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS(config=Game)

class ADemo3Character : public ACharacter
{
	GENERATED_BODY()

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
// Linked Components
#pragma region Components
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Movement) UDemo3MovementComponent* Demo3MovementComponent;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true")) USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true")) UCameraComponent* FollowCamera;

public:
	UPROPERTY(BlueprintReadOnly, Category=Character) bool bPressedDemo3Jump;

#pragma endregion
	
public:
	ADemo3Character(const FObjectInitializer& ObjectInitializer);

	 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera) float BaseTurnRate;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera) float BaseLookUpRate;
	
	// Camera Zoom
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=CameraZoom) float ZoomStep = 200.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=CameraZoom) float MaxZoomDistance = 1000.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=CameraZoom) float MinZoomDistance = 150.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=CameraZoom) float CameraZoomSpeed = 2.5f;
	float ScrollArmLength = 300.f;

protected:
	
	void MoveForward(float Value);
	void MoveRight(float Value);

	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);
	
	void CameraZoom(float value);

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
	//virtual void Jump() override;
	//virtual void StopJumping() override;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; } 
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; } 
	FORCEINLINE UDemo3MovementComponent* GetDemo3MovementComponent() const { return Demo3MovementComponent; } 
	FCollisionQueryParams GetIgnoreCharacterParams() const;

	UFUNCTION(BlueprintImplementableEvent, Category=CustomFunctions) void OnTryMantle();
	
};

