#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Items/ItemType.h"
#include "BTT_UseInventoryItemFierensDries.generated.h"

UCLASS()
class FIERENSDRIESZOMBIERUNTIME_API UBTT_UseInventoryItemFierensDries : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_UseInventoryItemFierensDries();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	EItemType ItemTypeToUse{EItemType::Food};
};