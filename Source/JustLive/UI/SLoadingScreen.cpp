#include "SLoadingScreen.h"
#include "UIManager.h"
#include "JustLiveStyle.h"
#include "JustLiveStyle.h"
#include "../Managers/TextureManager.h"
#include "../Managers/AudioManager.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Layout/SBorder.h"

void SLoadingScreen::Construct(const FArguments& InArgs)
{
	GameInstance = InArgs._GameInstance;


	// Play Loading Music
	if (GameInstance.IsValid())
	{
		if (UAudioManager* AudioMgr = GameInstance->GetSubsystem<UAudioManager>())
		{
			AudioMgr->PlayBackgroundMusic(TEXT("/Game/Assets/Frontend/Loading/Drum_Alanwalker.Drum_Alanwalker"));
		}
	}

	// Load Carousel Images
	if (GameInstance.IsValid())
	{
		if (UTextureManager* TexMgr = GameInstance->GetSubsystem<UTextureManager>())
		{
			// Try loading loading_1.png, loading_2.png, etc.
			for (int32 i = 1; i <= 3; i++)
			{
				FString Path = FString::Printf(TEXT("@data/ui/loading/loading_%d.png"), i);
				if (UTexture2D* Tex = TexMgr->LoadTexture(Path))
				{
					BackgroundImages.Add(Tex);
				}
			}
		}
	}

	// Set Initial Background
	if (BackgroundImages.Num() > 0)
	{
		BrushLayerA = FJustLiveStyle::CreateDynamicBrush(BackgroundImages[0], FVector2D(1920, 1080));
	}
	else
	{
		BrushLayerA = MakeShareable(new FSlateColorBrush(FLinearColor::Black));
	}
	
	// Initialize B as well (placeholder)
	BrushLayerB = BrushLayerA;

	ChildSlot
	[
		SNew(SOverlay)
		
		// Layer A (Initially Visible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(ImageLayerA, SImage)
			.Image(BrushLayerA.Get())
			.ColorAndOpacity(FLinearColor(1, 1, 1, 1))
		]

		// Layer B (Initially Invisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(ImageLayerB, SImage)
			.Image(BrushLayerB.Get()) // Placeholder
			.ColorAndOpacity(FLinearColor(1, 1, 1, 0))
		]

		// Content Overlay
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.VAlign(VAlign_Bottom)
			.Padding(50)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 10)
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(FText::FromString("Loading World..."))
					.TextStyle(FJustLiveStyle::Get(), "JustLive.NormalText")
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ProgressBar, SProgressBar)
					.Percent(0.0f)
				]
			]
		]
	];
}

void SLoadingScreen::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Carousel Logic
	// Only update carousel if we have time remaining (prevents snapping at the end)
	if (BackgroundImages.Num() > 1)
	{
		if (!bIsFading)
		{
			CarouselTimer += InDeltaTime;
			if (CarouselTimer >= 3.0f) // Switch every 3 seconds
			{
				CarouselTimer = 0.0f;
				bIsFading = true;
				CrossFadeAlpha = 0.0f;

				// Prepare Next Image
				int32 NextIndex = (CurrentImageIndex + 1) % BackgroundImages.Num();
				
				// Create the new brush
				TSharedPtr<FSlateBrush> NewBrush = FJustLiveStyle::CreateDynamicBrush(BackgroundImages[NextIndex], FVector2D(1920, 1080));

				// Set Next Image to the INACTIVE layer and update the persistent brush member
				if (ActiveLayerIndex == 0) // A is active, prepare B
				{
					BrushLayerB = NewBrush; // Keep it alive
					ImageLayerB->SetImage(BrushLayerB.Get());
				}
				else // B is active, prepare A
				{
					BrushLayerA = NewBrush; // Keep it alive
					ImageLayerA->SetImage(BrushLayerA.Get());
				}
				
				CurrentImageIndex = NextIndex;
			}
		}
		else
		{
			// Fading Logic
			CrossFadeAlpha += InDeltaTime / CrossFadeDuration;
			if (CrossFadeAlpha >= 1.0f)
			{
				CrossFadeAlpha = 1.0f;
				bIsFading = false;
				ActiveLayerIndex = (ActiveLayerIndex == 0) ? 1 : 0; // Swap Active Layer
			}

			// Update Opacities
			// If A is active (0), it fades OUT (1 -> 0), B fades IN (0 -> 1)
			// If B is active (1), it fades OUT (1 -> 0), A fades IN (0 -> 1)
			
			float FadeOut = 1.0f - CrossFadeAlpha;
			float FadeIn = CrossFadeAlpha;

			if (ActiveLayerIndex == 0) // Transitioning A -> B
			{
				ImageLayerA->SetColorAndOpacity(FLinearColor(1, 1, 1, FadeOut));
				ImageLayerB->SetColorAndOpacity(FLinearColor(1, 1, 1, FadeIn));
			}
			else // Transitioning B -> A
			{
				ImageLayerB->SetColorAndOpacity(FLinearColor(1, 1, 1, FadeOut));
				ImageLayerA->SetColorAndOpacity(FLinearColor(1, 1, 1, FadeIn));
			}
		}
	}
}

void SLoadingScreen::SetStatusText(const FString& Text)
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(FText::FromString(Text));
	}
}

void SLoadingScreen::SetProgress(float Percent)
{
	if (ProgressBar.IsValid())
	{
		ProgressBar->SetPercent(Percent);
	}
}
