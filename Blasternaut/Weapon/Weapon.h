// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

/*
* 
*/
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "EquippedSecondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};


UCLASS()
class BLASTERNAUT_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;

	// replicated props
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;

	virtual void Fire(const FVector& HitTarget);

	void ShowPickupWidget(bool bShowWidget);
	void Dropped();
	void AddAmmo(int32 AmmoToAdd);

	
	// --------------- Weapon Crosshairs textures ---------------

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;

	// --------------- Zoom FOV while aiming ---------------
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	 
	// --------------- Automatic weapon fire ---------------

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	// --------------- HUD ---------------
	void TryUpdateHUDAmmo();

	
	// --------------- Custom depth ---------------
	void EnableCustomDepth(bool bEnabled);
	void UpdateOutlineColor(int DepthValue);

	bool bDestroyWeapon = false;

protected:
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();

	virtual void HandleEquippedState();
	virtual void HandleDroppedState();
	virtual void HandleEquippedSecondaryState();

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	virtual void OnSphereEndOverlap(
			UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex
	);

	void SetWeaponPhysics(bool Enable);

	void SetSpecialWeaponPhysics(bool Enable);

private:

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	// --------------- SOUND ---------------
	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	UPROPERTY(EditAnywhere)
	USoundCue* EmptyShotSound;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo;

	UFUNCTION()
	void OnRep_Ammo();

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	// --------------- Owner ---------------
	UPROPERTY()
	class ABlasternautCharacter* OwnerCharacter;
	UPROPERTY()
	class ABlasternautController* OwnerController;

	// --------------- Weapon Type ---------------
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	// used for naming animation sections of weapon in montages
	FName WeaponNameType;


public:

	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh;  }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsFull() const { return (Ammo == MagCapacity); }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE FName GetWeaponTypeName() const { return WeaponNameType;}
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

	FORCEINLINE USoundCue* GetEmptyShotSound() const { return EmptyShotSound; }
	FORCEINLINE USoundCue* GetEquipSound() const { return EquipSound; }
};
