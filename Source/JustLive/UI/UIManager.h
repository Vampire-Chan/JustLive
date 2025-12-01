#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UIManager.generated.h"

UENUM(BlueprintType)
enum class EUIState : uint8
{
	Startup,
	MainMenu,
	Loading,
	GameHUD,
	None
};

/**
 * UI Manager
 * Manages the high-level UI state and widget viewport injection.
 */
UCLASS()
class JUSTLIVE_API UUIManager : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	static UUIManager* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

	/**
	 * Switches the UI to the specified state with a fade transition.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SwitchToState(EUIState NewState);

	UFUNCTION(BlueprintCallable, Category = "UI")
	EUIState GetCurrentState() const { return CurrentState; }

	/**
	 * Manually triggers a loading screen with custom text/duration.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void TriggerLoadingScreen(const FString& LoadingText, float Duration);

	/**
	 * Updates the progress bar on the active loading screen.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateLoadingProgress(float Percent, const FString& StatusText);

private:
	EUIState CurrentState = EUIState::None;
	EUIState PendingState = EUIState::None;
	TSharedPtr<class SCompoundWidget> CurrentWidget;
	TWeakPtr<class SLoadingScreen> LoadingScreenWidget; // Weak ptr to specific widget type
	TSharedPtr<class SImage> FadeWidget;

	void ShowStartupScreen();
	void ShowMainMenu();
	void ShowLoadingScreen();
	void ShowGameHUD();
	void ClearCurrentWidget();

	// Fade Logic
	enum class EFadeState { None, FadingOut, FadingIn };
	EFadeState FadeState = EFadeState::None;
	float FadeAlpha = 0.0f;
	float FadeDuration = 0.5f;
	
	void UpdateFade(float DeltaTime);
	void CreateFadeWidget();

	// Fake Loading Logic
	bool bIsFakeLoading = false;
	float LoadingTimer = 0.0f;
};
