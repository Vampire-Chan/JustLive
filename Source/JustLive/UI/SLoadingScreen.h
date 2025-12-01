#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Images/SImage.h"

/**
 * Loading Screen Widget
 * Displays progress and status text.
 */
class JUSTLIVE_API SLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLoadingScreen) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UGameInstance>, GameInstance)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void SetStatusText(const FString& Text);
	void SetProgress(float Percent);

private:
	TSharedPtr<class STextBlock> StatusTextBlock;
	TSharedPtr<class SProgressBar> ProgressBar;
	TWeakObjectPtr<UGameInstance> GameInstance;
	
	// Carousel
	TSharedPtr<class SImage> ImageLayerA;
	TSharedPtr<class SImage> ImageLayerB;
	
	// Brushes to keep textures alive
	TSharedPtr<FSlateBrush> BrushLayerA;
	TSharedPtr<FSlateBrush> BrushLayerB;

	TArray<UTexture2D*> BackgroundImages;
	
	float CarouselTimer = 0.0f;
	int32 CurrentImageIndex = 0;
	
	// Cross-Fade Logic
	bool bIsFading = false;
	float CrossFadeAlpha = 0.0f;
	float CrossFadeDuration = 1.0f;
	int32 ActiveLayerIndex = 0; // 0 = A is visible, 1 = B is visible
};
