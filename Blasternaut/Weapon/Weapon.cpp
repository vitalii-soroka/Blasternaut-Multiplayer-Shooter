// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Casing.h"
#include "Blasternaut/PlayerController/BlasternautController.h"
#include "Blasternaut/CharacterComponents/CombatComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty(); // refresh
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);

	// Disable collision detection by default, will be enabled only for server machine
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::EnableCustomDepth(bool bEnabled)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnabled);
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}

	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	// temp moved, animation montages names
	switch (WeaponType)
	{
	case EWeaponType::EWT_AssaultRifle:
		WeaponNameType = FName("Rifle");
		break;
	case EWeaponType::EWT_RocketLauncher:
		WeaponNameType = FName("RocketLauncher");
		break;
	case EWeaponType::EWT_Pistol:
		WeaponNameType = FName("Pistol");
		break;
	case EWeaponType::EWT_SubmachineGun:
		WeaponNameType = FName("Pistol"); 
		break;
	case EWeaponType::EWT_Shotgun:
		WeaponNameType = FName("Shotgun"); 
		break;
	case EWeaponType::EWT_SniperRifle:
		WeaponNameType = FName("SniperRifle"); 
		break;
	case EWeaponType::EWT_GrenadeLauncher:
		WeaponNameType = FName("GrenadeLauncher"); 
		break;
	default: WeaponNameType = FName("Rifle");
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (Owner == nullptr)
	{
		OwnerCharacter = nullptr;
		OwnerController = nullptr;
	}
	else
	{
		OwnerCharacter = OwnerCharacter == nullptr ? Cast <ABlasternautCharacter>(Owner) : OwnerCharacter;

		if (OwnerCharacter && OwnerCharacter->GetEquippedWeapon() == this/* OwnerCharacter->GetEquippedWeapon() && */)
		{
			TryUpdateHUDAmmo();
		}
		
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasternautCharacter* BlasternautCharacter = Cast<ABlasternautCharacter>(OtherActor);

	if (BlasternautCharacter)
	{
		BlasternautCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasternautCharacter* BlasternautCharacter = Cast<ABlasternautCharacter>(OtherActor);
	if (BlasternautCharacter)
	{
		BlasternautCharacter->SetOverlappingWeapon(nullptr);
	}

}

void AWeapon::SetWeaponPhysics(bool Enable)
{
	WeaponMesh->SetSimulatePhysics(Enable);
	WeaponMesh->SetEnableGravity(Enable);

	WeaponMesh->SetCollisionEnabled(
		Enable ? 
		ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision
	);
}

void AWeapon::SetSpecialWeaponPhysics(bool Enable)
{

	if (Enable)
	{
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}
	}
	else
	{
		if (WeaponType == EWeaponType::EWT_SubmachineGun)
		{
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
			WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
			WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		}
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	TryUpdateHUDAmmo();
}

void AWeapon::OnRep_Ammo()
{
	TryUpdateHUDAmmo();
	if (OwnerCharacter && OwnerCharacter->GetCombat() && IsFull())
	{
		OwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
}

void AWeapon::TryUpdateHUDAmmo()
{
	OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasternautCharacter>(GetOwner()) : OwnerCharacter;
	if (OwnerCharacter)
	{
		OwnerController = OwnerController == nullptr ? Cast<ABlasternautController>(OwnerCharacter->Controller) : OwnerController;
		if (OwnerController)
		{
			OwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::UpdateOutlineColor(int DepthValue)
{
	if (WeaponMesh) 
	{
		WeaponMesh->SetCustomDepthStencilValue(DepthValue);
		WeaponMesh->MarkRenderStateDirty();
		EnableCustomDepth(true);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;

	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		HandleEquippedState();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		HandleEquippedSecondaryState();
		break;
	case EWeaponState::EWS_Dropped:
		HandleDroppedState();
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		HandleEquippedState();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		HandleEquippedSecondaryState();
		break;
	case EWeaponState::EWS_Dropped:
		HandleDroppedState();
		break;
	}
}

void AWeapon::HandleEquippedState()
{
	if (HasAuthority()) AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ShowPickupWidget(false);
	SetWeaponPhysics(false);
	SetSpecialWeaponPhysics(true);
	EnableCustomDepth(false);
}

void AWeapon::HandleDroppedState()
{
	if (HasAuthority()) AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	SetWeaponPhysics(true);
	SetSpecialWeaponPhysics(false);
	UpdateOutlineColor(CUSTOM_DEPTH_BLUE);
}

void AWeapon::HandleEquippedSecondaryState()
{
	if (HasAuthority()) AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ShowPickupWidget(false);
	SetWeaponPhysics(false);
	SetSpecialWeaponPhysics(true);
	EnableCustomDepth(true);
	UpdateOutlineColor(CUSTOM_DEPTH_TAN);

}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));

		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}

	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);

	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo - AmmoToAdd, 0, MagCapacity);
	TryUpdateHUDAmmo();
}

