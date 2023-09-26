// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasternautController.h"
#include "Blasternaut/HUD/BlasternautHUD.h"
#include "Blasternaut/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void ABlasternautController::BeginPlay()
{
	Super::BeginPlay();

	BlasternautHUD = Cast<ABlasternautHUD>(GetHUD());
}

void ABlasternautController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasternautHUD = BlasternautHUD ? BlasternautHUD : Cast<ABlasternautHUD>(GetHUD());

	if (BlasternautHUD && 
		BlasternautHUD->CharacterOverlay && 
		BlasternautHUD->CharacterOverlay->HealthBar &&
		BlasternautHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasternautHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		
		 FString HealthText = FString::Printf(TEXT("%d/%d"), 
			FMath::CeilToInt(Health),FMath::CeilToInt(MaxHealth));

		BlasternautHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}