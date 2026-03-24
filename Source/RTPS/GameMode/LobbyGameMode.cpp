// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "RTPS/Character/RTPSPlayerController.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"

ALobbyGameMode::ALobbyGameMode()
{
	PlayerControllerClass = ARTPSPlayerController::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("Lobby GM: %s, PlayerControllerClass=%s"),
		*GetName(),
		*GetNameSafe(PlayerControllerClass));
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	//int32 RandomNumber = FMath::RandRange(1, 100000);

	//if (NewPlayer)
	//{
	//	APlayerState* PlayerState = NewPlayer->PlayerState;
	//	if (PlayerState)
	//	{
	//		PlayerState->SetPlayerName(FString::Printf(TEXT("Player-%d"), RandomNumber));
	//	}
	//}

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	
	if (NumberOfPlayers >= 3)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			bUseSeamlessTravel = true;
			World->ServerTravel(FString("/Game/Maps/DemoMapShantyTown?listen"));
		}
	}
}