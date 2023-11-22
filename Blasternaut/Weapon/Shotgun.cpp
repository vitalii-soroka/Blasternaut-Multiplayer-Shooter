// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"
#include "Blasternaut/PlayerController/BlasternautController.h"
#include "Blasternaut/CharacterComponents/LagCompensationComponent.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);

	auto* OwnerPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (OwnerPawn == nullptr || MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
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
			/*const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");

			if (bHeadShot)
			{
				if (HeadShotHitMap.Contains(BlasternautCharacter)) ++(HeadShotHitMap[BlasternautCharacter]);
				else HeadShotHitMap.Emplace(BlasternautCharacter, 1);
			}*/
			
			if (HitMap.Contains(BlasternautCharacter)) ++(HitMap[BlasternautCharacter]);
			else HitMap.Emplace(BlasternautCharacter, 1);
			
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

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());

	auto* OwnerPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (OwnerPawn == nullptr || MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector Start = SocketTransform.GetLocation();
	AController* InstigatorController = OwnerPawn->GetController();

	// Maps hit character to number of times hit
	TMap<ABlasternautCharacter*, uint32> HitMap;
	TMap<ABlasternautCharacter*, uint32> HeadShotHitMap;
	for (auto HitTarget : HitTargets)
	{
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		auto* BlasternautCharacter = Cast<ABlasternautCharacter>(FireHit.GetActor());
		if (BlasternautCharacter)
		{
			const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");

			if (bHeadShot)
			{
				if (HeadShotHitMap.Contains(BlasternautCharacter)) ++(HeadShotHitMap[BlasternautCharacter]);
				else HeadShotHitMap.Emplace(BlasternautCharacter, 1);
			}
			else
			{
				if (HitMap.Contains(BlasternautCharacter)) ++(HitMap[BlasternautCharacter]);
				else HitMap.Emplace(BlasternautCharacter, 1);
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

	// ---------- Damage ----------
	TArray<ABlasternautCharacter*> HitCharacters;
	// Maps Character hits in total damage
	TMap<ABlasternautCharacter*, float> DamageMap;

	// Body shot damage calculated by multiplying times hit x Damage
	for (auto HitPair : HitMap)
	{
		if (HitPair.Key)
		{
			DamageMap.Emplace(HitPair.Key, HitPair.Value * Damage);
			HitCharacters.AddUnique(HitPair.Key);
		}
	}
	// Head shot damage calculated by multiplying times hit x HeadShotDamage
	for (auto HitPair : HeadShotHitMap)
	{
		if (HitPair.Key)
		{
			if (DamageMap.Contains(HitPair.Key)) DamageMap[HitPair.Key] += HitPair.Value * HeadShotDamage;
			else DamageMap.Emplace(HitPair.Key, HitPair.Value * HeadShotDamage);
	
			HitCharacters.AddUnique(HitPair.Key);
		}
	}

	// Loop DamageMap to get total damage for every character
	for (auto DamagePair : DamageMap)
	{
		if (DamagePair.Key && InstigatorController)
		{
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
			if (HasAuthority() && bCauseAuthDamage)
			{
				UGameplayStatics::ApplyDamage(
					DamagePair.Key,			// character that was hit
					DamagePair.Value,		// Damage calculated in two loops for body and head
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}

	// ---------- Lag compensation ----------
	if (!HasAuthority() && bUseServerSideRewind)
	{
		OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasternautCharacter>(OwnerPawn) : OwnerCharacter;
		OwnerController = OwnerController == nullptr ? Cast<ABlasternautController>(InstigatorController) : OwnerController;

		if (OwnerCharacter && OwnerController && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled())
		{
			OwnerCharacter->GetLagCompensation()->ShotugunServerScoreRequest(
				HitCharacters,
				Start,
				HitTargets,
				OwnerController->GetServerTime() - OwnerController->SingleTripTime);
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfShards; ++i)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
	
		HitTargets.Add(ToEndLoc);
	}
}
