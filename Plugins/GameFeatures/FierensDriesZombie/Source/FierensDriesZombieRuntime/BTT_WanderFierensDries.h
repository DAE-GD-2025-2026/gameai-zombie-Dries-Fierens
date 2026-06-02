#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTT_WanderFierensDries.generated.h"

UCLASS()
class FIERENSDRIESZOMBIERUNTIME_API UBTT_WanderFierensDries : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_WanderFierensDries();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MoveLocationKey;

	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
	float Radius = 1500.0f;
};