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

	bIsInAir = RobotCharacter->GetCharacterMovement()->IsFalling();
	bIsJumping = RobotCharacter->bWasJumping;
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
	// UE_LOG(LogTemp, Log, TEXT("RobotCharacter Lean: %f"), Lean);
	
	bIsAccelerating = RobotCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	if (bIsFalling)
	{
		bIsJumpApexReached = RobotCharacter->GetIsJumpApexReached();
		FHitResult HitResult;
		FVector Start = RobotCharacter->GetActorLocation();
		FVector End = Start + (FVector::DownVector * 1000.0f); // Trace 1000 units down
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(RobotCharacter); // Ignore the character itself

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
		{
			float HeightDifference = Start.Z - HitResult.ImpactPoint.Z;
			float HeightThreshold = bWeaponEquipped ? 480.f : 240.f;
			// Use HeightDifference here
			if (HeightDifference < HeightThreshold && !bIsJumping)
			{
				bIsAboutToLand = true;
				UE_LOG(LogTemp, Log, TEXT("RobotCharacter about to hit, height difference %f, bIsJumping %hs, GetIsJumpApexReached %hs"), 
					HeightDifference, bIsJumping ? "True" : "False", bIsJumpApexReached ? "True" : "False");
				if (bIsJumpApexReached)
				{
					RobotCharacter->SetIsJumpApexReached(false);
				}
			}
		}
	}
	if (!bIsFalling)
	{
		bIsAboutToLand = false;
		bIsJumping = false;
		// RobotCharacter->SetIsPreJumping(false);
	}
}
