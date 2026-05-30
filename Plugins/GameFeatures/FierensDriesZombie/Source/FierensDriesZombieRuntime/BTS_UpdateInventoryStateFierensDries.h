#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_UpdateInventoryStateFierensDries.generated.h"

UCLASS()
class FIERENSDRIESZOMBIERUNTIME_API UBTS_UpdateInventoryStateFierensDries : public UBTService
{
	GENERATED_BODY()

public:
	UBTS_UpdateInventoryStateFierensDries();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};