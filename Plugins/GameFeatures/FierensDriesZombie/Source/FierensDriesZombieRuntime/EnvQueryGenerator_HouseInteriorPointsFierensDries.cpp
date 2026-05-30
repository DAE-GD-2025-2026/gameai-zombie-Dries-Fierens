#include "EnvQueryGenerator_HouseInteriorPointsFierensDries.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"
#include "Village/House/House.h"

namespace
{
	const FName TargetHouseKey(TEXT("TargetHouse"));

	const AHouse* GetTargetHouseFromQueryOwner(const UObject* QueryOwner)
	{
		const AAIController* AIController = Cast<AAIController>(QueryOwner);
		if (AIController == nullptr)
		{
			const APawn* Pawn = Cast<APawn>(QueryOwner);
			if (Pawn != nullptr)
			{
				AIController = Cast<AAIController>(Pawn->GetController());
			}
		}

		if (AIController == nullptr)
		{
			return nullptr;
		}

		const UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
		if (Blackboard == nullptr)
		{
			return nullptr;
		}

		return Cast<AHouse>(Blackboard->GetValueAsObject(TargetHouseKey));
	}
}

UEnvQueryGenerator_HouseInteriorPointsFierensDries::UEnvQueryGenerator_HouseInteriorPointsFierensDries()
{
	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UEnvQueryGenerator_HouseInteriorPointsFierensDries::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	const UObject* QueryOwner = QueryInstance.Owner.Get();
	const AHouse* TargetHouse = GetTargetHouseFromQueryOwner(QueryOwner);
	if (TargetHouse == nullptr)
	{
		return;
	}

	const FHouseBounds Bounds = TargetHouse->GetBounds();

	const float UsableExtentX = FMath::Max(0.0f, Bounds.Extent.X - EdgePadding);
	const float UsableExtentY = FMath::Max(0.0f, Bounds.Extent.Y - EdgePadding);

	TArray<FNavLocation> Points;
	for (float X = -UsableExtentX; X <= UsableExtentX; X += PointSpacing)
	{
		for (float Y = -UsableExtentY; Y <= UsableExtentY; Y += PointSpacing)
		{
			Points.Add(FNavLocation(Bounds.Origin + FVector(X, Y, 0.0f)));
		}
	}

	if (Points.IsEmpty())
	{
		Points.Add(FNavLocation(Bounds.Origin));
	}

	ProjectAndFilterNavPoints(Points, QueryInstance);
	StoreNavPoints(Points, QueryInstance);
}