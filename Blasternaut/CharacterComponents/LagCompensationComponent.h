#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ABlasternautCharacter;

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	ABlasternautCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABlasternautCharacter*, uint32> HeadShots;
	UPROPERTY()
	TMap<ABlasternautCharacter*, uint32> BodyShots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTERNAUT_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	ULagCompensationComponent();
	friend class ABlasternautCharacter;

	void SaveFramePackage(FFramePackage& Package);

	// ---------- HitScan Weapon ----------

	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		ABlasternautCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

	FServerSideRewindResult ServerSideRewind(
		ABlasternautCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

	// ---------- Projectile Weapon ----------
	UFUNCTION(Server, Reliable)
	void ServerProjectileScoreRequest(
		ABlasternautCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	FServerSideRewindResult ProjectileServerSideRewind(
		ABlasternautCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	// ---------- Shotgun Weapon ----------
	UFUNCTION(Server, Reliable)
	void ShotugunServerScoreRequest(
		const TArray<ABlasternautCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
	);

	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ABlasternautCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
	);

protected:
	
	virtual void BeginPlay() override;

	void SaveFramePackage();
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

	FFramePackage InterpBetweenFrames(
		const FFramePackage& OlderFrame,
		const FFramePackage& YoungerFrame,
		float HitTime);

	void CacheBoxPosition(ABlasternautCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ABlasternautCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ABlasternautCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ABlasternautCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
	FFramePackage GetFrameToCheck(ABlasternautCharacter* HitCharacter, float HitTime);

	// ---------- HitScan ----------

	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package,
		ABlasternautCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation
	);

	// ---------- Projectile ----------

	FServerSideRewindResult ProjectileConfirmHit(
		const FFramePackage& Package,
		ABlasternautCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	// ---------- Shotgun ----------

	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations
	);

private:

	UPROPERTY()
	ABlasternautCharacter* Character;

	UPROPERTY()
	class ABlasternautController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;

private:

	bool IsFrameHistoryValid(ABlasternautCharacter* HitCharacter) const;

public:	
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
};
