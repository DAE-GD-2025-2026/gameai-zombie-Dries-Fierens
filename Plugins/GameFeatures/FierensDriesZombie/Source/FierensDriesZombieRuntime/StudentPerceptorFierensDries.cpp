// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptorFierensDries.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Common/HealthComponent.h"
#include "Common/InventoryComponent.h"
#include "Common/StaminaComponent.h"
#include "Engine/Engine.h"
#include "Items/BaseItem.h"
#include "Perception/AIPerceptionSystem.h"
#include "Village/House/House.h"
#include "Zombies/BaseZombie.h"

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

#pragma region HelperFunctions
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

	void ClearActorKey(UBlackboardComponent* Blackboard, const FName& Key, const AActor* Actor)
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

	bool IsConsumableItemType(const EItemType ItemType)
	{
		return ItemType == EItemType::Food || ItemType == EItemType::Medkit;
	}
}
#pragma endregion

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

	UpdateInventoryBlackboard();
}

#pragma region Loot

bool UStudentPerceptorFierensDries::TryPickupItem(ABaseItem* Item)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr || Item == nullptr || !IsValid(Item) || Item->IsActorBeingDestroyed() || Item->IsHidden())
	{
		return false;
	}

	UInventoryComponent* InventoryComponent = GetInventoryFromOwner(OwnerActor);
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	if (!ShouldTargetItem(Item))
	{
		ForgetKnownItem(Item);
		RefreshTargetItem();
		return false;
	}

	const float PickupRangeSq = FMath::Square(InventoryComponent->GetPickupRange());
	if (FVector::DistSquared(OwnerActor->GetActorLocation(), Item->GetActorLocation()) > PickupRangeSq)
	{
		return false;
	}

	CleanInventory();

	const EItemType ItemType = Item->GetItemType();
	int32 ExistingSlot = FindBestInventorySlot(ItemType);
	if (ExistingSlot != INDEX_NONE)
	{
		const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
		ABaseItem* ExistingItem = Inventory.IsValidIndex(ExistingSlot) ? Inventory[ExistingSlot] : nullptr;
		if (ExistingItem != nullptr)
		{
			if (Item->GetValue() <= ExistingItem->GetValue())
			{
				ForgetKnownItem(Item);
				RefreshTargetItem();
				return false;
			}

			if (IsConsumableItemType(ItemType) && CanUseConsumableNow(ItemType))
			{
				UseBestInventoryItem(ItemType);
				CleanInventory();
				ExistingSlot = FindBestInventorySlot(ItemType);
			}

			if (ExistingSlot != INDEX_NONE)
			{
				InventoryComponent->RemoveItem(ExistingSlot);
			}
		}
	}

	CleanInventory();

	int32 FreeSlot = FindFreeInventorySlot();
	if (FreeSlot == INDEX_NONE)
	{
		RefreshTargetItem();
		return false;
	}

	if (!InventoryComponent->GrabItem(FreeSlot, Item))
	{
		RefreshTargetItem();
		return false;
	}

	ForgetKnownItem(Item);

	if (UBlackboardComponent* Blackboard = GetBlackboardFromOwner(OwnerActor))
	{
		ClearActorKey(Blackboard, Keys::TargetItemKey, Item);
	}

	UpdateInventoryBlackboard();
	RefreshTargetItem();
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
	ForgetKnownHouse(House);

	if (UBlackboardComponent* Blackboard = GetBlackboardFromOwner(GetOwner()))
	{
		ClearActorKey(Blackboard, Keys::TargetHouseKey, House);
	}

	RefreshTargetHouse();
}

bool UStudentPerceptorFierensDries::IsHouseSearched(const AHouse* House) const
{
	return House != nullptr && SearchedHouses.Contains(House);
}

bool UStudentPerceptorFierensDries::IsHigherPriorityHouse(const AHouse* Candidate, const AHouse* CurrentBest) const
{
	if (Candidate == nullptr)
	{
		return false;
	}

	if (CurrentBest == nullptr || GetOwner() == nullptr)
	{
		return true;
	}

	const float CandidateDistanceSq =
		FVector::DistSquared(GetOwner()->GetActorLocation(), Candidate->GetActorLocation());
	const float CurrentDistanceSq =
		FVector::DistSquared(GetOwner()->GetActorLocation(), CurrentBest->GetActorLocation());

	return CandidateDistanceSq < CurrentDistanceSq;
}

void UStudentPerceptorFierensDries::CleanupKnownHouses()
{
	for (auto It = KnownHouses.CreateIterator(); It; ++It)
	{
		AHouse* House = It->Get();
		if (IsHouseSearched(House))
		{
			It.RemoveCurrent();
		}
	}
}

void UStudentPerceptorFierensDries::ForgetKnownHouse(const AHouse* House)
{
	if (House == nullptr)
	{
		return;
	}

	KnownHouses.Remove(House);
}

void UStudentPerceptorFierensDries::RefreshTargetHouse()
{
	UBlackboardComponent* Blackboard = GetBlackboardFromOwner(GetOwner());
	if (Blackboard == nullptr)
	{
		return;
	}

	CleanupKnownHouses();

	AHouse* BestHouse = nullptr;

	for (const TObjectPtr<AHouse>& HousePtr : KnownHouses)
	{
		AHouse* House = HousePtr.Get();
		if (IsHouseSearched(House))
		{
			continue;
		}

		if (IsHigherPriorityHouse(House, BestHouse))
		{
			BestHouse = House;
		}
	}

	if (BestHouse != nullptr)
	{
		Blackboard->SetValueAsObject(Keys::TargetHouseKey, BestHouse);

		if (Blackboard->GetValueAsObject(Keys::TargetItemKey) == nullptr)
		{
			SetMoveLocationToActor(Blackboard, BestHouse);
		}
	}
	else
	{
		Blackboard->ClearValue(Keys::TargetHouseKey);
	}
}

#pragma endregion

#pragma region Inventory

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

// Make sure that there is at least one of each item type
int32 UStudentPerceptorFierensDries::FindBestInventorySlot(EItemType ItemType) const
{
	const UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return INDEX_NONE;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	int32 BestSlot = INDEX_NONE;
	int32 BestValue = TNumericLimits<int32>::Lowest();

	for (int32 Index = 0; Index < Inventory.Num(); ++Index)
	{
		const ABaseItem* Item = Inventory[Index];
		if (Item == nullptr || Item->GetItemType() != ItemType || Item->GetValue() <= 0)
		{
			continue;
		}

		if (Item->GetValue() > BestValue)
		{
			BestValue = Item->GetValue();
			BestSlot = Index;
		}
	}

	return BestSlot;
}

int32 UStudentPerceptorFierensDries::FindLowestValueInventorySlot(EItemType ItemType) const
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
	return HasInventoryItemType(EItemType::Shotgun) || HasInventoryItemType(EItemType::Pistol);
}

bool UStudentPerceptorFierensDries::CanUseConsumableNow(EItemType ItemType) const
{
	switch (ItemType)
	{
	case EItemType::Food:
	{
		const UStaminaComponent* StaminaComponent = GetStaminaFromOwner(GetOwner());
		return StaminaComponent != nullptr &&
			StaminaComponent->GetCurrentStamina() < StaminaComponent->GetMaxStamina();
	}
	case EItemType::Medkit:
	{
		const UHealthComponent* HealthComponent = GetHealthFromOwner(GetOwner());
		return HealthComponent != nullptr &&
			HealthComponent->GetHealth() < HealthComponent->GetMaxHealth();
	}
	default:
		return false;
	}
}

void UStudentPerceptorFierensDries::RemoveGarbageItems()
{
	UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return;
	}
	
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

			InventoryComponent->RemoveItem(Index);
		}
	}
}

void UStudentPerceptorFierensDries::ReplaceItems()
{
	UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	TMap<EItemType, int32> BestSlotsByType;
	TMap<EItemType, int32> BestValuesByType;

	for (int32 Index = 0; Index < Inventory.Num(); ++Index)
	{
		const ABaseItem* Item = Inventory[Index];
		if (Item == nullptr || Item->GetValue() <= 0 || Item->GetItemType() == EItemType::Garbage)
		{
			continue;
		}

		const EItemType ItemType = Item->GetItemType();
		const int32 ItemValue = Item->GetValue();

		const int32* CurrentBestValue = BestValuesByType.Find(ItemType);
		if (CurrentBestValue == nullptr || ItemValue > *CurrentBestValue)
		{
			BestValuesByType.Add(ItemType, ItemValue);
			BestSlotsByType.Add(ItemType, Index);
		}
	}

	for (int32 Index = Inventory.Num() - 1; Index >= 0; --Index)
	{
		ABaseItem* Item = Inventory[Index];
		if (Item == nullptr || Item->GetValue() <= 0 || Item->GetItemType() == EItemType::Garbage)
		{
			continue;
		}

		const int32* BestSlot = BestSlotsByType.Find(Item->GetItemType());
		if (BestSlot != nullptr && *BestSlot != Index)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, FString::Printf(TEXT("Removing duplicate item from slot %d: %s"), Index, *Item->GetName()));
			InventoryComponent->RemoveItem(Index);
		}
	}
}

void UStudentPerceptorFierensDries::CleanInventory()
{
	RemoveGarbageItems();
	ReplaceItems();
}

bool UStudentPerceptorFierensDries::ShouldTargetItem(const ABaseItem* Item) const
{
	if (Item == nullptr || !IsValid(Item) || Item->IsActorBeingDestroyed() || Item->IsHidden())
	{
		return false;
	}

	if (Item->GetItemType() == EItemType::Garbage || Item->GetValue() <= 0)
	{
		return false;
	}

	const int32 BestOwnedSlot = FindBestInventorySlot(Item->GetItemType());
	if (BestOwnedSlot == INDEX_NONE)
	{
		return true;
	}

	const UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	const ABaseItem* OwnedItem = Inventory.IsValidIndex(BestOwnedSlot) ? Inventory[BestOwnedSlot] : nullptr;
	if (OwnedItem == nullptr)
	{
		return true;
	}

	return Item->GetValue() > OwnedItem->GetValue();
}

int32 UStudentPerceptorFierensDries::GetItemPriority(const ABaseItem* Item) const
{
	if (!ShouldTargetItem(Item))
	{
		return -1;
	}

	const UHealthComponent* HealthComponent = GetHealthFromOwner(GetOwner());
	const UStaminaComponent* StaminaComponent = GetStaminaFromOwner(GetOwner());
	
	const bool bLowHealth =
		HealthComponent != nullptr &&
		HealthComponent->GetHealth() <= LowHealthThreshold;
	const bool bLowStamina =
		StaminaComponent != nullptr &&
		StaminaComponent->GetCurrentStamina() <= LowStaminaThreshold;

	switch (Item->GetItemType())
	{
	case EItemType::Medkit:
		return bLowHealth ? 5 : 2;

	case EItemType::Shotgun:
	case EItemType::Pistol:
		return HasWeaponInInventory() ? 3 : 4;

	case EItemType::Food:
		return bLowStamina ? 3 : 1;

	default:
		return 0;
	}
}

int32 UStudentPerceptorFierensDries::GetItemValue(const ABaseItem* Item) const
{
	if (Item == nullptr)
	{
		return 0;
	}

	const int32 BestOwnedSlot = FindBestInventorySlot(Item->GetItemType());
	if (BestOwnedSlot == INDEX_NONE)
	{
		return Item->GetValue();
	}

	const UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return Item->GetValue();
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	const ABaseItem* OwnedItem = Inventory.IsValidIndex(BestOwnedSlot) ? Inventory[BestOwnedSlot] : nullptr;
	if (OwnedItem == nullptr)
	{
		return Item->GetValue();
	}

	return Item->GetValue() - OwnedItem->GetValue();
}

// Priority order: Item type > Item Value > Distance
bool UStudentPerceptorFierensDries::IsHigherPriorityItem(const ABaseItem* Candidate, const ABaseItem* CurrentBest) const
{
	if (Candidate == nullptr)
	{
		return false;
	}

	if (CurrentBest == nullptr)
	{
		return true;
	}

	const int32 CandidatePriority = GetItemPriority(Candidate);
	const int32 CurrentPriority = GetItemPriority(CurrentBest);
	if (CandidatePriority != CurrentPriority)
	{
		return CandidatePriority > CurrentPriority;
	}

	const int32 CandidateValue = GetItemValue(Candidate);
	const int32 CurrentValue = GetItemValue(CurrentBest);
	if (CandidateValue != CurrentValue)
	{
		return CandidateValue > CurrentValue;
	}

	if (GetOwner() == nullptr)
	{
		return false;
	}

	const float CandidateDistanceSq = FVector::DistSquared(GetOwner()->GetActorLocation(), Candidate->GetActorLocation());
	const float CurrentDistanceSq = FVector::DistSquared(GetOwner()->GetActorLocation(), CurrentBest->GetActorLocation());
	return CandidateDistanceSq < CurrentDistanceSq;
}

void UStudentPerceptorFierensDries::CleanupKnownItems()
{
	for (auto It = KnownItems.CreateIterator(); It; ++It)
	{
		ABaseItem* Item = It->Get();
		if (!IsValid(Item) || Item->IsActorBeingDestroyed() || Item->IsHidden())
		{
			It.RemoveCurrent();
		}
	}
}

void UStudentPerceptorFierensDries::ForgetKnownItem(const ABaseItem* Item)
{
	if (Item == nullptr)
	{
		return;
	}

	KnownItems.Remove(Item);
}

void UStudentPerceptorFierensDries::RefreshTargetItem()
{
	UBlackboardComponent* Blackboard = GetBlackboardFromOwner(GetOwner());
	if (Blackboard == nullptr)
	{
		return;
	}

	CleanupKnownItems();

	ABaseItem* BestItem = nullptr;

	for (const TObjectPtr<ABaseItem>& ItemPtr : KnownItems)
	{
		ABaseItem* Item = ItemPtr.Get();
		if (!ShouldTargetItem(Item))
		{
			continue;
		}

		if (IsHigherPriorityItem(Item, BestItem))
		{
			BestItem = Item;
		}
	}

	if (BestItem != nullptr)
	{
		Blackboard->SetValueAsObject(Keys::TargetItemKey, BestItem);
		SetMoveLocationToActor(Blackboard, BestItem);
	}
	else
	{
		Blackboard->ClearValue(Keys::TargetItemKey);
		RefreshTargetHouse();
	}
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
	
	CleanInventory();

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

	RefreshTargetItem();
}

bool UStudentPerceptorFierensDries::UseBestInventoryItem(EItemType ItemType)
{
	UInventoryComponent* InventoryComponent = GetInventoryFromOwner(GetOwner());
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	const int32 SlotIndex = FindLowestValueInventorySlot(ItemType);
	if (SlotIndex == INDEX_NONE)
	{
		UpdateInventoryBlackboard();
		return false;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	ABaseItem* Item = Inventory[SlotIndex];
	if (Item == nullptr)
	{	
		UpdateInventoryBlackboard();
		return false;
	}

	if (!InventoryComponent->UseItem(SlotIndex))
	{
		UpdateInventoryBlackboard();
		return false;
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Used %s from slot %d"), *Item->GetName(), SlotIndex));

	const TArray<ABaseItem*>& UpdatedInventory = InventoryComponent->GetInventory();
	ABaseItem* UpdatedItem = UpdatedInventory.IsValidIndex(SlotIndex) ? UpdatedInventory[SlotIndex] : nullptr;
	if (UpdatedItem != nullptr && (UpdatedItem->GetItemType() == EItemType::Garbage || UpdatedItem->GetValue() <= 0))
	{
		InventoryComponent->RemoveItem(SlotIndex);
	}

	UpdateInventoryBlackboard();
	return true;
} 

#pragma endregion

#pragma region Combat

int32 UStudentPerceptorFierensDries::FindWeaponSlotForTarget(float TargetDistance) const
{
	if (TargetDistance <= PreferredShotgunRange)
	{
		const int32 ShotgunSlot = FindBestInventorySlot(EItemType::Shotgun);
		if (ShotgunSlot != INDEX_NONE)
		{
			return ShotgunSlot;
		}
	}

	const int32 PistolSlot = FindBestInventorySlot(EItemType::Pistol);
	if (PistolSlot != INDEX_NONE)
	{
		return PistolSlot;
	}

	return FindBestInventorySlot(EItemType::Shotgun);
}

bool UStudentPerceptorFierensDries::AimAtActor(const AActor* TargetActor)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr || TargetActor == nullptr)
	{
		return false;
	}

	FVector Direction = TargetActor->GetActorLocation() - OwnerActor->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	OwnerActor->SetActorRotation(Direction.Rotation());
	return true;
}

bool UStudentPerceptorFierensDries::ShootBestWeaponAtActor(const AActor* TargetActor)
{
	AActor* OwnerActor = GetOwner();
	UInventoryComponent* InventoryComponent = GetInventoryFromOwner(OwnerActor);
	if (OwnerActor == nullptr || TargetActor == nullptr || InventoryComponent == nullptr)
	{
		return false;
	}

	AimAtActor(TargetActor);

	const float TargetDistance = FVector::Dist(OwnerActor->GetActorLocation(), TargetActor->GetActorLocation());
	const int32 SlotIndex = FindWeaponSlotForTarget(TargetDistance);
	if (SlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (!InventoryComponent->UseItem(SlotIndex))
	{
		return false;
	}

	const TArray<ABaseItem*>& Inventory = InventoryComponent->GetInventory();
	ABaseItem* UsedItem = Inventory.IsValidIndex(SlotIndex) ? Inventory[SlotIndex] : nullptr;
	if (UsedItem != nullptr && (UsedItem->GetItemType() == EItemType::Garbage || UsedItem->GetValue() <= 0))
	{
		InventoryComponent->RemoveItem(SlotIndex);
	}

	UpdateInventoryBlackboard();
	return true;
}

#pragma endregion

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

	if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		if (bSuccessfullySensed)
		{
			KnownItems.Add(Item);
		}

		RefreshTargetItem();
		return;
	}

	AHouse* House = Cast<AHouse>(Actor);
	if (House == nullptr)
	{
		return;
	}

	if (bSuccessfullySensed)
	{
		KnownHouses.Add(House);
	}

	RefreshTargetHouse();

	if (IsHouseSearched(House))
	{
		ClearActorKey(Blackboard, Keys::TargetHouseKey, House);
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
		ClearActorKey(Blackboard, Keys::TargetHouseKey, House);
	}
}