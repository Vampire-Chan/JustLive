#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Main Menu Widget
 * Displays options to Play, Browse Mods, and Quit.
 */
class JUSTLIVE_API SMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UGameInstance>, GameInstance)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	FReply OnPlayClicked();
	FReply OnModsClicked();
	FReply OnQuitClicked();

	TSharedPtr<FSlateBrush> BgBrush;
	TWeakObjectPtr<UGameInstance> GameInstance;
};
