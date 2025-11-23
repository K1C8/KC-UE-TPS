// Fill out your copyright notice in the Description page of Project Settings.


#include "OnScreenLog.h"
#include "Engine/Engine.h"
#include <string>

//OnScreenLog::OnScreenLog()
//{
//}
//
//OnScreenLog::~OnScreenLog()
//{
//}

void OnScreenLog::OnScreenLogDisplay(const FString& InText)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, InText);
	}
}
