#include "WorldCycleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "EngineUtils.h"

void UWorldCycleManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("WorldCycleManager: Initialized."));
}

void UWorldCycleManager::Deinitialize()
{
	Super::Deinitialize();
}

void UWorldCycleManager::RefreshActors()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Find existing actors in the level
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->IsA(ADirectionalLight::StaticClass()))
		{
			SunLight = Cast<ADirectionalLight>(Actor);
		}
		else if (Actor->GetComponentByClass(USkyAtmosphereComponent::StaticClass()))
		{
			SkyAtmosphere = Actor;
		}
		else if (Actor->IsA(AExponentialHeightFog::StaticClass()))
		{
			HeightFog = Actor;
		}
	}

	if (SunLight.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("WorldCycleManager: Found Sun '%s'"), *SunLight->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldCycleManager: No DirectionalLight found!"));
	}
}

void UWorldCycleManager::Tick(float DeltaTime)
{
	if (TimeSpeed > 0.0f)
	{
		// Update Time
		// 24 hours = 86400 seconds
		// If TimeSpeed is 1.0, we advance 1 second per second.
		// In game units, 1.0f = 1 hour.
		// So 1 real second = (1/3600) * TimeSpeed hours.
		
		// Let's simplify: TimeSpeed = GameMinutes per Real Second.
		// If Speed=1, 1 min passes every second. Day = 24 mins.
		float HoursToAdd = (TimeSpeed / 60.0f) * DeltaTime;
		TimeOfDay += HoursToAdd;

		if (TimeOfDay >= 24.0f) TimeOfDay -= 24.0f;
	}

	UpdateSunPosition();
}

void UWorldCycleManager::SetTimeOfDay(float NewTime)
{
	TimeOfDay = FMath::Clamp(NewTime, 0.0f, 24.0f);
	UpdateSunPosition();
}

void UWorldCycleManager::UpdateSunPosition()
{
	if (!SunLight.IsValid()) return;

	// Simple Day/Night Cycle logic
	// 06:00 = Sunrise (0 degrees pitch)
	// 12:00 = Noon (-90 degrees pitch, overhead)
	// 18:00 = Sunset (0 degrees pitch / 180 degrees)
	// 00:00 = Midnight (90 degrees pitch)

	// Map 0-24 to 0-360 degrees
	// We want Noon (12) to be 270 (-90)
	// Midnight (0) to be 90
	
	// Rate: 15 degrees per hour (360 / 24)
	// At 0h:  90 deg
	// At 6h:  0 deg
	// At 12h: -90 deg (270)
	// At 18h: -180 deg (180)
	// At 24h: -270 deg (90)

	float Pitch = 90.0f - (TimeOfDay * 15.0f);
	
	// We can add a slight Yaw rotation to make shadows move interesting
	float Yaw = 45.0f; // Fixed sun angle

	FRotator SunRot(Pitch, Yaw, 0.0f);
	SunLight->SetActorRotation(SunRot);

	// Adjust Light Intensity/Color based on time (Simple Implementation)
	// If needed, we can use Curves here.
}

void UWorldCycleManager::UpdateAtmosphere()
{
	// If we modify the Directional Light rotation, SkyAtmosphere usually updates automatically 
	// if the light is set as "Atmosphere Sun Light".
}
