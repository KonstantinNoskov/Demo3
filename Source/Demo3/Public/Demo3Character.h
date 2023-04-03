#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Demo3Character.generated.h"

class UDemo3MovementComponent;

UCLASS(config=Game)



class ADemo3Character : public ACharacter
{
	GENERATED_BODY()

	virtual void BeginPlay() override;

// Linked Components
#pragma region Components
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Movement)
	UDemo3MovementComponent* Demo3MovementComponent;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

#pragma endregion
	
public:
	ADemo3Character(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

protected:
	
	void MoveForward(float Value);
	void MoveRight(float Value);
	
	void TurnAtRate(float Rate);
	
	void LookUpAtRate(float Rate);
	
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE class UDemo3MovementComponent* GetDemo3MovementComponent() const { return Demo3MovementComponent; }
	
};

