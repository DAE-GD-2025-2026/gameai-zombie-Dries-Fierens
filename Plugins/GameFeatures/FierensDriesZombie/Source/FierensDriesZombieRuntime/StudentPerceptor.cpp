// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Engine/Engine.h"

namespace
{
	const FName TargetEnemyKey(TEXT("TargetEnemy"));
	const FName LastKnownEnemyLocationKey(TEXT("LastKnownEnemyLocation"));
	const FName HasEnemyInSightKey(TEXT("HasEnemyInSight"));
	const FName TargetItemKey(TEXT("TargetItem"));
	const FName TargetHouseKey(TEXT("TargetHouse"));
	const FName IsInPurgeDangerKey(TEXT("IsInPurgeDanger"));
}

UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
}

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor == nullptr)
	{
		return;
	}

	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (PawnOwner == nullptr)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(PawnOwner->GetController());
	if (AIController == nullptr)
	{
		return;
	}

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (Blackboard == nullptr)
	{
		return;
	}

	const bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();
	
	if (bSuccessfullySensed)
	{
		Blackboard->SetValueAsObject(TEXT("TargetEnemy"), Actor);
		Blackboard->SetValueAsBool(TEXT("HasEnemyInSight"), true);
		Blackboard->SetValueAsVector(TEXT("LastKnownEnemyLocation"), Actor->GetActorLocation());
	}
	
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			5,
			1.0f,
			bSuccessfullySensed ? FColor::Green : FColor::Red,
			FString::Printf(TEXT("Perception updated: %s"), *GetNameSafe(Actor)));
	}
}
