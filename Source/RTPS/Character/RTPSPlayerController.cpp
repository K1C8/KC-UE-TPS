// Fill out your copyright notice in the Description page of Project Settings.


#include "RTPSPlayerController.h"
#include "RobotCharacter.h"
#include "GameFramework/PlayerState.h"
#include "MultiplayerSessions/Public/RTPSGameInstanceSubsystem.h"

void ARTPSPlayerController::Server_SetPlayerName_Implementation(const FString& InName)
{
	UE_LOG(LogPlayerController, Display, TEXT("Server received data from client: %s"), *InName);

	APlayerState* PlayerStateToProcess = GetPlayerState<APlayerState>();
	if (PlayerStateToProcess)
	{
		PlayerStateToProcess->SetPlayerName(InName);
		APawn* PawnToUpdate = PlayerState->GetPawn();
		if (PawnToUpdate)
		{
			ARobotCharacter* CharacterToUpdate = Cast<ARobotCharacter>(PawnToUpdate);
			if (CharacterToUpdate)
			{
				UE_LOG(LogPlayerController, Display, TEXT("Server is going to update the OverheadWidget of the ARobotCharacter of the client on the server side."));
				CharacterToUpdate->UpdateOverheadWidget();
			}
		}
	}
}

bool ARTPSPlayerController::Server_SetPlayerName_Validate(const FString& InName)
{
	return true;
}

void ARTPSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		URTPSGameInstanceSubsystem* RTPSGameInstanceSubsystem = GameInstance->GetSubsystem<URTPSGameInstanceSubsystem>();
		if (RTPSGameInstanceSubsystem)
		{
			const FString LocalName = RTPSGameInstanceSubsystem->GetPlayerName();
			if (!LocalName.IsEmpty())
			{
				Server_SetPlayerName(LocalName);
			}
		}
	}
}
