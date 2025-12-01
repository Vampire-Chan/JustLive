#include "JustLiveStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Sound/SoundBase.h"

TSharedPtr<FSlateStyleSet> FJustLiveStyle::StyleInstance = nullptr;

void FJustLiveStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FJustLiveStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FJustLiveStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("JustLiveStyle"));
	return StyleSetName;
}

const ISlateStyle& FJustLiveStyle::Get()
{
	return *StyleInstance;
}

TSharedPtr<FSlateImageBrush> FJustLiveStyle::CreateDynamicBrush(UTexture2D* Texture, const FVector2D& Size)
{
	if (Texture)
	{
		return MakeShareable(new FSlateImageBrush(Texture, Size));
	}
	return nullptr;
}

void FJustLiveStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef<FSlateStyleSet> FJustLiveStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	
	// Set Content Root (usually Content/Slate)
	Style->SetContentRoot(FPaths::ProjectContentDir() / TEXT("Slate"));
	Style->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	// Text Styles
	const FTextBlockStyle NormalText = FTextBlockStyle()
		.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12))
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black);

	Style->Set("JustLive.NormalText", NormalText);

	const FTextBlockStyle HeaderText = FTextBlockStyle()
		.SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 24))
		.SetColorAndOpacity(FSlateColor::UseForeground())
		.SetShadowOffset(FVector2D(1.0f, 1.0f))
		.SetShadowColorAndOpacity(FLinearColor::Black);

	Style->Set("JustLive.HeaderText", HeaderText);

	// Outlined Text (High Contrast)
	const FTextBlockStyle OutlinedText = FTextBlockStyle()
		.SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D(2.0f, 2.0f))
		.SetShadowColorAndOpacity(FLinearColor::Black);

	Style->Set("JustLive.OutlinedText", OutlinedText);

	// Load UI Sounds
	FSlateSound HoverSound;
	HoverSound.SetResourceObject(LoadObject<USoundBase>(nullptr, TEXT("/Game/Assets/Frontend/UI/hover.hover")));
	
	FSlateSound ClickSound;
	ClickSound.SetResourceObject(LoadObject<USoundBase>(nullptr, TEXT("/Game/Assets/Frontend/UI/click.click")));

	// Glass Button Style
	FButtonStyle GlassButton = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.3f), 4.0f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f), 4.0f))
		.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.2f, 0.2f, 0.2f, 0.6f), 4.0f))
		.SetNormalPadding(FMargin(20, 10))
		.SetPressedPadding(FMargin(20, 12, 20, 8))
		.SetHoveredSound(HoverSound)
		.SetPressedSound(ClickSound);
	
	Style->Set("JustLive.GlassButton", GlassButton);

	return Style;
}
