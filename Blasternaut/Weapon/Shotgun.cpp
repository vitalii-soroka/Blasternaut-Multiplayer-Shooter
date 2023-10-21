// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);

	auto* OwnerPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (OwnerPawn == nullptr || MuzzleFlashSocket == nullptr) return;

	FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector Start = SocketTransform.GetLocation();
	AController* InstigatorController = OwnerPawn->GetController();

	// Hits
	TMap<ABlasternautCharacter*, uint32> HitMap;
	for (uint32 i = 0; i < NumberOfShards; ++i)
	{
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		auto* BlasternautCharacter = Cast<ABlasternautCharacter>(FireHit.GetActor());
		if (BlasternautCharacter && HasAuthority() && InstigatorController)
		{
			if (HitMap.Contains(BlasternautCharacter))
			{
				++(HitMap[BlasternautCharacter]);
			}
			else
			{
				HitMap.Emplace(BlasternautCharacter, 1);
			}
		}

		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint,
				.5f,
				FMath::FRandRange(-.5f, .5f)
			);
		}

	}	
	
	// Damage
	for (auto HitPair : HitMap)
	{
		if (HasAuthority() && HitPair.Key && InstigatorController)
		{
			UGameplayStatics::ApplyDamage(
				HitPair.Key,
				Damage * HitPair.Value,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);
		}
	}
}
