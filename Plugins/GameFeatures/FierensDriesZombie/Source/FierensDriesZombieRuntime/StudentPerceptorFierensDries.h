#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Items/ItemType.h"
#include "StudentPerceptorFierensDries.generated.h"

class ABaseItem;
class AHouse;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FIERENSDRIESZOMBIERUNTIME_API UStudentPerceptorFierensDries : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptorFierensDries();

	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION(BlueprintCallable, Category = "Loot")
	bool TryPickupItem(ABaseItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Loot")
	void MarkHouseAsSearched(AHouse* House);

	UFUNCTION(BlueprintPure, Category = "Loot")
	bool IsHouseSearched(const AHouse* House) const;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateInventoryBlackboard();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseBestInventoryItem(EItemType ItemType);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasWeaponInInventory() const;
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool AimAtActor(const AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ShootBestWeaponAtActor(const AActor* TargetActor);
	
private:
	int32 FindFreeInventorySlot() const;
	bool HasInventorySpace() const;
	int32 FindBestInventorySlot(EItemType ItemType) const;
	int32 FindLowestValueInventorySlot(EItemType ItemType) const;
	bool HasInventoryItemType(EItemType ItemType) const;
	int32 FindWeaponSlotForTarget(float TargetDistance) const;

	bool CanUseConsumableNow(EItemType ItemType) const;
	bool ShouldTargetItem(const ABaseItem* Item) const;
	int32 GetItemPriority(const ABaseItem* Item) const;
	int32 GetItemValue(const ABaseItem* Item) const;
	bool IsHigherPriorityItem(const ABaseItem* Candidate, const ABaseItem* CurrentBest) const;
	bool IsHigherPriorityHouse(const AHouse* Candidate, const AHouse* CurrentBest) const;

	void RefreshTargetItem();
	void CleanupKnownItems();
	void ForgetKnownItem(const ABaseItem* Item);

	void RefreshTargetHouse();
	void CleanupKnownHouses();
	void ForgetKnownHouse(const AHouse* House);

	void RemoveGarbageItems();
	void ReplaceItems();
	void CleanInventory();
	
	UPROPERTY(Transient)
	TSet<TObjectPtr<AHouse>> SearchedHouses;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AHouse>> KnownHouses;

	UPROPERTY(Transient)
	TSet<TObjectPtr<ABaseItem>> KnownItems;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float LowStaminaThreshold{3.5f};

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float LowHealthThreshold{5.f};
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float PreferredShotgunRange{20.f};
};