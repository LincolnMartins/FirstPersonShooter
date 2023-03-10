// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

// Weapon Types
UENUM()
enum EWeaponType
{
	Melee,
	Revolver,
	Shotgun,
	Rifle,
	MAX_WEAPON_TYPE
};

UCLASS()
class FIRSTPERSON_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();

	/// Gun Mesh
	UPROPERTY(VisibleAnyWhere, Category = Mesh)
	class UStaticMeshComponent* mesh;

	//// Type of the weapon
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	TEnumAsByte<EWeaponType> weaponType;

	// Total amount of ammo that can be in the weapon
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	int maxClipAmmo;

	// Total amount of ammo in the weapon
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	int clipAmmo;

	// Time it takes to reload the weapon
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	int reloadTime;

	// Weapon damage
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	int damage;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void OnWeaponPickup();

	UFUNCTION()
	void OnWeaponSpawn();

};
