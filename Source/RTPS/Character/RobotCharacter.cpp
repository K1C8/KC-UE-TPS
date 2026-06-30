// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "RTPS/Weapon/Weapon.h"
#include "RTPS/RobotComponent/CombatComponent.h"
#include "RTPS/HUD/OverheadWidget.h"

// Sets default values
ARobotCharacter::ARobotCharacter()
{

	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 3000.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	GetCharacterMovement()->bNotifyApex = true;
}


void ARobotCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);
	UpdateJumpStatus();
}

void ARobotCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ARobotCharacter::NotifyJumpApex()
{
	Super::NotifyJumpApex();
	bIsJumpApexReached = true;
}

void ARobotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ARobotCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ARobotCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ARobotCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ARobotCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ARobotCharacter::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ARobotCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ARobotCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ARobotCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ARobotCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("GunFire", IE_Pressed, this, &ARobotCharacter::GunFireButtonPresses);
	PlayerInputComponent->BindAction("GunFire", IE_Released, this, &ARobotCharacter::GunFireButtonReleased);
	PlayerInputComponent->BindAction("MeleeStrike", IE_Pressed, this, &ARobotCharacter::MeleeStrikeButtonPressed);
	PlayerInputComponent->BindAction("MeleeStrike", IE_Released, this, &ARobotCharacter::MeleeStrikeButtonReleased);

}


void ARobotCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

bool ARobotCharacter::GetIsJumpApexReached()
{
	return bIsJumpApexReached;
}

void ARobotCharacter::SetIsJumpApexReached(bool NewIsJumpApexReached)
{
	bIsJumpApexReached = NewIsJumpApexReached;
}

bool ARobotCharacter::GetIsPreJumping()
{
	return bIsPreJumping;
}

bool ARobotCharacter::GetIsAboutToLand()
{
	return bIsAboutToLand;
}

void ARobotCharacter::SetIsAboutToLand(const bool InIsAboutToLand)
{
	if (bIsAboutToLand == InIsAboutToLand)
	{
		return;
	}
	if (HasAuthority())
	{
		bIsAboutToLand = InIsAboutToLand;
	}
	else if (IsLocallyControlled())
	{
		bIsAboutToLand = InIsAboutToLand;
		ServerSetIsAboutToLand(InIsAboutToLand);
	}
}

void ARobotCharacter::OnRep_PlayerState()
{
	UpdateOverheadWidget();
}

void ARobotCharacter::Jump()
{
	// Prevent re-triggering if already in a pre-jump or currently being pulled by gravity
	if (bIsPreJumping || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	// 1. Enter pre-jump state (AnimBP will read this to play the wind-up animation)
	SetIsPreJumping(true);

	// 2. Set a timer to execute the physical jump after the delay window
	GetWorldTimerManager().SetTimer(
		JumpTimerHandle, 
		this, 
		&ARobotCharacter::CommitJump, 
		PreJumpDelay, 
		false
	);
}

void ARobotCharacter::PlayMeleeStrikeMontage(const int32 InStage) const
{
	if (Combat == nullptr || Combat->bAiming || bIsCrouched || InStage > 2 || InStage < 0)
	{
		return;
	}
	UAnimInstance* RobotAnimInstance = GetMesh()->GetAnimInstance();
	if (RobotAnimInstance && MeleeStrikeMontage)
	{
		RobotAnimInstance->Montage_Play(MeleeStrikeMontage);
		UE_LOG(LogTemp, Log, TEXT("[ARobotCharacter] PlayMeleeStrikeMontage() ready to play stage %d."), InStage);
		FName SectionName;
		switch (InStage)
		{
		case 0:
			{
				SectionName = FName("CrossPunch");
				break;
			}
		case 1:
			{
				SectionName = FName("HookPunch");
				break;
			}
		case 2:
			{
				SectionName = FName("StandingKick");
				break;
			}
		default:
			{
				break;
			}
		}
		RobotAnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ARobotCharacter::UpdateOverheadWidget()
{
	APlayerState* PlayerStateToUpdate = GetPlayerState<APlayerState>();

	if (PlayerStateToUpdate)
	{
		FString PlayerName = PlayerStateToUpdate->GetPlayerName();
		if (!PlayerName.IsEmpty())
		{
			UUserWidget* OverheadUserWidget = OverheadWidget->GetUserWidgetObject();
			if (OverheadUserWidget)
			{
				UOverheadWidget* OverheadUserWidgetCasted = Cast<UOverheadWidget>(OverheadUserWidget);
				if (OverheadUserWidgetCasted)
				{
					OverheadUserWidgetCasted->SetDisplayText(PlayerName);
				}
				else
				{
					UE_LOG(LogActor, Display, TEXT("Failed to cast UOverheadWidget* OverheadWidgetCasted from OverheadWidget of RobotCharacter."));
				}
			}
		}
	}
}

void ARobotCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ARobotCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ARobotCharacter, bIsPreJumping);
	DOREPLIFETIME(ARobotCharacter, bIsAboutToLand);
}


void ARobotCharacter::MoveForward(float Value) 
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ARobotCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ARobotCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ARobotCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ARobotCharacter::EquipButtonPressed()
{
	// UE_LOG(LogTemp, Log, TEXT("ARobotCharacter EquipButtonPressed() ready to start."));
	if (Combat)
	{
		if (HasAuthority())
		{
			UE_LOG(LogTemp, Log, TEXT("ARobotCharacter EquipButtonPressed() with authority."));
			Combat->EquipWeapon(OverlappingWeapon);			
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("ARobotCharacter EquipButtonPressed() without authority."));
			ServerEquipButtonPressed();
		}
	}
}

void ARobotCharacter::CommitJump()
{
	SetIsPreJumping(false);
	Super::Jump();
	GetCharacterMovement()->bNotifyApex = true;
}

void ARobotCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else if (IsWeaponEquipped())
	{
		Crouch();
	}
}

void ARobotCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ARobotCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ARobotCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr)
	{
		return;
	}
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsFalling = GetCharacterMovement()->IsFalling();
	
	if (Speed == 0.f && !bIsFalling)	// Standing still and not jumping
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		bUseControllerRotationYaw = false;
		AO_Yaw = DeltaAimRotation.Yaw;
	}
	
	if (Speed > 0.f || bIsFalling)	// Moving or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
	}
	
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (IsWeaponEquipped() && !IsLocallyControlled())
	{
		UE_LOG(LogTemp, Log, TEXT("RobotCharacter not controlled before remap Yaw %f, Pitch %f."), AO_Yaw, AO_Pitch);
	}
	
	FVector2D InRange(270.f, 360.f);
	FVector2D OutRange(-89.99f, 0.f);
	// Map yaw/pitch because yaw/pitch is compressed into uint16 during network sync.
	if (AO_Yaw >= 270.f && !IsLocallyControlled())
	{
		AO_Yaw = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Yaw);
	}
	else if (AO_Yaw > 90.f)
	{
		AO_Yaw = 90.f;
	}
	if (AO_Pitch >= 270.f && !IsLocallyControlled())
	{
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
	else if (AO_Yaw > 90.f)
	{
		AO_Yaw = 90.f;
	}
	if (IsWeaponEquipped() && !IsLocallyControlled())
	{
		UE_LOG(LogTemp, Log, TEXT("RobotCharacter not controlled after remap Yaw %f, Pitch %f."), AO_Yaw, AO_Pitch);
	}
}

void ARobotCharacter::GunFireButtonPresses()
{
	if (Combat)
	{
		Combat->GunFireButtonPressed(true);
	}
}

void ARobotCharacter::GunFireButtonReleased()
{
	if (Combat)
	{
		Combat->GunFireButtonPressed(false);
	}
}

void ARobotCharacter::MeleeStrikeButtonPressed()
{
	if (Combat)
	{
		Combat->MeleeStrikeButtonPressed(true);
	}
}

void ARobotCharacter::MeleeStrikeButtonReleased()
{
	if (Combat)
	{
		Combat->MeleeStrikeButtonPressed(false);
	}
}

void ARobotCharacter::UpdateJumpStatus()
{
	if (!HasAuthority() && !IsLocallyControlled())
	{
		return;
	}
	const bool bIsFalling = GetCharacterMovement()->IsFalling();
	if (bIsFalling)
	{
		bIsJumpApexReached = GetIsJumpApexReached();
		FHitResult HitResult;
		FVector Start = GetActorLocation();
		FVector End = Start + (FVector::DownVector * 4000.0f); // Trace 1000 units down
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this); // Ignore the character itself

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
		{
			const float HeightDifference = Start.Z - HitResult.ImpactPoint.Z;
			const float HeightThreshold = IsWeaponEquipped() ? 1700.f : 1240.f;
			// Use HeightDifference here
			if (HeightDifference < HeightThreshold && !bWasJumping)
			{
				SetIsAboutToLand(true);
				if (bIsJumpApexReached)
				{
					SetIsJumpApexReached(false);
				}
			}
		}
	}
	if (!bIsFalling)
	{
		SetIsAboutToLand(false);
	}
}

void ARobotCharacter::SetIsPreJumping(bool InIsPreJumping)
{
	bIsPreJumping = InIsPreJumping;
	ServerSetIsPreJumping(InIsPreJumping);
}

void ARobotCharacter::ServerSetIsAboutToLand_Implementation(bool InIsAboutToLand)
{
	if (HasAuthority())
	{
		bIsAboutToLand = InIsAboutToLand;
	}
}

void ARobotCharacter::ServerSetIsPreJumping_Implementation(bool InIsPreJumping)
{
	bIsPreJumping = InIsPreJumping;
}

void ARobotCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ARobotCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// Handle server side of overlapping.
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		// Server only case.
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}


void ARobotCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		// UE_LOG(LogTemp, Log, TEXT("Showing pickup widget"));
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}

}

bool ARobotCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ARobotCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

float ARobotCharacter::GetAO_Yaw() const
{
	return AO_Yaw;
}

float ARobotCharacter::GetAO_Pitch() const
{
	return AO_Pitch;
}


