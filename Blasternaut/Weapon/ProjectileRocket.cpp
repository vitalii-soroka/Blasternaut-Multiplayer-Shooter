// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,						// World context 
				Damage,						// Base Damage
				Damage / 4.f,				// Minimum Damage
				GetActorLocation(),			// Origin Location
				DamageInnerRadius,			// DamageInnerRadius
				DamageOuterRadius,			// DamageOuterRadius
				1.f,						// Damage FallOff
				UDamageType::StaticClass(), // Damage Type
				TArray<AActor*>(),			// Ignore Actors
				this,						// Damage Causer
				FiringController			// Instigator Controller
			);
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
