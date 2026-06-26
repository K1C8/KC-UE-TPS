// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotAnimInstance.h"
#include "RobotCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	bWeaponEquipped = RobotCharacter->IsWeaponEquipped();
	
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
			// Use HeightDifference here
			if (HeightDifference < 180.0f && !bIsJumping)
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
	}
}
