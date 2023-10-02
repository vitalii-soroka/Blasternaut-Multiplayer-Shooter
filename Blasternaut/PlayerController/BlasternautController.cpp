// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasternautController.h"
#include "Blasternaut/HUD/BlasternautHUD.h"
#include "Blasternaut/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blasternaut/Character/BlasternautCharacter.h"

void ABlasternautController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	auto* BlasternautCharacter = Cast<ABlasternautCharacter>(InPawn);
	if (BlasternautCharacter)
	{
		SetHUDHealth(BlasternautCharacter->GetHealth(), BlasternautCharacter->GetMaxHealth());
	}
}

void ABlasternautController::BeginPlay()
{
	Super::BeginPlay();

	BlasternautHUD = Cast<ABlasternautHUD>(GetHUD());
}

void ABlasternautController::TrySetHUD()
{
	BlasternautHUD = BlasternautHUD == nullptr ? Cast<ABlasternautHUD>(GetHUD()) : BlasternautHUD;
}

bool ABlasternautController::IsOverlayValid() const
{
	return BlasternautHUD && BlasternautHUD->CharacterOverlay;
}

//
// SET HUD
//

void ABlasternautController::SetHUDHealth(float Health, float MaxHealth)
{
	TrySetHUD();

	if (IsOverlayValid() &&
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

void ABlasternautController::SetHUDScore(float Score)
{
	TrySetHUD();

	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->ScoreAmount)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasternautHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void ABlasternautController::SetHUDDefeats(int32 Defeats)
{
	TrySetHUD();

	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->DefeatsAmount)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasternautHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void ABlasternautController::SetHUDWeaponAmmo(int32 Ammo)
{
	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->WeaponAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasternautHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasternautController::SetHUDCarriedAmmo(int32 Ammo)
{
	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->CarriedAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasternautHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}
