// Fill out your copyright notice in the Description page of Project Settings.

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

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveGarbageItems();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasWeaponInInventory() const;
	
private:
	int32 FindFreeInventorySlot() const;
	bool HasInventorySpace() const;
	int32 FindBestInventorySlot(EItemType ItemType) const;
	bool HasInventoryItemType(EItemType ItemType) const;
	
	UPROPERTY(Transient)
	TSet<TObjectPtr<AHouse>> SearchedHouses;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float LowStaminaThreshold{3.5f};

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float LowHealthThreshold{5.f};
};