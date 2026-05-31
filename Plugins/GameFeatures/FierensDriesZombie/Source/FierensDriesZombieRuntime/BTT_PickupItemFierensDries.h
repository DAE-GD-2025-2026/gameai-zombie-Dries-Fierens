#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTT_PickupItemFierensDries.generated.h"

UCLASS()
class FIERENSDRIESZOMBIERUNTIME_API UBTT_PickupItemFierensDries : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_PickupItemFierensDries();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetItemKey;
};