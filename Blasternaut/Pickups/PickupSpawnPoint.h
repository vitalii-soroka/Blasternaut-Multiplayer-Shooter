#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

UCLASS()
class BLASTERNAUT_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	
	APickupSpawnPoint();
	virtual void Tick(float DeltaTime) override;

protected:
	
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<class APickup>> PickupClasses;

	UPROPERTY()
	APickup* SpawnedPickup = nullptr;

	// Called on pickup destruction
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);
	
	void SpawnPickup();
	void SpawnPickupTimerFinished();

private:

	FTimerHandle SpawnPickupTimer;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMin;

	UPROPERTY(EditAnywhere)
	float SpawnPickupTimeMax;

public:	


};
