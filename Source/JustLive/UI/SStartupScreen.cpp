#include "SStartupScreen.h"
#include "UIManager.h"
#include "../Managers/AudioManager.h"
#include "JustLiveStyle.h"
#include "../Managers/TextureManager.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"

void SStartupScreen::Construct(const FArguments& InArgs)
{
	GameInstance = InArgs._GameInstance;

	// Try to load logo
	LogoBrush = nullptr;
	if (GameInstance.IsValid())
	{
		if (UTextureManager* TexMgr = GameInstance->GetSubsystem<UTextureManager>())
		{
			if (UTexture2D* LogoTex = TexMgr->LoadTexture(TEXT("@data/ui/logo_engine.png")))
			{
				LogoBrush = FJustLiveStyle::CreateDynamicBrush(LogoTex, FVector2D(512, 512));
			}
		}
	}

	// Play Startup Music
	if (GameInstance.IsValid())
	{
		if (UAudioManager* AudioMgr = GameInstance->GetSubsystem<UAudioManager>())
		{
			AudioMgr->PlayBackgroundMusic(TEXT("/Game/Assets/Frontend/Initial/alonept2_alanwalker.alonept2_alanwalker"));
		}
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
		.Padding(20)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 20)
			.HAlign(HAlign_Center)
			[
				LogoBrush.IsValid() ?
				static_cast<TSharedRef<SWidget>>(SNew(SImage).Image(LogoBrush.Get())) :
				static_cast<TSharedRef<SWidget>>(SNew(STextBlock)
					.Text(FText::FromString("JUSTLIVE ENGINE"))
					.TextStyle(FJustLiveStyle::Get(), "JustLive.HeaderText")
					.Justification(ETextJustify::Center))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString("Powered by Unreal Engine 5.6"))
				.TextStyle(FJustLiveStyle::Get(), "JustLive.NormalText")
				.Justification(ETextJustify::Center)
			]
		]
	];
}

void SStartupScreen::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (!bTransitioned)
	{
		TimeRemaining -= InDeltaTime;
		if (TimeRemaining <= 0.0f)
		{
			OnSkipClicked();
		}
	}
}

FReply SStartupScreen::OnSkipClicked()
{
	if (bTransitioned) return FReply::Handled();
	bTransitioned = true;

	if (GameInstance.IsValid())
	{
		if (UUIManager* UI = GameInstance->GetSubsystem<UUIManager>())
		{
			UI->SwitchToState(EUIState::MainMenu);
		}
	}
	
	return FReply::Handled();
}
