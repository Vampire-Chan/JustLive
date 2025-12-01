#include "SGameHUDWidget.h"
#include "JustLiveStyle.h"
#include "../Managers/TextureManager.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

void SGameHUDWidget::Construct(const FArguments& InArgs)
{
	GameInstance = InArgs._GameInstance;

	// Load Textures
	if (GameInstance.IsValid())
	{
		if (UTextureManager* TexMgr = GameInstance->GetSubsystem<UTextureManager>())
		{
			if (UTexture2D* Tex = TexMgr->LoadTexture(TEXT("@data/ui/crosshair_icon.png")))
				CrosshairBrush = FJustLiveStyle::CreateDynamicBrush(Tex, FVector2D(32, 32));
			
			if (UTexture2D* Tex = TexMgr->LoadTexture(TEXT("@data/ui/health_icon.png")))
				HealthIconBrush = FJustLiveStyle::CreateDynamicBrush(Tex, FVector2D(32, 32));

			if (UTexture2D* Tex = TexMgr->LoadTexture(TEXT("@data/ui/weapon_icon.png")))
				AmmoIconBrush = FJustLiveStyle::CreateDynamicBrush(Tex, FVector2D(32, 32));
		}
	}

	ChildSlot
	[
		SNew(SOverlay)
		
		// Crosshair (Center)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			CrosshairBrush.IsValid() ?
			static_cast<TSharedRef<SWidget>>(SNew(SImage).Image(CrosshairBrush.Get())) :
			static_cast<TSharedRef<SWidget>>(SNew(STextBlock)
				.Text(FText::FromString("+"))
				.TextStyle(FJustLiveStyle::Get(), "JustLive.NormalText")
				.ColorAndOpacity(FLinearColor::White))
		]

		// Bottom Left: Radar (Placeholder) & Health/Armor
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(40)
		[
			SNew(SHorizontalBox)
			// Radar Placeholder
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 20, 0)
			[
				SNew(SBox)
				.WidthOverride(128)
				.HeightOverride(128)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush")) // Placeholder for Radar
					.ColorAndOpacity(FLinearColor(0, 0, 0, 0.5f)) // Semi-transparent circle
				]
			]
			// Health & Armor
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 10, 0)
					[
						HealthIconBrush.IsValid() ?
						static_cast<TSharedRef<SWidget>>(SNew(SImage).Image(HealthIconBrush.Get()).ColorAndOpacity(FLinearColor::White)) :
						static_cast<TSharedRef<SWidget>>(SNullWidget::NullWidget)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SGameHUDWidget::GetHealthText)
						.TextStyle(FJustLiveStyle::Get(), "JustLive.OutlinedText") // New Style
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString("ARMOR: 100")) // Placeholder
					.TextStyle(FJustLiveStyle::Get(), "JustLive.OutlinedText")
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 1.0f)) // Light Blue
				]
			]
		]

		// Bottom Right: Weapon & Ammo
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(40)
		[
			SNew(SVerticalBox)
			// Context Info (Above Weapon)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0, 0, 0, 10)
			[
				SNew(STextBlock)
				.Text(FText::FromString("ZONE: Safe Haven"))
				.TextStyle(FJustLiveStyle::Get(), "JustLive.OutlinedText")
				.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 0.5f)) // Yellowish
			]
			// Weapon & Ammo
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 15, 0)
				[
					SNew(STextBlock)
					.Text(this, &SGameHUDWidget::GetAmmoText)
					.TextStyle(FJustLiveStyle::Get(), "JustLive.HeaderText") // Big Text for Ammo
					.ColorAndOpacity(FLinearColor::White)
					.ShadowOffset(FVector2D(2, 2))
					.ShadowColorAndOpacity(FLinearColor::Black)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					AmmoIconBrush.IsValid() ? // Using Weapon Icon here
					static_cast<TSharedRef<SWidget>>(SNew(SImage).Image(AmmoIconBrush.Get()).ColorAndOpacity(FLinearColor::White)) :
					static_cast<TSharedRef<SWidget>>(SNullWidget::NullWidget)
				]
			]
		]

		// Top Right: Objectives
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(40)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString("OBJECTIVES"))
				.TextStyle(FJustLiveStyle::Get(), "JustLive.OutlinedText")
				.ColorAndOpacity(FLinearColor::Green)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString("- Explore the World"))
				.TextStyle(FJustLiveStyle::Get(), "JustLive.NormalText")
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(FLinearColor::Black)
			]
		]

		// Top Center: Global Messages
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0, 20, 0, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Welcome to JustLive Sandbox"))
			.TextStyle(FJustLiveStyle::Get(), "JustLive.OutlinedText")
			.ColorAndOpacity(FLinearColor(1.0f, 0.5f, 0.0f)) // Orange
		]
	];
}

FText SGameHUDWidget::GetHealthText() const
{
	// TODO: Bind to actual player health
	return FText::FromString("HP: 100");
}

FText SGameHUDWidget::GetAmmoText() const
{
	// TODO: Bind to actual weapon ammo
	return FText::FromString("AMMO: 30/90");
}
