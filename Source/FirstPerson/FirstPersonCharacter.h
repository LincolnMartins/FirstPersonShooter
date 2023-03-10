// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Weapon.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FirstPersonCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

USTRUCT()
struct FGun
{
	GENERATED_BODY()

	// Total amount of ammo that can be in the weapon
	//UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	int maxClipAmmo;

	// Total amount of ammo in the weapon
	//UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	int clipAmmo;

	// Time it takes to reload the weapon
	//UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	int reloadTime;

	// Weapon damage
	//UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = Weapon)
	int damage;

	bool active;

	void Initialize(EWeaponType type)
	{
		switch (type)
		{
			case Revolver:
			{
				clipAmmo = 6;
				maxClipAmmo = 6;
				reloadTime = 7;
				damage = 25;
				active = true;
				break;
			}
			case Melee: // Melee/Knife
			{
				clipAmmo = 0;
				maxClipAmmo = 0;
				reloadTime = 0;
				damage = 35;
				active = true;
				break;
			}
		}
	}
};

UCLASS(config=Game)
class AFirstPersonCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Character mesh: Network view (entire body; seen only by others) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* Mesh3P;

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Gun meshes: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* FP_GunMeshes[MAX_WEAPON_TYPE];

	/** Gun meshes: 3rd person view (seen only by others) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* TP_GunMeshes[MAX_WEAPON_TYPE];

	// Player's health
	UPROPERTY(VisibleDefaultsOnly, Category = Gameplay)
	float health = 100;

public:
	AFirstPersonCharacter();

	UFUNCTION()
	void OnOverLapBegin(UPrimitiveComponent* OverLappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIntdex, bool bFromSweep, const FHitResult& SweepResult);

	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	//Weapons that the player is carring
	FGun weapons[MAX_WEAPON_TYPE];

	// Total amount of ammo that can be carried foreach weapon type
	int maxTotalAmmo[MAX_WEAPON_TYPE] = { 0, 200, 100, 300 };

	// Total amount of ammo being carried foreach weapon type
	int totalAmmo[MAX_WEAPON_TYPE] = { 0, 20, 10, 30 };

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	FString TP_MeshName;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	FRotator LookRotation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	TSubclassOf<class AFirstPersonProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	/** Animation state to change each time we crouch */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	bool isCrouched;

	// Weapon index of the player is currently using
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	TEnumAsByte<EWeaponType> EquippedGun = Revolver;

	// Weapon index of the player is cached for switch
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	TEnumAsByte<EWeaponType> CachedGun = Melee;

protected:
	virtual void BeginPlay();

	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	/** Fires a projectile. */
	void OnFire();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnFire(FVector Location, FRotator Rotation);
	bool Server_OnFire_Validate(FVector Location, FRotator Rotation);
	void Server_OnFire_Implementation(FVector Location, FRotator Rotation);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void Multi_OnFire(FVector Location, FRotator Rotation);
	bool Multi_OnFire_Validate(FVector Location, FRotator Rotation);
	void Multi_OnFire_Implementation(FVector Location, FRotator Rotation);

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	void Sprint();
	void Walk();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetMaxWalkSpeed(float speed);
	bool Server_SetMaxWalkSpeed_Validate(float speed);
	void Server_SetMaxWalkSpeed_Implementation(float speed);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_LookUp(FRotator Rotation);
	bool Server_LookUp_Validate(FRotator Rotation);
	void Server_LookUp_Implementation(FRotator Rotation);

	void StartCrouch();
	void StopCrouch();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_isCrouch(bool val);
	bool Server_isCrouch_Validate(bool val);
	void Server_isCrouch_Implementation(bool val);

	// Weapon events
	void OnMelee();
	void OnRevolver();
	void OnShotgun();
	void OnRifle();
	void OnSwitchWeapon();
	void OnReload();
	void OnDropWeapon();
	void OnEquipWeapon(EWeaponType weapontype);

};
