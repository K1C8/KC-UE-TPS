// Fill out your copyright notice in the Description page of Project Settings.


#include "RTPSGameInstanceSubsystem.h"
#include "GameFramework/PlayerState.h"

void URTPSGameInstanceSubsystem::SetPlayerName(FString InPlayerName)
{
	PlayerName = InPlayerName;

	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	APlayerState* PlayerState = NULL;
	if (PlayerController)
	{
		PlayerState = PlayerController->GetPlayerState<APlayerState>();
	}
	if (PlayerState)
	{
		PlayerState->SetPlayerName(InPlayerName);
	} 
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, "Cannot set player name to a null Player State");
		}
	}
}

FString URTPSGameInstanceSubsystem::GetPlayerName()
{
	return PlayerName;
}
