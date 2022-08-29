// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FirstPersonCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class AFirstPersonCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Character mesh: Network view (entire body; seen only by others) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* Mesh0;

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

public:
	AFirstPersonCharacter();

protected:
	virtual void BeginPlay();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	FRotator LookRotation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AFirstPersonProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	bool isCrouched;

protected:
	
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
	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
};
