// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasternautAnimInstance.h"

#include "Blasternaut/BlasternautTypes/CombatState.h"
#include "Blasternaut/Weapon/Weapon.h"
#include "BlasternautCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UBlasternautAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasternautCharacter = Cast<ABlasternautCharacter>(TryGetPawnOwner());
}

void UBlasternautAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!BlasternautCharacter)
	{
		BlasternautCharacter = Cast<ABlasternautCharacter>(TryGetPawnOwner());
		if (!BlasternautCharacter) return; // 23.09 // 
	}

	// 23.09 // FVector Velocity = BlasternautCharacter->GetVelocity();
	// 23.09 // Velocity.Z = 0.f;

	// booleans for animation states
	bIsInAir = BlasternautCharacter->GetMovementComponent()->IsFalling();
	bIsCrouched = BlasternautCharacter->bIsCrouched;
	bIsAccelerating = BlasternautCharacter->GetCharacterMovement()
		->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = BlasternautCharacter->IsWeaponEquipped();
	bAiming = BlasternautCharacter->IsAiming();
	bRotateRootBone = BlasternautCharacter->ShouldRotateRootBone();
	bElimmed = BlasternautCharacter->IsElimmed();

	Speed = BlasternautCharacter->CalculateSpeed(); // 23.09 // Velocity.Size();
	EquippedWeapon = BlasternautCharacter->GetEquippedWeapon();
	TurningInPlace = BlasternautCharacter->GetTurningInPlace();

	// Offset Yaw for Starfing
	FRotator AimRotation = BlasternautCharacter->GetBaseAimRotation(); // global rotation
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasternautCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;
	/*if (!BlasternautCharacter->HasAuthority() && !BlasternautCharacter->IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning, TEXT("MovementRotation Yaw: %f"), MovementRotation.Yaw);
		UE_LOG(LogTemp, Warning, TEXT("AimRotation Yaw: %f"), AimRotation.Yaw);
	}*/

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasternautCharacter->GetActorRotation();

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasternautCharacter->GetAO_Yaw();
	AO_Pitch = BlasternautCharacter->GetAO_Pitch();

	if (bWeaponEquipped && 
		EquippedWeapon && EquippedWeapon->GetWeaponMesh() &&
		BlasternautCharacter->GetMesh()
		)
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->
			GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);

		FVector OutPosition;
		FRotator OutRotation;
		BlasternautCharacter->GetMesh()->
			TransformToBoneSpace(FName("hand_r"),LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);

		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (BlasternautCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTrasform = EquippedWeapon->GetWeaponMesh()->
				GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);

			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandTrasform.GetLocation(),
				RightHandTrasform.GetLocation() + (RightHandTrasform.GetLocation() - BlasternautCharacter->GetHitTarget())
			);
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
	
		//
		//FTransform MuzzleTransform = EquippedWeapon->GetWeaponMesh()
		//	->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		//FVector MuzzleX(FRotationMatrix(MuzzleTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		//DrawDebugLine(GetWorld(), MuzzleTransform.GetLocation(), MuzzleTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
		//DrawDebugLine(GetWorld(), MuzzleTransform.GetLocation(), BlasternautCharacter->GetHitTarget(), FColor::Orange);
		//
	}

	bUseFABRIK = BlasternautCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;

	bool bFABRIKOverride = BlasternautCharacter->IsLocallyControlled() &&
		BlasternautCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade &&
		BlasternautCharacter->IsSwapFinished();

	if (bFABRIKOverride)
	{
		bUseFABRIK = !BlasternautCharacter->IsLocallyReloading();
	}
	
	bUseAimOffsets = BlasternautCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasternautCharacter->GetDisableGameplay();
	bTransformRightHand = BlasternautCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasternautCharacter->GetDisableGameplay();
}
