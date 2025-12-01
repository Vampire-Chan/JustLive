#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Game HUD Widget
 * Displays Health, Ammo, and Crosshair.
 */
class JUSTLIVE_API SGameHUDWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGameHUDWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UGameInstance>, GameInstance)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// Placeholder for future data binding
	FText GetHealthText() const;
	FText GetAmmoText() const;

	TWeakObjectPtr<UGameInstance> GameInstance;
	TSharedPtr<FSlateBrush> CrosshairBrush;
	TSharedPtr<FSlateBrush> HealthIconBrush;
	TSharedPtr<FSlateBrush> AmmoIconBrush;
};
