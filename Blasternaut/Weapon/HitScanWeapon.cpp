// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"
#include "Blasternaut/CharacterComponents/LagCompensationComponent.h"
#include "Blasternaut/PlayerController/BlasternautController.h"
//#include "DrawDebugHelpers.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	auto* OwnerPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if ( OwnerPawn == nullptr || MuzzleFlashSocket == nullptr) return;

	FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector Start = SocketTransform.GetLocation();
	
	FHitResult FireHit;
	WeaponTraceHit(Start, HitTarget, FireHit);

	// InstigatorController not valid on simulated proxy as they don't have controller
	AController* InstigatorController = OwnerPawn->GetController();
	auto* BlasternautCharacter = Cast<ABlasternautCharacter>(FireHit.GetActor());
	
	// Damage
	if (BlasternautCharacter && InstigatorController)
	{
		bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
		// On server
		if (HasAuthority() && bCauseAuthDamage)
		{
			const float DamageToCause = FireHit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;

			UGameplayStatics::ApplyDamage(
				BlasternautCharacter,
				DamageToCause,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);
		}
		
		if (!HasAuthority() && bUseServerSideRewind) // Lag Compensation
		{
			OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasternautCharacter>(OwnerPawn) : OwnerCharacter;
			OwnerController = OwnerController == nullptr ? Cast<ABlasternautController>(InstigatorController) : OwnerController;

			if (OwnerCharacter && OwnerController && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled())
			{
				OwnerCharacter->GetLagCompensation()->ServerScoreRequest(
					BlasternautCharacter,
					Start,
					HitTarget,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime
				);
			}
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
			FireHit.ImpactPoint
		);
	}

	// Played if no sounds and effects in fire animation
	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			MuzzleFlash,
			SocketTransform
		);
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSound,
			GetActorLocation()
		);
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	
	const FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		
	World->LineTraceSingleByChannel(
		OutHit,
		TraceStart,
		End,
		ECollisionChannel::ECC_Visibility
	);

	FVector BeamEnd = End;
	if (OutHit.bBlockingHit)
	{
		BeamEnd = OutHit.ImpactPoint;
	}
	else
	{
		OutHit.ImpactPoint = End;
	}

	//DrawDebugSphere(GetWorld(), BeamEnd, 16.f, 12.f, FColor::Orange, true);

	if (BeamParticles)
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
			World,
			BeamParticles,
			TraceStart,
			FRotator::ZeroRotator,
			true
		);

		if (Beam)
		{
			Beam->SetVectorParameter(FName("Target"), BeamEnd);
		}
	}
}
