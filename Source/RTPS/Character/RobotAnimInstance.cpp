// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotAnimInstance.h"
#include "RobotCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void URobotAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RobotCharacter = Cast<ARobotCharacter>(TryGetPawnOwner());
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

	bIsAccelerating = RobotCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
}
