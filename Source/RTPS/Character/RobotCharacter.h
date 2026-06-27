// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RobotCharacter.generated.h"

UCLASS()
class RTPS_API ARobotCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARobotCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	virtual void NotifyJumpApex() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void OnRep_PlayerState() override;

	virtual void UpdateOverheadWidget();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PostInitializeComponents() override;
	
	bool GetIsJumpApexReached();
	void SetIsJumpApexReached(bool NewIsJumpApexReached);
	
	bool GetIsPreJumping();
	void SetIsPreJumping(bool InIsPreJumping);

	virtual void Jump() override;
	
protected:

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CommitJump();
	void CrouchButtonPressed();
	
	// Timer Handle to track the pre-jump delay
	FTimerHandle JumpTimerHandle;
	FTimerHandle LandTimerHandle;

	// Time in seconds to wait for the pre-jump animation to play out
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
	float PreJumpDelay = 0.35f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
	float LandingDelay = 0.35f;

	// Exposed to AnimBP to trigger the pre-jump/wind-up state
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump")
	bool bIsPreJumping = false;

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;
	
	bool bIsJumpApexReached = false;
	bool bIsLanded = false;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;
	
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

public:	
	void SetOverlappingWeapon(AWeapon* Weapon);
	
	bool IsWeaponEquipped();
};
