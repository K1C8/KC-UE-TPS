// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
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
	CameraBoom->TargetArmLength = 500.f;
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

}

void ARobotCharacter::BeginPlay()
{
	Super::BeginPlay();
	bIsLanded = true;
	
}

void ARobotCharacter::NotifyJumpApex()
{
	Super::NotifyJumpApex();
	bIsJumpApexReached = true;
}

void ARobotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

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

void ARobotCharacter::SetIsPreJumping(bool InIsPreJumping)
{
	bIsPreJumping = InIsPreJumping;
}

void ARobotCharacter::OnRep_PlayerState()
{
	UpdateOverheadWidget();
}

void ARobotCharacter::Jump()
{
	// Prevent re-triggering if already in a pre-jump or currently mid-air
	if (bIsPreJumping || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	// 1. Enter pre-jump state (AnimBP will read this to play the wind-up animation)
	bIsPreJumping = true;
	bIsLanded = false;

	// 2. Set a timer to execute the physical jump after the delay window
	GetWorldTimerManager().SetTimer(
		JumpTimerHandle, 
		this, 
		&ARobotCharacter::CommitJump, 
		PreJumpDelay, 
		false
	);
}

void ARobotCharacter::PlayMeleeStrikeMontage(const int32 InSection)
{
	if (Combat == nullptr || Combat->bAiming || InSection > 2 || InSection < 0)
	{
		return;
	}
	UAnimInstance* RobotAnimInstance = GetMesh()->GetAnimInstance();
	if (RobotAnimInstance && MeleeStrikeMontage)
	{
		RobotAnimInstance->Montage_Play(MeleeStrikeMontage);
		FName SectionName;
		switch (InSection)
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
	
	UE_LOG(LogTemp, Log, TEXT("ARobotCharacter EquipButtonPressed() ready to start."));
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
	bIsPreJumping = false;
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




