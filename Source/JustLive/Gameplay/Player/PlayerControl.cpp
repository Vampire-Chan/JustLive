#include "PlayerControl.h"

APlayerControl::APlayerControl()
{
	// Enable Mouse Cursor?
	bShowMouseCursor = false; 
}

void APlayerControl::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerControl::SetupInputComponent()
{
	Super::SetupInputComponent();
	// Bind Controller-specific inputs here (e.g. Pause Menu)
}
