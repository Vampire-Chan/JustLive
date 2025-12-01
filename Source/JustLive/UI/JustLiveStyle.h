#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**
 * JustLive Style System
 * Central repository for all UI styles (Fonts, Colors, Brushes).
 */
class FJustLiveStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static void ReloadTextures();

	static const ISlateStyle& Get();
	static FName GetStyleSetName();

	/**
	 * Creates a Slate Brush from a UTexture2D.
	 * Useful for dynamic UI elements loaded via TextureManager.
	 */
	static TSharedPtr<FSlateImageBrush> CreateDynamicBrush(class UTexture2D* Texture, const FVector2D& Size);

private:
	static TSharedRef<class FSlateStyleSet> Create();
	static TSharedPtr<class FSlateStyleSet> StyleInstance;
};
