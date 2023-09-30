// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasternautPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERNAUT_API ABlasternautPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//
	// Replication notifies
	//
	UFUNCTION()
	virtual void OnRep_Defeats();
	virtual void OnRep_Score() override;

	void AddToScore(float ScoreAmount); 
	void AddToDefeats(int32 DefeatsAmount);

protected:

private:
	UPROPERTY()
	class ABlasternautCharacter* Character = nullptr;
	UPROPERTY()
	class ABlasternautController* Controller = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	bool TrySetController();
};
