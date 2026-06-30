// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "RTPS/Character/RobotCharacter.h"
#include "RTPS/Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::SetMeleeStrikeStage(int8 InStage)
{
	UE_LOG(LogTemp, Log, TEXT("[UCombatComponent] SetMeleeStrikeStage() received request, InStage: %hhd."), InStage);
	if (MeleeStrikeStage == InStage)
	{
		return;
	}
	if (Character && Character->HasAuthority())
	{
		MeleeStrikeStage = InStage;
	}
	else if (Character && Character->IsLocallyControlled())
	{
		MeleeStrikeStage = InStage;
		ServerSetMeleeStrikeStage(InStage);
	}
	else if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UCombatComponent] SetMeleeStrikeStage() got a nullptr of Character"));
	}
}

void UCombatComponent::ServerSetMeleeStrikeStage_Implementation(int8 InStage)
{
	if (Character->HasAuthority())
	{
		MeleeStrikeStage = InStage;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::GunFireButtonPressed(bool bPressed)
{
	bGunFireButtonPressed = bPressed;
}

void UCombatComponent::MeleeStrikeButtonPressed(bool bPressed)
{
	UE_LOG(LogTemp, Log, TEXT("[UCombatComponent] MeleeStrikeButtonPressed() received request, bPressed: %hhd."), bPressed);
	bIsMeleeStriking = bPressed;
	if (!bPressed)
	{
		return;
	}
	FDateTime CurrentTime = FDateTime::Now();
	FTimespan TimeFromLastMeleeStrike = CurrentTime - LastMeleeStrikeTime;
	if (TimeFromLastMeleeStrike > FTimespan(0, 0, 4.5))
	{
		SetMeleeStrikeStage(0);
	}
	else
	{
		const int8 NewStage = (MeleeStrikeStage + 1) % 3;
		SetMeleeStrikeStage(NewStage);
	}
	if (Character)
	{
		Character->PlayMeleeStrikeMontage(MeleeStrikeStage);
	}
	LastMeleeStrikeTime = CurrentTime;
}



void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, MeleeStrikeStage);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	
	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

