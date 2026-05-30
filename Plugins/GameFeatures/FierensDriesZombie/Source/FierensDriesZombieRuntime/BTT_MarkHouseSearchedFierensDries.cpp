#include "BTT_MarkHouseSearchedFierensDries.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "StudentPerceptorFierensDries.h"
#include "Village/House/House.h"

UBTT_MarkHouseSearchedFierensDries::UBTT_MarkHouseSearchedFierensDries()
{
	NodeName = TEXT("Mark House Searched");
	TargetHouseKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_MarkHouseSearchedFierensDries, TargetHouseKey), AHouse::StaticClass());
	TargetHouseKey.SelectedKeyName = TEXT("TargetHouse");
}

EBTNodeResult::Type UBTT_MarkHouseSearchedFierensDries::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
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

	UStudentPerceptorFierensDries* Perceptor = ControlledPawn->FindComponentByClass<UStudentPerceptorFierensDries>();
	if (Perceptor == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	AHouse* TargetHouse = Cast<AHouse>(Blackboard->GetValueAsObject(TargetHouseKey.SelectedKeyName));
	if (TargetHouse == nullptr)
	{
		return EBTNodeResult::Succeeded;
	}

	Perceptor->MarkHouseAsSearched(TargetHouse);
	return EBTNodeResult::Succeeded;
}