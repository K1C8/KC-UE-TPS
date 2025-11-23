// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <string>

/**
 * 
 */
static class RTPS_API OnScreenLog
{
public:
	/*OnScreenLog();
	~OnScreenLog();*/
	static void OnScreenLogDisplay(const FString&);
	//static void OnScreenLogWarning(FText);
};
