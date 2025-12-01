#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Texture2D.h"
#include "TextureManager.generated.h"

/**
 * Texture Manager
 * Handles runtime loading of textures from disk (png, jpg).
 */
UCLASS()
class JUSTLIVE_API UTextureManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UTextureManager* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Loads a texture from a virtual path.
	 * @param VirtualPath - e.g., "@data/textures/myimg.png"
	 * @return Loaded Texture2D or nullptr if failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	UTexture2D* LoadTexture(const FString& VirtualPath);

private:
	// Cache of loaded textures to prevent reloading
	UPROPERTY()
	TMap<FString, UTexture2D*> TextureCache;

	class UVFSManager* GetVFS() const;
};
