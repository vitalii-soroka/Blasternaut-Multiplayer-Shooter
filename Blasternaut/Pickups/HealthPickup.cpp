// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"
#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Blasternaut/CharacterComponents/BuffComponent.h"


AHealthPickup::AHealthPickup()
{
	bReplicates = true;

}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasternautCharacter* BlasternautCharacter = Cast<ABlasternautCharacter>(OtherActor);
	if (BlasternautCharacter)
	{
		UBuffComponent* Buff = BlasternautCharacter->GetBuff();
		if (Buff)
		{
			Buff->Heal(HealAmount, HealingTime);
		}
	}
	Destroy();
}
