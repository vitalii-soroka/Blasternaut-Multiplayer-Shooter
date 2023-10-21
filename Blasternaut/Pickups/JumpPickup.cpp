
#include "JumpPickup.h"
#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Blasternaut/CharacterComponents/BuffComponent.h"

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasternautCharacter* BlasternautCharacter = Cast<ABlasternautCharacter>(OtherActor);
	if (BlasternautCharacter)
	{
		UBuffComponent* Buff = BlasternautCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
		}
	}
	Destroy();
}
