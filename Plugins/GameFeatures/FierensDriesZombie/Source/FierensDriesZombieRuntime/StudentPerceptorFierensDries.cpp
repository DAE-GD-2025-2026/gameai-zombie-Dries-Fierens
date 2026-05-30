// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptorFierensDries.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Perception/AIPerceptionSystem.h"
#include "Village/House/House.h"
#include "Zombies/BaseZombie.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Engine/Engine.h"

namespace Keys
{
	const FName TargetEnemyKey(TEXT("TargetEnemy"));
	const FName LastKnownEnemyLocationKey(TEXT("LastKnownEnemyLocation"));
	const FName HasEnemyInSightKey(TEXT("HasEnemyInSight"));
	const FName TargetItemKey(TEXT("TargetItem"));
	const FName TargetHouseKey(TEXT("TargetHouse"));
	const FName MoveLocationKey(TEXT("MoveLocation"));
	const FName NeedsFoodKey(TEXT("NeedsFood"));
	const FName NeedsMedkitKey(TEXT("NeedsMedkit"));
	const FName HasWeaponKey(TEXT("HasWeapon"));
	const FName CanFightEnemyKey(TEXT("CanFightEnemy"));
}

namespace
{
	UBlackboardComponent* GetBlackboardFromOwner(AActor* Owner)
	{
		APawn* PawnOwner = Cast<APawn>(Owner);
		if (PawnOwner == nullptr)
		{
			return nullptr;
		}

		AAIController* AIController = Cast<AAIController>(PawnOwner->GetController());
		if (AIController == nullptr)
		{
			return nullptr;
		}

		return AIController->GetBlackboardComponent();
	}

	UInventoryComponent* GetInventoryFromOwner(AActor* Owner)
	{
		return Owner != nullptr
			? Owner->FindComponentByClass<UInventoryComponent>()
			: nullptr;
	}

	void ClearActorKeyIfMatching(UBlackboardComponent* Blackboard, const FName& Key, const AActor* Actor)
	{
		if (Blackboard == nullptr || Actor == nullptr)
		{
			return;
		}

		if (Blackboard->GetValueAsObject(Key) == Actor)
		{
			Blackboard->ClearValue(Key);
		}
	}

	void SetMoveLocationToActor(UBlackboardComponent* Blackboard, const AActor* Actor)
	{
		if (Blackboard == nullptr || Actor == nullptr)
		{
			return;
		}

		Blackboard->SetValueAsVector(Keys::MoveLocationKey, Actor->GetActorLocation());
	}

	bool IsCloserToOwner(const AActor* Owner, const AActor* Candidate, const UObject* CurrentValue)
	{
		const AActor* CurrentActor = Cast<AActor>(CurrentValue);
		if (Owner == nullptr || Candidate == nullptr)
		{
			return false;
		}

		if (CurrentActor == nullptr)
		{
			return true;
		}

		const FVector OwnerLocation = Owner->GetActorLocation();
		const float CandidateDistanceSq = FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation());
		const float CurrentDistanceSq = FVector::DistSquared(OwnerLocation, CurrentActor->GetActorLocation());
		return CandidateDistanceSq < CurrentDistanceSq;
	}

	bool IsDamageStimulus(UObject* WorldContextObject, const FAIStimulus& Stimulus)
	{
		const TSubclassOf<UAISense> SenseClass =
			UAIPerceptionSystem::GetSenseClassForStimulus(WorldContextObject, Stimulus);

		return SenseClass == UAISense_Damage::StaticClass();
	}
	
	UHealthComponent* GetHealthFromOwner(AActor* Owner)
	{
		return Owner != nullptr
			? Owner->FindComponentByClass<UHealthComponent>()
			: nullptr;
	}

	UStaminaComponent* GetStaminaFromOwner(AActor* Owner)
	{
		return Owner != nullptr
			? Owner->FindComponentByClass<UStaminaComponent>()
			: nullptr;
	}

	bool IsWeaponItemType(const EItemType ItemType)
	{
		return ItemType == EItemType::Pistol || ItemType == EItemType::Shotgun;
	}
}

UStudentPerceptorFierensDries::UStudentPerceptorFierensDries()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudentPerceptorFierensDries::BeginPlay()
{
	Super::BeginPlay();

	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptorFierensDries::OnPerceptionUpdated);
	}
}

int32 UStudentPerceptorFierensDries::FindFreeInventorySlot() const
{
	const UInventoryComponent* InventoryComponent =
		GetOwner() != nullptr
			? GetOwner()->FindComponentByClass<UInventoryComponent>()
			: nullptr;

	if (InventoryComponent == nullptr)
	{
		return INDEX_NONE;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	for (int32 Index = 0; Index < Inventory.Num(); ++Index)
	{
		if (Inventory[Index] == nullptr)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool UStudentPerceptorFierensDries::HasInventorySpace() const
{
	return FindFreeInventorySlot() != INDEX_NONE;
}

bool UStudentPerceptorFierensDries::TryPickupItem(ABaseItem* Item)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr || Item == nullptr)
	{
		return false;
	}

	UInventoryComponent* InventoryComponent = GetInventoryFromOwner(OwnerActor);
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	const float PickupRangeSq = FMath::Square(InventoryComponent->GetPickupRange());
	if (FVector::DistSquared(OwnerActor->GetActorLocation(), Item->GetActorLocation()) > PickupRangeSq)
	{
		return false;
	}

	int32 FreeSlot = FindFreeInventorySlot();
	if (FreeSlot == INDEX_NONE)
	{
		RemoveGarbageItems();
		FreeSlot = FindFreeInventorySlot();
		if (FreeSlot == INDEX_NONE)
		{
			return false;
		}
	}

	if (!InventoryComponent->GrabItem(FreeSlot, Item))
	{
		return false;
	}

	if (UBlackboardComponent* Blackboard = GetBlackboardFromOwner(OwnerActor))
	{
		ClearActorKeyIfMatching(Blackboard, Keys::TargetItemKey, Item);
	}

	UpdateInventoryBlackboard();
	return true;
}

void UStudentPerceptorFierensDries::MarkHouseAsSearched(AHouse* House)
{
	if (House == nullptr)
	{
		return;
	}

	
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Searching: %s"), *House->GetName()));
	SearchedHouses.Add(House);

	if (UBlackboardComponent* Blackboard = GetBlackboardFromOwner(GetOwner()))
	{
		ClearActorKeyIfMatching(Blackboard, Keys::TargetHouseKey, House);
	}
}

bool UStudentPerceptorFierensDries::IsHouseSearched(const AHouse* House) const
{
	bool Result = House != nullptr && SearchedHouses.Contains(House);
	if (Result) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("%s already searched"), *House->GetName()));
	return Result;
}

int32 UStudentPerceptorFierensDries::FindBestInventorySlot(EItemType ItemType) const
{
	const UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return INDEX_NONE;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	int32 BestSlot = INDEX_NONE;
	int32 BestValue = TNumericLimits<int32>::Max();

	for (int32 Index = 0; Index < Inventory.Num(); ++Index)
	{
		const ABaseItem* Item = Inventory[Index];
		if (Item == nullptr || Item->GetItemType() != ItemType || Item->GetValue() <= 0)
		{
			continue;
		}

		if (Item->GetValue() < BestValue)
		{
			BestValue = Item->GetValue();
			BestSlot = Index;
		}
	}

	return BestSlot;
}

bool UStudentPerceptorFierensDries::HasInventoryItemType(EItemType ItemType) const
{
	return FindBestInventorySlot(ItemType) != INDEX_NONE;
}

bool UStudentPerceptorFierensDries::HasWeaponInInventory() const
{
	const UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	for (const ABaseItem* Item : Inventory)
	{
		if (Item != nullptr && Item->GetValue() > 0 && IsWeaponItemType(Item->GetItemType()))
		{
			return true;
		}
	}

	return false;
}

int32 UStudentPerceptorFierensDries::RemoveGarbageItems()
{
	UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return 0;
	}

	int32 RemovedCount = 0;
	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();

	for (int32 Index = Inventory.Num() - 1; Index >= 0; --Index)
	{
		ABaseItem* Item = Inventory[Index];
		if (Item == nullptr)
		{
			continue;
		}

		if (Item->GetItemType() == EItemType::Garbage || Item->GetValue() <= 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, FString::Printf(TEXT("Removing garbage item from slot %d: %s"), Index, *Item->GetName()));

			if (InventoryComponent->RemoveItem(Index))
			{
				++RemovedCount;
			}
		}
	}

	return RemovedCount;
}

void UStudentPerceptorFierensDries::UpdateInventoryBlackboard()
{
	UBlackboardComponent* Blackboard = GetBlackboardFromOwner(GetOwner());
	const UHealthComponent* HealthComponent = GetHealthFromOwner(GetOwner());
	const UStaminaComponent* StaminaComponent = GetStaminaFromOwner(GetOwner());
	
	if (Blackboard == nullptr || HealthComponent == nullptr || StaminaComponent == nullptr)
	{
		return;
	}
	
	RemoveGarbageItems();

	const bool bHasFood = HasInventoryItemType(EItemType::Food);
	const bool bHasMedkit = HasInventoryItemType(EItemType::Medkit);
	const bool bHasWeapon = HasWeaponInInventory();

	const bool bNeedsFood = bHasFood && StaminaComponent->GetCurrentStamina() <= LowStaminaThreshold;
	const bool bNeedsMedkit = bHasMedkit && HealthComponent->GetHealth() <= LowHealthThreshold;
	const bool bCanFightEnemy = bHasWeapon && !bNeedsMedkit;

	Blackboard->SetValueAsBool(Keys::NeedsFoodKey, bNeedsFood);
	Blackboard->SetValueAsBool(Keys::NeedsMedkitKey, bNeedsMedkit);
	Blackboard->SetValueAsBool(Keys::HasWeaponKey, bHasWeapon);
	Blackboard->SetValueAsBool(Keys::CanFightEnemyKey, bCanFightEnemy);
}

bool UStudentPerceptorFierensDries::UseBestInventoryItem(EItemType ItemType)
{
	UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("UseBestInventoryItem failed: InventoryComponent not found"));
		return false;
	}

	const int32 SlotIndex = FindBestInventorySlot(ItemType);
	if (SlotIndex == INDEX_NONE)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("UseBestInventoryItem failed: no matching item found"));
		UpdateInventoryBlackboard();
		return false;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	ABaseItem* Item = Inventory[SlotIndex];
	if (Item == nullptr)
	{	
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("UseBestInventoryItem failed: slot item is null"));
		UpdateInventoryBlackboard();
		return false;
	}

	if (!InventoryComponent->UseItem(SlotIndex))
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("UseBestInventoryItem failed: could not use %s from slot %d"), *Item->GetName(), SlotIndex));
		UpdateInventoryBlackboard();
		return false;
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Used %s from slot %d"), *Item->GetName(), SlotIndex));

	const TArray<ABaseItem*>& UpdatedInventory = InventoryComponent->GetInventory();
	ABaseItem* UpdatedItem = UpdatedInventory.IsValidIndex(SlotIndex) ? UpdatedInventory[SlotIndex] : nullptr;
	if (UpdatedItem != nullptr && (UpdatedItem->GetItemType() == EItemType::Garbage || UpdatedItem->GetValue() <= 0))
	{
		InventoryComponent->RemoveItem(SlotIndex);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, FString::Printf(TEXT("Removed used-up item from slot %d"), SlotIndex));
	}

	UpdateInventoryBlackboard();
	return true;
}

void UStudentPerceptorFierensDries::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor == nullptr)
	{
		return;
	}

	UBlackboardComponent* Blackboard = GetBlackboardFromOwner(GetOwner());
	if (Blackboard == nullptr)
	{
		return;
	}

	const bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();
	const bool bIsDamageSense = IsDamageStimulus(this, Stimulus);
	const FVector SensedLocation =
		Stimulus.StimulusLocation.IsNearlyZero()
			? Actor->GetActorLocation()
			: Stimulus.StimulusLocation;

	if (Actor->IsA(ABaseZombie::StaticClass()))
	{
		const bool bEnemyThreatDetected = bSuccessfullySensed || bIsDamageSense;

		if (bEnemyThreatDetected)
		{
			Blackboard->SetValueAsObject(Keys::TargetEnemyKey, Actor);
			Blackboard->SetValueAsBool(Keys::HasEnemyInSightKey, true);
			Blackboard->SetValueAsVector(Keys::LastKnownEnemyLocationKey, SensedLocation);
		}
		else if (Blackboard->GetValueAsObject(Keys::TargetEnemyKey) == Actor)
		{
			Blackboard->ClearValue(Keys::TargetEnemyKey);
			Blackboard->SetValueAsBool(Keys::HasEnemyInSightKey, false);
			Blackboard->SetValueAsVector(Keys::LastKnownEnemyLocationKey, SensedLocation);
		}

		return;
	}

	if (Actor->IsA(ABaseItem::StaticClass()))
	{
		if (!HasInventorySpace())
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Perception: inventory full, clearing TargetItem"));
			ClearActorKeyIfMatching(Blackboard, Keys::TargetItemKey, Actor);
			return;
		}

		if (bSuccessfullySensed)
		{
			if (IsCloserToOwner(GetOwner(), Actor, Blackboard->GetValueAsObject(Keys::TargetItemKey)))
			{
				Blackboard->SetValueAsObject(Keys::TargetItemKey, Actor);
				SetMoveLocationToActor(Blackboard, Actor);
				
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("Perception: set TargetItem to %s"), *Actor->GetName()));
			}
		}
		else if (Blackboard->GetValueAsObject(Keys::TargetItemKey) == Actor)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Perception: lost sight of %s, keeping TargetItem"), *Actor->GetName()));
		}

		return;
	}

	AHouse* House = Cast<AHouse>(Actor);
	if (House == nullptr)
	{
		return;
	}

	if (IsHouseSearched(House))
	{
		ClearActorKeyIfMatching(Blackboard, Keys::TargetHouseKey, House);
		return;
	}

	if (bSuccessfullySensed &&
		IsCloserToOwner(GetOwner(), House, Blackboard->GetValueAsObject(Keys::TargetHouseKey)))
	{
		Blackboard->SetValueAsObject(Keys::TargetHouseKey, House);
		SetMoveLocationToActor(Blackboard, House);
	}
	else if (!bSuccessfullySensed)
	{
		ClearActorKeyIfMatching(Blackboard, Keys::TargetHouseKey, House);
	}
}