// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasternautController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERNAUT_API ABlasternautController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void BeginPlay() override;

private:
	void TrySetHUD();
	bool IsOverlayValid() const;


private:
	UPROPERTY()
	class ABlasternautHUD* BlasternautHUD = nullptr;

public:
};
