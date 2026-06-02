#include "BTT_WanderFierensDries.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTT_WanderFierensDries::UBTT_WanderFierensDries()
{
	NodeName = TEXT("Wander");
	MoveLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_WanderFierensDries, MoveLocationKey));
	MoveLocationKey.SelectedKeyName = TEXT("MoveLocation");
}

EBTNodeResult::Type UBTT_WanderFierensDries::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (ControlledPawn == nullptr || Blackboard == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	FVector RandomDirection = FMath::VRand();
	RandomDirection.Z = 0.0f;
	RandomDirection.Normalize();

	const FVector RandomLocation = ControlledPawn->GetActorLocation() + RandomDirection * FMath::FRandRange(0.0f, Radius);
	Blackboard->SetValueAsVector(MoveLocationKey.SelectedKeyName, RandomLocation);

	return EBTNodeResult::Succeeded;
}