// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RTPSGameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API URTPSGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	

public:
	void SetPlayerName(FString InPlayerName);
	FString GetPlayerName();

private:
	//UPROPERTY()
	class FString PlayerName;
};
