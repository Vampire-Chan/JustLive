#include "UIManager.h"
#include "SStartupScreen.h"
#include "SMainMenu.h"
#include "SLoadingScreen.h"
#include "SGameHUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/Engine.h"
#include "Engine/Engine.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Images/SImage.h"
#include "../Managers/AudioManager.h"

UUIManager* UUIManager::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UUIManager>();
	}
	return nullptr;
}

void UUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreateFadeWidget();
	UE_LOG(LogTemp, Log, TEXT("UIManager Initialized."));
}

void UUIManager::Deinitialize()
{
	if (FadeWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(FadeWidget.ToSharedRef());
	}
	Super::Deinitialize();
}

void UUIManager::Tick(float DeltaTime)
{
	UpdateFade(DeltaTime);

	// Fake Loading Logic
	if (bIsFakeLoading && CurrentState == EUIState::Loading)
	{
		LoadingTimer += DeltaTime;
		float Percent = FMath::Clamp(LoadingTimer / 1.0f, 0.0f, 1.0f);
		
		UpdateLoadingProgress(Percent, TEXT("Loading World..."));

		if (LoadingTimer >= 1.0f)
		{
			bIsFakeLoading = false;
			SwitchToState(EUIState::GameHUD);
		}
	}
}

void UUIManager::SwitchToState(EUIState NewState)
{
	if (CurrentState == NewState || FadeState != EFadeState::None) return;

	PendingState = NewState;
	FadeState = EFadeState::FadingOut; // Start fading to black
}

void UUIManager::TriggerLoadingScreen(const FString& LoadingText, float Duration)
{
	// Start fake loading
	bIsFakeLoading = true;
	LoadingTimer = 0.0f;
	SwitchToState(EUIState::Loading);
}

void UUIManager::UpdateLoadingProgress(float Percent, const FString& StatusText)
{
	if (CurrentState == EUIState::Loading && LoadingScreenWidget.IsValid())
	{
		LoadingScreenWidget.Pin()->SetProgress(Percent);
		LoadingScreenWidget.Pin()->SetStatusText(StatusText);
	}
}

void UUIManager::UpdateFade(float DeltaTime)
{
	if (FadeState == EFadeState::None) return;

	if (FadeState == EFadeState::FadingOut)
	{
		FadeAlpha += DeltaTime / FadeDuration;
		if (FadeAlpha >= 1.0f)
		{
			FadeAlpha = 1.0f;
			
			// Screen is black, switch widget now
			ClearCurrentWidget();
			CurrentState = PendingState;

			switch (CurrentState)
			{
			case EUIState::Startup: ShowStartupScreen(); break;
			case EUIState::MainMenu: ShowMainMenu(); break;
			case EUIState::Loading: ShowLoadingScreen(); break;
			case EUIState::GameHUD: ShowGameHUD(); break;
			default: break;
			}

			FadeState = EFadeState::FadingIn; // Start fading back in
		}
	}
	else if (FadeState == EFadeState::FadingIn)
	{
		FadeAlpha -= DeltaTime / FadeDuration;
		if (FadeAlpha <= 0.0f)
		{
			FadeAlpha = 0.0f;
			FadeState = EFadeState::None; // Done
		}
	}

	// Update Fade Widget Opacity
	if (FadeWidget.IsValid())
	{
		FadeWidget->SetColorAndOpacity(FLinearColor(0, 0, 0, FadeAlpha));
		
		// Ensure Fade Widget is always on top
		if (FadeAlpha > 0.0f)
		{
			if (!FadeWidget->GetParentWidget().IsValid() && GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->AddViewportWidgetContent(FadeWidget.ToSharedRef(), 1000); // Z-Order 1000
			}
			FadeWidget->SetVisibility(EVisibility::Visible); // Block Input
		}
		else
		{
			if (FadeWidget->GetParentWidget().IsValid() && GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->RemoveViewportWidgetContent(FadeWidget.ToSharedRef());
			}
			FadeWidget->SetVisibility(EVisibility::HitTestInvisible); // Let input pass
		}
	}
}

void UUIManager::CreateFadeWidget()
{
	FadeWidget = SNew(SImage)
		.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.ColorAndOpacity(FLinearColor::Black);
}

void UUIManager::ClearCurrentWidget()
{
	if (CurrentWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(CurrentWidget.ToSharedRef());
	}
	CurrentWidget.Reset();
}

void UUIManager::ShowStartupScreen()
{
	if (GEngine && GEngine->GameViewport)
	{
		CurrentWidget = SNew(SStartupScreen)
			.GameInstance(GetGameInstance());
		GEngine->GameViewport->AddViewportWidgetContent(CurrentWidget.ToSharedRef());
		
		// Set Input Mode to UI Only
		if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(CurrentWidget);
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
}

void UUIManager::ShowMainMenu()
{
	if (GEngine && GEngine->GameViewport)
	{
		CurrentWidget = SNew(SMainMenu)
			.GameInstance(GetGameInstance());
		GEngine->GameViewport->AddViewportWidgetContent(CurrentWidget.ToSharedRef());
		
		if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(CurrentWidget);
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
}

void UUIManager::ShowLoadingScreen()
{
	if (GEngine && GEngine->GameViewport)
	{
		TSharedPtr<SLoadingScreen> NewWidget = SNew(SLoadingScreen)
			.GameInstance(GetGameInstance());
		
		CurrentWidget = NewWidget;
		LoadingScreenWidget = NewWidget;

		GEngine->GameViewport->AddViewportWidgetContent(CurrentWidget.ToSharedRef());
		
		// Loading screen might not need input
	}
}

void UUIManager::ShowGameHUD()
{
	if (GEngine && GEngine->GameViewport)
	{
		CurrentWidget = SNew(SGameHUDWidget)
			.GameInstance(GetGameInstance());
		GEngine->GameViewport->AddViewportWidgetContent(CurrentWidget.ToSharedRef());
		
		if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;

			// Spawn In-Game 3D Music Test
			if (UAudioManager* AudioMgr = GetGameInstance()->GetSubsystem<UAudioManager>())
			{
				AudioMgr->StopBackgroundMusic(); // Stop loading music
				
				if (APawn* Pawn = PC->GetPawn())
				{
					AudioMgr->SpawnProximityMusic(
						TEXT("/Game/Assets/Frontend/Gameplay/alonept2_alanwalker.alonept2_alanwalker"), 
						Pawn->GetActorLocation()
					);
				}
			}
		}
	}
}
