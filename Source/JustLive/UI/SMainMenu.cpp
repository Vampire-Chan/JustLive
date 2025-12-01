#include "SMainMenu.h"
#include "JustLiveStyle.h"
#include "../Managers/TextureManager.h"
#include "../Managers/AudioManager.h"
#include "UIManager.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Core/MapLoader.h"

void SMainMenu::Construct(const FArguments& InArgs)
{
	GameInstance = InArgs._GameInstance;

	// Try to load background
	BgBrush = nullptr;
	if (GameInstance.IsValid())
	{
		if (UTextureManager* TexMgr = GameInstance->GetSubsystem<UTextureManager>())
		{
			// Try PNG first, then JPG
			UTexture2D* BgTex = TexMgr->LoadTexture(TEXT("@data/ui/menu_bg.png"));
			if (!BgTex)
			{
				BgTex = TexMgr->LoadTexture(TEXT("@data/ui/menu_bg.jpg"));
			}

			if (BgTex)
			{
				BgBrush = FJustLiveStyle::CreateDynamicBrush(BgTex, FVector2D(1920, 1080));
			}
		}
	}

	// Play Menu Music
	if (GameInstance.IsValid())
	{
		if (UAudioManager* AudioMgr = GameInstance->GetSubsystem<UAudioManager>())
		{
			AudioMgr->PlayBackgroundMusic(TEXT("/Game/Assets/Frontend/Menu/Drum_Alanwalker.Drum_Alanwalker"));
		}
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(BgBrush.IsValid() ? BgBrush.Get() : FCoreStyle::Get().GetBrush("BlackBrush"))
		.Padding(50)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 40)
			[
				SNew(STextBlock)
				.Text(FText::FromString("JUSTLIVE"))
				.TextStyle(FJustLiveStyle::Get(), "JustLive.HeaderText")
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 10)
			[
				SNew(SButton)
				.OnClicked(this, &SMainMenu::OnPlayClicked)
				[
					SNew(STextBlock)
					.Text(FText::FromString("PLAY SANDBOX"))
					.TextStyle(FJustLiveStyle::Get(), "JustLive.NormalText")
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 10)
			[
				SNew(SButton)
				.OnClicked(this, &SMainMenu::OnModsClicked)
				[
					SNew(STextBlock)
					.Text(FText::FromString("MODS"))
					.TextStyle(FJustLiveStyle::Get(), "JustLive.NormalText")
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 10)
			[
				SNew(SButton)
				.OnClicked(this, &SMainMenu::OnQuitClicked)
				[
					SNew(STextBlock)
					.Text(FText::FromString("QUIT"))
					.TextStyle(FJustLiveStyle::Get(), "JustLive.NormalText")
				]
			]
		]
	];
}

FReply SMainMenu::OnPlayClicked()
{
	if (GameInstance.IsValid())
	{
		if (UMapLoader* Loader = GameInstance->GetSubsystem<UMapLoader>())
		{
			// For this prototype, we load the map directly into the current level.
			// In a full game, we would OpenLevel("GameLevel") first.
			Loader->LoadMap(TEXT("@data/maps/example_map.json"));
		}
	}
	return FReply::Handled();
}

FReply SMainMenu::OnModsClicked()
{
	// TODO: Open Mod Browser
	return FReply::Handled();
}

FReply SMainMenu::OnQuitClicked()
{
	if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(GWorld))
	{
		UKismetSystemLibrary::QuitGame(GWorld, PC, EQuitPreference::Quit, true);
	}
	return FReply::Handled();
}
