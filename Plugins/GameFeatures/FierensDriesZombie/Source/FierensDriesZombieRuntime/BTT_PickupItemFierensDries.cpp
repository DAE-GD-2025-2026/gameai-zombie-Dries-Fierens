#include "BTT_PickupItemFierensDries.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Engine/Engine.h"
#include "Items/BaseItem.h"
#include "StudentPerceptorFierensDries.h"

UBTT_PickupItemFierensDries::UBTT_PickupItemFierensDries()
{
	NodeName = TEXT("Pickup Target Item");
	TargetItemKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_PickupItemFierensDries, TargetItemKey), ABaseItem::StaticClass());
	TargetItemKey.SelectedKeyName = TEXT("TargetItem");
}

void UBTT_PickupItemFierensDries::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetItemKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

EBTNodeResult::Type UBTT_PickupItemFierensDries::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Pickup failed: AIController is null"));
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (ControlledPawn == nullptr || Blackboard == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Pickup failed: Pawn or Blackboard is null"));
		return EBTNodeResult::Failed;
	}

	UStudentPerceptorFierensDries* Perceptor = ControlledPawn->FindComponentByClass<UStudentPerceptorFierensDries>();
	if (Perceptor == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Pickup failed: StudentPerceptor not found"));
		return EBTNodeResult::Failed;
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("Pickup task reading key: %s"), *TargetItemKey.SelectedKeyName.ToString()));

	UObject* TargetItemObject = Blackboard->GetValueAsObject(TargetItemKey.SelectedKeyName);
	
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(
			TEXT("Blackboard value for key '%s': %s"),
			*TargetItemKey.SelectedKeyName.ToString(),
			*GetNameSafe(TargetItemObject)));

	ABaseItem* TargetItem = Cast<ABaseItem>(TargetItemObject);
	if (TargetItem == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Pickup skipped: TargetItem is null"));
		return EBTNodeResult::Failed;
	}

	const bool bPickedUp = Perceptor->TryPickupItem(TargetItem);
	FColor DebugColor = bPickedUp ? FColor::Green : FColor::Red;
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, DebugColor,FString::Printf(
			TEXT("Pickup %s: %s"),
			bPickedUp ? TEXT("succeeded") : TEXT("failed"),
			*TargetItem->GetName()));

	return bPickedUp
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}