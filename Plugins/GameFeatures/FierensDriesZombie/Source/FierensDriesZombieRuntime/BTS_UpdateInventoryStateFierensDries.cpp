#include "BTS_UpdateInventoryStateFierensDries.h"

#include "AIController.h"
#include "StudentPerceptorFierensDries.h"

UBTS_UpdateInventoryStateFierensDries::UBTS_UpdateInventoryStateFierensDries()
{
	NodeName = TEXT("Update Inventory State");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
}

void UBTS_UpdateInventoryStateFierensDries::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (ControlledPawn == nullptr)
	{
		return;
	}

	if (UStudentPerceptorFierensDries* Perceptor = ControlledPawn->FindComponentByClass<UStudentPerceptorFierensDries>())
	{
		Perceptor->UpdateInventoryBlackboard();
	}
}