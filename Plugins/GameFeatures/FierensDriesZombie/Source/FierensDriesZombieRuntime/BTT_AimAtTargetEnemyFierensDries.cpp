#include "BTT_AimAtTargetEnemyFierensDries.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "StudentPerceptorFierensDries.h"

UBTT_AimAtTargetEnemyFierensDries::UBTT_AimAtTargetEnemyFierensDries()
{
	NodeName = TEXT("Aim At Target Enemy");
	TargetEnemyKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_AimAtTargetEnemyFierensDries, TargetEnemyKey), AActor::StaticClass());
	TargetEnemyKey.SelectedKeyName = TEXT("TargetEnemy");
}

EBTNodeResult::Type UBTT_AimAtTargetEnemyFierensDries::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	UStudentPerceptorFierensDries* Perceptor = ControlledPawn->FindComponentByClass<UStudentPerceptorFierensDries>();
	AActor* TargetEnemy = Cast<AActor>(Blackboard->GetValueAsObject(TargetEnemyKey.SelectedKeyName));
	if (Perceptor == nullptr || TargetEnemy == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	return Perceptor->AimAtActor(TargetEnemy)
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}