#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTERNAUT_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	friend class ABlasternautCharacter;

	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Heal(float HealAmount, float HealingTime);
	void ReplenishShield(float Amount, float Time);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffJump(float BuffJumpVelocity, float BuffTime);
	void SetInitialSpeed(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float VelocityZ);

protected:
	
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);

private:

	UPROPERTY()
	class ABlasternautCharacter* Character;

	//
	// ---------- Heal Buff ----------
	//
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	//
	// ---------- Speed Buff ----------
	//
	FTimerHandle SpeedBuffTimer;
	void ResetSpeed();
	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	//
	// ---------- Jump Buff ----------
	//
	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);

	//
	// ---------- Shield Buff ----------
	//
	bool bReplenishShield = false;
	float ShieldReplenishRate = 0.f;
	float ShieldReplenishAmount = 0.f;

public:	
	
	FORCEINLINE bool IsCharacterActive() const;
};
