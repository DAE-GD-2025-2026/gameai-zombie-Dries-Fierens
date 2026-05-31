#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTT_ShootTargetEnemyFierensDries.generated.h"

UCLASS()
class FIERENSDRIESZOMBIERUNTIME_API UBTT_ShootTargetEnemyFierensDries : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_ShootTargetEnemyFierensDries();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetEnemyKey;
};