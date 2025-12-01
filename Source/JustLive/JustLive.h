// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FJustLiveModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
