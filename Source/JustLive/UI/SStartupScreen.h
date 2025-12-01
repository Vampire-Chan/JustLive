
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Startup Screen Widget
 * Displays logos and transitions to the Main Menu.
 */
class JUSTLIVE_API SStartupScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStartupScreen) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UGameInstance>, GameInstance)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	// Callback to skip animation or finish
	FReply OnSkipClicked();

	float TimeRemaining = 5.0f;
	bool bTransitioned = false;
	TWeakObjectPtr<UGameInstance> GameInstance;
	TSharedPtr<FSlateBrush> LogoBrush;
};
