// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blasternaut/BlasternautTypes/TurningInPlace.h"
#include "Blasternaut/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blasternaut/BlasternautTypes/CombatState.h"
#include "Blasternaut/BlasternautTypes/Team.h"
#include "BlasternautCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

UCLASS()
class BLASTERNAUT_API ABlasternautCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasternautCharacter();
	
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	// --------------- ---------------
	float CalculateSpeed();
	virtual void OnRep_ReplicatedMovement() override;

	// --------------- Playing Montages ---------------
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayHitReactMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapWeaponMontage();

	// --------------- Elimination and Destroying ---------------
	void Elim(bool bPlayerLeft);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeft);
	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();

	FOnLeftGame OnLeftGame;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_GainedTheLead();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_LostTheLead();

	// --------------- Team ---------------

	void SetTeamColor(ETeam Team);

	// --------------- HUD ---------------
	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	// --------------- DefaultWeapon ---------------
	void SpawnDefaultWeapon();

	// --------------- HitBoxes ---------------
	// Hitboxes used for server-side rewind;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* Pelvis_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* headBox;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Spine02_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Spine03_Box;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* UpperarmL_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* UpperarmR_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* LowerarmL_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* LowerarmR_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HandL_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HandR_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* BackPack_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* Blanket_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* ThighL_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* ThighR_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* CalfL_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* CalfR_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* FootL_Box;

	UPROPERTY(EditAnywhere)
	UBoxComponent* FootR_Box;

	UPROPERTY()
	TMap<FName, UBoxComponent*> HitCollisionBoxes;

protected:

	virtual void BeginPlay() override;

	// --------------- Movement ---------------
	virtual void Jump() override;
	void CalculateAO_Pitch();
	void SimProxiesTurn();
	void Turn(float Value);
	void LookUp(float Value);
	void MoveRight(float Value);
	void MoveForward(float Value);
	void AimOffset(float DeltaTime);

	// --------------- Button Pressed ---------------
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void ThrowGrenadeButtonPressed();

	// --------------- Damage ---------------
	UFUNCTION()
	void ReceiveDamage(
		AActor* DamageActor, 
		float Damage, 
		const UDamageType* DamageType, 
		class AController* InstigatorController,
		AActor* DamageCauser
	);

	// Poll for any relevant classes and initialize class
	void PollInit();
	void RotateInPlace(float DeltaTime);

	// --------------- Grenade ---------------
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

private:
	UPROPERTY()
	class ABlasternautGameMode* BlasternautGameMode;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	// --------------- Actor Components ---------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UBuffComponent* Buff;
	
	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float AO_Pitch;
	float InterpAO_Yaw;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	
	// --------------- Montages ---------------
	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* SwapWeaponMontage;

	bool bFinishedSwapping = false;

	void HideCameraInCharacter();

	UPROPERTY(EditAnywhere)
	float CameraThreshold  = 200.f;

	bool bRotateRootBone;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float TurnTreshold = 0.5f;

	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;

	float ProxyYaw;
	float TimeSinceLastMoveReplication;
	
	// --------------- Player Health ---------------
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;
	
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	// --------------- Player Shield ---------------
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats")
	float Shield = 20.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class ABlasternautController* BlasternautController;

	// --------------- Disolve Effect ---------------
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	
	FOnTimelineFloat DissolveTrack;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	// Dynamic instance that can be changed at runtime
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInst;

	// Material instance set in Blueprints, used with dynamic material instance
	UPROPERTY(VisibleAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialInst;

	// --------------- Team Colors ---------------

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DefaultMaterial;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* RedDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* RedMaterial;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* BlueDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* BlueMaterial;

	// --------------- Elim Effect ---------------
	FTimerHandle ElimTimer;
	void OnElimTimerFinished();

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	bool bElimmed = false;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY()
	class ABlasternautPlayerState* BlasternautPlayerState = nullptr;
	
	bool bLeftGame = false;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;
	
	UPROPERTY()
	class UNiagaraComponent* CrownComponent;


	// --------------- Default Weapon ---------------
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	void HandleWeaponsOnElim();
	void HandleWeaponOnElim(AWeapon* Weapon);

	
public:
	bool IsAiming();
	bool IsWeaponEquipped();
	void SetOverlappingWeapon(AWeapon* Weapon);

	FVector GetHitTarget() const;
	AWeapon* GetEquippedWeapon() const;

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float HealthToSet) { Health = HealthToSet; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	ECombatState GetCombatState() const;

	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }

	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	bool IsLocallyReloading() const;

	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	//FORCEINLINE const TMap<FName, class UBoxComponent*> GetHitCollisionBoxes() { return HitCollisionBoxes; }
	FORCEINLINE bool IsSwapFinished() const { return bFinishedSwapping; }
	FORCEINLINE void SetSwapFinished(bool SwapFinished) { bFinishedSwapping = SwapFinished; }
};
