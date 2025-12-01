// Copyright Epic Games, Inc. All Rights Reserved.

#include "JustLive.h"
#include "UI/JustLiveStyle.h"
#include "Modules/ModuleManager.h"

void FJustLiveModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();
	FJustLiveStyle::Initialize();
	FJustLiveStyle::ReloadTextures();
}

void FJustLiveModule::ShutdownModule()
{
	FJustLiveStyle::Shutdown();
	FDefaultGameModuleImpl::ShutdownModule();
}

IMPLEMENT_PRIMARY_GAME_MODULE( FJustLiveModule, JustLive, "JustLive" );
