#include "BTT_ShootTargetEnemyFierensDries.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "StudentPerceptorFierensDries.h"

UBTT_ShootTargetEnemyFierensDries::UBTT_ShootTargetEnemyFierensDries()
{
	NodeName = TEXT("Shoot Target Enemy");
	TargetEnemyKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_ShootTargetEnemyFierensDries, TargetEnemyKey), AActor::StaticClass());
	TargetEnemyKey.SelectedKeyName = TEXT("TargetEnemy");
}

EBTNodeResult::Type UBTT_ShootTargetEnemyFierensDries::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	return Perceptor->ShootBestWeaponAtActor(TargetEnemy)
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}