#include "TextureManager.h"
#include "../Core/VFS/VFSManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Kismet/GameplayStatics.h"

UTextureManager* UTextureManager::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UTextureManager>();
	}
	return nullptr;
}

void UTextureManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency(UVFSManager::StaticClass());
	UE_LOG(LogTemp, Log, TEXT("TextureManager Initialized."));
}

UVFSManager* UTextureManager::GetVFS() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UVFSManager>();
	}
	return nullptr;
}

UTexture2D* UTextureManager::LoadTexture(const FString& VirtualPath)
{
	// Check cache first
	if (TextureCache.Contains(VirtualPath))
	{
		return TextureCache[VirtualPath];
	}

	// 1. Handle @content (Unreal Assets)
	if (VirtualPath.StartsWith(TEXT("@content")))
	{
		// Convert @content/MyFolder/MyTex -> /Game/MyFolder/MyTex
		FString PackagePath = VirtualPath;
		PackagePath.RemoveFromStart(TEXT("@content"));
		if (PackagePath.StartsWith(TEXT("/")) || PackagePath.StartsWith(TEXT("\\")))
		{
			PackagePath = PackagePath.RightChop(1);
		}
		PackagePath = TEXT("/Game/") + PackagePath;

		// Load Object (Synchronous for now, can be async later)
		UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, *PackagePath);
		if (LoadedTexture)
		{
			// Ensure it's kept alive
			LoadedTexture->AddToRoot(); 
			TextureCache.Add(VirtualPath, LoadedTexture);
			UE_LOG(LogTemp, Log, TEXT("TextureManager: Loaded Asset '%s'"), *PackagePath);
			return LoadedTexture;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("TextureManager: Failed to load Asset '%s'"), *PackagePath);
			return nullptr;
		}
	}

	// 2. Handle Raw Files (@data, @mods)
	UVFSManager* VFS = GetVFS();
	if (!VFS) return nullptr;

	FString PhysicalPath = VFS->Resolve(VirtualPath);
	if (PhysicalPath.IsEmpty()) return nullptr;

	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *PhysicalPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("TextureManager: Failed to load file '%s'"), *PhysicalPath);
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(RawFileData.GetData(), RawFileData.Num());
	
	if (ImageFormat == EImageFormat::Invalid)
	{
		UE_LOG(LogTemp, Warning, TEXT("TextureManager: Invalid image format for '%s'"), *PhysicalPath);
		return nullptr;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		TArray<uint8> UncompressedBGRA;
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		{
			int32 Width = ImageWrapper->GetWidth();
			int32 Height = ImageWrapper->GetHeight();

			UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
			if (NewTexture)
			{
				void* TextureData = NewTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
				NewTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

				NewTexture->UpdateResource();
				
				// Cache it
				NewTexture->AddToRoot(); // Prevent GC
				TextureCache.Add(VirtualPath, NewTexture);
				UE_LOG(LogTemp, Log, TEXT("TextureManager: Loaded texture '%s'"), *VirtualPath);
				return NewTexture;
			}
		}
	}

	UE_LOG(LogTemp, Error, TEXT("TextureManager: Failed to decode image '%s'"), *PhysicalPath);
	return nullptr;
}
