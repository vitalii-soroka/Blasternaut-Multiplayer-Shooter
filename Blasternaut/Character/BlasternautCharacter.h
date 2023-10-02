// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blasternaut/BlasternautTypes/TurningInPlace.h"
#include "Blasternaut/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blasternaut/BlasternautTypes/CombatState.h"
#include "BlasternautCharacter.generated.h"

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

	// --------------- Elimination and Destroying ---------------
	void Elim();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	virtual void Destroyed() override;

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

	// --------------- Damage ---------------
	UFUNCTION()
	void ReceiveDamage(
		AActor* DamageActor, 
		float Damage, 
		const UDamageType* DamageType, 
		class AController* InstigatorController,
		AActor* DamageCauser
	);

	// --------------- HUD ---------------
	void UpdateHUDHealth();

	// Poll for any relevant classes and initialize class
	void PollInit();

private:
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

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
	void OnRep_Health();

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
	UPROPERTY(EditAnywhere, Category = "Elim")
	UMaterialInstance* DissolveMaterialInst;

	// --------------- Elim Bot ---------------
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

public:
	bool IsAiming();
	bool IsWeaponEquipped();
	void SetOverlappingWeapon(AWeapon* Weapon);

	FVector GetHitTarget() const;
	AWeapon* GetEquippedWeapon() const;

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	ECombatState GetCombatState() const;
};
