#include "BTT_UseInventoryItemFierensDries.h"

#include "AIController.h"
#include "StudentPerceptorFierensDries.h"

UBTT_UseInventoryItemFierensDries::UBTT_UseInventoryItemFierensDries()
{
	NodeName = TEXT("Use Inventory Item");
}

EBTNodeResult::Type UBTT_UseInventoryItemFierensDries::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (ControlledPawn == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	UStudentPerceptorFierensDries* Perceptor = ControlledPawn->FindComponentByClass<UStudentPerceptorFierensDries>();
	if (Perceptor == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	return Perceptor->UseBestInventoryItem(ItemTypeToUse)
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}