// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Blasternaut/CharacterComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	auto* BlasternautCharacter = Cast<ABlasternautCharacter>(OtherActor);
	if (BlasternautCharacter)
	{
		UCombatComponent* Combat = BlasternautCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}

	Destroy();
}
