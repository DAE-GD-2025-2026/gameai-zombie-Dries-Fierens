#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_HouseInteriorPointsFierensDries.generated.h"

UCLASS(meta = (DisplayName = "House Interior Points"))
class FIERENSDRIESZOMBIERUNTIME_API UEnvQueryGenerator_HouseInteriorPointsFierensDries : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_HouseInteriorPointsFierensDries();

protected:
	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "50.0"))
	float PointSpacing{100.0f};

	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "0.0"))
	float EdgePadding{75.0f};
};