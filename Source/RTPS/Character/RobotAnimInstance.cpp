// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotAnimInstance.h"
#include "RobotCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void URobotAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RobotCharacter = Cast<ARobotCharacter>(TryGetPawnOwner());
	// if (RobotCharacter)
	// {
	// 	RobotCharacter->GetCharacterMovement()->OnReachedJumpApex
	// }
}

void URobotAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (RobotCharacter == nullptr)
	{
		RobotCharacter = Cast<ARobotCharacter>(TryGetPawnOwner());
	}

	if (RobotCharacter == nullptr) return;

	FVector Velocity = RobotCharacter->GetVelocity();

	Velocity.Z = 0.0f;
	Speed = Velocity.Size();

	// bIsInAir = RobotCharacter->GetCharacterMovement()->IsFalling();
	// bool bIsJumping = RobotCharacter->bWasJumping;
	bIsFalling = RobotCharacter->GetCharacterMovement()->IsFalling();
	bIsPreJumping = RobotCharacter->GetIsPreJumping();

	bWeaponEquipped = RobotCharacter->IsWeaponEquipped();
	bIsCrouched = RobotCharacter->bIsCrouched;
	bAiming = RobotCharacter->IsAiming();
	
	// Offset Yaw for strafing
	FRotator AimRotation = RobotCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(RobotCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 5.f);
	YawOffset = DeltaRotation.Yaw;
	// YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = RobotCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -120.f, 120.f);
	
	AO_Yaw = RobotCharacter->GetAO_Yaw();
	AO_Pitch = RobotCharacter->GetAO_Pitch();
	
	bIsAccelerating = RobotCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bIsAboutToLand = RobotCharacter->GetIsAboutToLand();
}
