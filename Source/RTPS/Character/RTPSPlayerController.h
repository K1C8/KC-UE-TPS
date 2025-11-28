// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTPSPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class RTPS_API ARTPSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetPlayerName(const FString& InName);
	virtual void BeginPlay() override;
};
