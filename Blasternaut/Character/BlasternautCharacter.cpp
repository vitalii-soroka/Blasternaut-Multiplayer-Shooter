// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasternautCharacter.h"

#include "Blasternaut/Blasternaut.h"
#include "Blasternaut/CharacterComponents/CombatComponent.h"
#include "Blasternaut/CharacterComponents/BuffComponent.h"
#include "Blasternaut/CharacterComponents/LagCompensationComponent.h"
#include "Blasternaut/GameMode/BlasternautGameMode.h"
#include "Blasternaut/PlayerController/BlasternautController.h"
#include "Blasternaut/PlayerState/BlasternautPlayerState.h"
#include "Blasternaut/Weapon/Weapon.h"
#include "Blasternaut/Weapon/WeaponTypes.h"
#include "BlasternautAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blasternaut/GameState/BlasternautGameState.h"

//
// ------------------- Init and Tick -------------------
//
ABlasternautCharacter::ABlasternautCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Camera Setup
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// OverheadWidget
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	/*
	* Combat Component
	*/
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffsComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// Camera ignores character collision
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 600.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	// NET
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	// Grenade
	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/*
	* Hit boxes for server-side rewind
	*/
	
	Pelvis_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	Pelvis_Box->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), Pelvis_Box);

	Spine02_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	Spine02_Box->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), Spine02_Box);

	Spine03_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	Spine03_Box->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), Spine03_Box);

	UpperarmL_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	UpperarmL_Box->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), UpperarmL_Box);

	UpperarmR_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	UpperarmR_Box->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), UpperarmR_Box);

	LowerarmL_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	LowerarmL_Box->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), LowerarmL_Box);

	LowerarmR_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	LowerarmR_Box->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), LowerarmR_Box);

	HandL_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	HandL_Box->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), HandL_Box);

	HandR_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	HandR_Box->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), HandR_Box);

	BackPack_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
	BackPack_Box->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("backpack"), BackPack_Box);

	Blanket_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
	Blanket_Box->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("blanket"), Blanket_Box);

	ThighL_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	ThighL_Box->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), ThighL_Box);

	ThighR_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	ThighR_Box->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), ThighR_Box);

	CalfL_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	CalfL_Box->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), CalfL_Box);

	CalfR_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	CalfR_Box->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), CalfR_Box);

	FootL_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	FootL_Box->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), FootL_Box);

	FootR_Box = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	FootR_Box->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), FootR_Box);

	headBox = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	headBox->SetupAttachment(GetMesh(), FName("headSocket"));
	HitCollisionBoxes.Add(FName("head"), headBox);

	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ABlasternautCharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnDefaultWeapon();

	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();

	// Only server registers damage
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasternautCharacter::ReceiveDamage);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABlasternautCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasternautCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasternautCharacter, Health);
	DOREPLIFETIME(ABlasternautCharacter, Shield);
	DOREPLIFETIME(ABlasternautCharacter, bDisableGameplay);
}

void ABlasternautCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeed
		(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched
		);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasternautController>(Controller);
		}
	}
}

void ABlasternautCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Movement actions inputs
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasternautCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasternautCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasternautCharacter::CrouchButtonPressed);

	// Movement Axis input
	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasternautCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasternautCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasternautCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasternautCharacter::LookUp);

	// Combat input
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasternautCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasternautCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasternautCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasternautCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABlasternautCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ABlasternautCharacter::ThrowGrenadeButtonPressed);
}

void ABlasternautCharacter::PollInit()
{
	if (BlasternautPlayerState == nullptr)
	{
		BlasternautPlayerState = GetPlayerState<ABlasternautPlayerState>();
		if (BlasternautPlayerState)
		{
			BlasternautPlayerState->AddToScore(0.f);
			BlasternautPlayerState->AddToDefeats(0);
			SetTeamColor(BlasternautPlayerState->GetTeam());

			auto* BlasternautGameState = Cast<ABlasternautGameState>(UGameplayStatics::GetGameState(this));
			if (BlasternautGameState && BlasternautGameState->TopScoringPlayers.Contains(BlasternautPlayerState))
			{
				// client - runs on invoking client
				// server - runs on server and all clients
				Multicast_GainedTheLead();
			}

		}
	}
}

void ABlasternautCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PollInit();

	RotateInPlace(DeltaTime);

	HideCameraInCharacter();
}

void ABlasternautCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	BlasternautGameMode = BlasternautGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasternautGameMode>() : BlasternautGameMode;
	bool bMatchIsNotInProgress = BlasternautGameMode && BlasternautGameMode->GetMatchState() != MatchState::InProgress;

	if (Combat && Combat->EquippedWeapon && bMatchIsNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

//
// ------------------- Movement -------------------
//
void ABlasternautCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;

	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasternautCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;

	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasternautCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasternautCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasternautCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasternautCharacter::Jump()
{
	if (bDisableGameplay) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasternautCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMoveReplication += DeltaTime;

		if (TimeSinceLastMoveReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void ABlasternautCharacter::CalculateAO_Pitch()
{
	//
	// Due to compression when sending rotator through network
	// Pitch should be converted back for players that is not locally controlled
	//
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		AO_Pitch -= 360.f;
		//FVector2D InRange(270.f, 360.f);
		//FVector2D OutRange(-90.f, 0.f);
		//AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasternautCharacter::SimProxiesTurn()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	bRotateRootBone = false;

	// 23.09 // float Speed = CalculateSpeed();
	if (CalculateSpeed() > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	//UE_LOG(LogTemp, Warning, TEXT("Proxy Yaw: %f"), ProxyYaw);

	if (FMath::Abs(ProxyYaw) > TurnTreshold)
	{
		if (ProxyYaw > TurnTreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnTreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasternautCharacter::AimOffset(float DeltaTime)
{
	if (Combat && !Combat->EquippedWeapon) return;

	float Speed = CalculateSpeed();

	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir)	// stand
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;

		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || bIsInAir) // run or jump
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

//
// ------------------- Montages -------------------
//
void ABlasternautCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasternautCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		
		/*
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher"); 
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol"); // temp while no pistol animation
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol"); // temp while no submachine animation
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun"); // temp while no shotgun animation
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle"); // temp while no sniper animation
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher"); // temp while no grenade animation
			break;

		default: SectionName = FName("Rifle");
		}
		*/

		AnimInstance->Montage_JumpToSection(Combat->EquippedWeapon->GetWeaponTypeName());
	}
}

void ABlasternautCharacter::PlayElimMontage()
{
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		auto AnimationTimeResult = AnimInstance->Montage_Play(ElimMontage);

		if (AnimationTimeResult == 0.f) UE_LOG(LogTemp, Warning, TEXT("Failed to play Elim montage"));
	}
}

void ABlasternautCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		auto AnimationTimeResult = AnimInstance->Montage_Play(ThrowGrenadeMontage);
		if (AnimationTimeResult == 0.f) UE_LOG(LogTemp, Warning, TEXT("Failed to play throw grenade montage"));
	}
}

void ABlasternautCharacter::PlaySwapWeaponMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapWeaponMontage)
	{
		auto AnimationTimeResult = AnimInstance->Montage_Play(SwapWeaponMontage);
		if (AnimationTimeResult == 0.f) UE_LOG(LogTemp, Warning, TEXT("Failed to play swap weapon montage"));
	}
}

void ABlasternautCharacter::PlayHitReactMontage()
{
	// add montage for unequipped character
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		// add more montages variants, depend on hit
		AnimInstance->Montage_Play(HitReactMontage);
		//FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

//
// ------------------- Buttons -------------------
//
void ABlasternautCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;

	//if (Combat) ***
	if (IsWeaponEquipped())
	{
		Combat->SetAiming(true);
	}
}

void ABlasternautCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasternautCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		// Server RPC
		if (Combat->CombatState == ECombatState::ECS_Unoccupied) ServerEquipButtonPressed();

		// Local animation swap
		bool bSwap = Combat->ShouldSwapWeapons() &&
			!HasAuthority() &&
			Combat->CombatState == ECombatState::ECS_Unoccupied &&
			OverlappingWeapon == nullptr;	
		if (bSwap)
		{
			PlaySwapWeaponMontage();
			Combat->CombatState = ECombatState::ECS_SwappingWeapon;
			bFinishedSwapping = false;
		}
	}
}

void ABlasternautCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}

void ABlasternautCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasternautCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasternautCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->Reload();
	}
}

void ABlasternautCharacter::ThrowGrenadeButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void ABlasternautCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay || GetCharacterMovement()->IsFalling()) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

//
// ------------------- RPC -------------------
//
void ABlasternautCharacter::OnRep_ReplicatedMovement()	
{
	Super::OnRep_ReplicatedMovement();
	
	SimProxiesTurn();
	TimeSinceLastMoveReplication = 0.f;
}

void ABlasternautCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasternautCharacter::OnRep_Health(float LastHealth)
{
	// When health is changed and replicated, we can also call animation for clients
	UpdateHUDHealth();

	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ABlasternautCharacter::OnRep_Shield(float LastShield)
{
	// When health is changed and replicated, we can also call animation for clients
	UpdateHUDShield();

	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void ABlasternautCharacter::Multicast_GainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;

	// Creates effect for the first time
	if (CrownComponent == nullptr)
	{
		// how high should be crown from character mesh
		float offsetZ = bIsCrouched ? 150.f : 110.f;

		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetMesh(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, offsetZ),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	// Activates already created effect
	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void ABlasternautCharacter::Multicast_LostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void ABlasternautCharacter::SetTeamColor(ETeam Team)
{
	// TODO remake this method
	if (GetMesh() == nullptr || DefaultMaterial == nullptr) return;
	switch (Team)
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, DefaultMaterial);
		DissolveMaterialInst = BlueDissolveMatInst;
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		DissolveMaterialInst = BlueDissolveMatInst;
		break;
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		DissolveMaterialInst = RedDissolveMatInst;
		break;
	}
}

//
// ------------------- HUD -------------------
//
void ABlasternautCharacter::UpdateHUDHealth()
{
	BlasternautController = BlasternautController ? BlasternautController : Cast<ABlasternautController>(Controller);

	if (BlasternautController)
	{
		BlasternautController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasternautCharacter::UpdateHUDShield()
{
	BlasternautController = BlasternautController ? BlasternautController : Cast<ABlasternautController>(Controller);

	if (BlasternautController)
	{
		BlasternautController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasternautCharacter::UpdateHUDAmmo()
{
	BlasternautController = BlasternautController ? BlasternautController : Cast<ABlasternautController>(Controller);

	if (BlasternautController && Combat && Combat->EquippedWeapon)
	{
		BlasternautController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasternautController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

//
// ------------------- Default Weapon -------------------
//
void ABlasternautCharacter::SpawnDefaultWeapon()
{
	BlasternautGameMode = BlasternautGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasternautGameMode>() : BlasternautGameMode;
	UWorld* World = GetWorld();

	bool bShouldSpawn = !bElimmed && BlasternautGameMode && DefaultWeaponClass && World && Combat;

	if (bShouldSpawn)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true;
		Combat->EquipWeapon(StartingWeapon);
	}
}

//
// ------------------- Elim -------------------
//
void ABlasternautCharacter::Elim(bool bPlayerLeft)
{
	HandleWeaponsOnElim();

	MulticastElim(bPlayerLeft);
}

void ABlasternautCharacter::OnElimTimerFinished()
{
	BlasternautGameMode = BlasternautGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasternautGameMode>() : BlasternautGameMode;

	if (BlasternautGameMode && !bLeftGame)
	{
		BlasternautGameMode->RequestRespawn(this, Controller);
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void ABlasternautCharacter::MulticastElim_Implementation(bool bPlayerLeft)
{
	bLeftGame = bPlayerLeft;

	// Elim 
	bElimmed = true;
	PlayElimMontage();

	if (BlasternautController) { BlasternautController->SetHUDWeaponAmmo(0); }
	
	// Movement disable
	//GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();

	// Disable fire button
	if (Combat) { Combat->FireButtonPressed(false); }
	
	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Dissolve effect
	if (DissolveMaterialInst)
	{
		DynamicDissolveMaterialInst = UMaterialInstanceDynamic::Create(DissolveMaterialInst, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInst);
	
		DynamicDissolveMaterialInst->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInst->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Spawns elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	// Spawns Elim sound
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
	}

	// Hide Sniper Rifle Scope
	bool HideAimWidget = IsLocallyControlled() && 
		Combat && Combat->bIsAiming && 
		Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (HideAimWidget)
	{
		ShowSniperScopeWidget(false);
	}

	// Destroy lead crown
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}

	//
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasternautCharacter::OnElimTimerFinished,
		ElimDelay
	);
}

void ABlasternautCharacter::ServerLeaveGame_Implementation()
{
	BlasternautGameMode = BlasternautGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasternautGameMode>() : BlasternautGameMode;

	BlasternautPlayerState = BlasternautPlayerState == nullptr ? GetPlayerState<ABlasternautPlayerState>() : BlasternautPlayerState;
	if (BlasternautGameMode && BlasternautPlayerState)
	{
		BlasternautGameMode->PlayerLeftGame(BlasternautPlayerState);
	}
}

void ABlasternautCharacter::HandleWeaponsOnElim()
{
	if (Combat == nullptr) return;
	
	if (Combat->EquippedWeapon)
	{
		HandleWeaponOnElim(Combat->EquippedWeapon);
	}
	if (Combat->SecondaryWeapon)
	{
		HandleWeaponOnElim(Combat->SecondaryWeapon);
	}
}

void ABlasternautCharacter::HandleWeaponOnElim(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;

	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

//
// ------------------- To Sort -------------------
//

// Called only on server
void ABlasternautCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed) return;

	BlasternautGameMode = BlasternautGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasternautGameMode>() : BlasternautGameMode;
	if (BlasternautGameMode == nullptr) return;

	Damage = BlasternautGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	if (Damage <= 0) return;

	// Shield absorbs
	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	// Character should be eliminated
	if (Health == 0.f)
	{
		if (BlasternautGameMode)
		{
			BlasternautController = BlasternautController ? BlasternautController : Cast<ABlasternautController>(Controller);
			auto* AttackerController = Cast<ABlasternautController>(InstigatorController);
			BlasternautGameMode->PlayerEliminated(this, BlasternautController, AttackerController);
		}
	}
}

void ABlasternautCharacter::HideCameraInCharacter()
{
	if (!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

float ABlasternautCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	return Velocity.Size();
}

//
// ------------------- GET - SET - CHECK -------------------
//
AWeapon* ABlasternautCharacter::GetEquippedWeapon() const
{
	return Combat == nullptr ? nullptr : Combat->EquippedWeapon;
}

void ABlasternautCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// for host
	if (OverlappingWeapon && IsLocallyControlled())
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	// Player controlled by host
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

ECombatState ABlasternautCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

FVector ABlasternautCharacter::GetHitTarget() const
{
	if (Combat) return Combat->HitTarget;
	return FVector();
}

void ABlasternautCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInst)
	{
		DynamicDissolveMaterialInst->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasternautCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasternautCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

bool ABlasternautCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasternautCharacter::IsAiming()
{
	return (Combat && Combat->bIsAiming);
}

bool ABlasternautCharacter::IsLocallyReloading() const
{
	return Combat && Combat->bLocallyReloading;
}