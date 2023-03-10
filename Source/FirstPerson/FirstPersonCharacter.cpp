// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonCharacter.h"
#include "FirstPersonProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AFirstPersonCharacter

AFirstPersonCharacter::AFirstPersonCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AFirstPersonCharacter::OnOverLapBegin);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	
	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);
}

void AFirstPersonCharacter::BeginPlay()
{
	// Call the base class
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor	
	/*
	if (IsLocallyControlled())
	{
		FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
		FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
	}
	else
	{
		FP_Gun->SetupAttachment(Mesh3P, TEXT("GripPoint"));
		FP_Gun->AttachToComponent(Mesh3P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
	}
	*/

	// Get the mesh component that will be used when being viewed from a networked view (when not controlling this pawn)
	TArray<USkeletalMeshComponent*> comps;
	GetComponents(comps);
	for (auto SkeletalMeshComponent : comps)
	{
		if (SkeletalMeshComponent->GetName() == TP_MeshName)
		{
			Mesh3P = SkeletalMeshComponent;
			break;
		}
	}

	TArray<UStaticMeshComponent*> guns;
	GetComponents(guns);
	for (auto StaticMeshComponent : guns)
	{
		if (StaticMeshComponent->GetName() == TEXT("FP_Revolver"))
		{
			FP_GunMeshes[Revolver] = StaticMeshComponent;
		}
		else if (StaticMeshComponent->GetName() == TEXT("FP_Shotgun"))
		{
			FP_GunMeshes[Shotgun] = StaticMeshComponent;
		}
		else if (StaticMeshComponent->GetName() == TEXT("FP_Rifle"))
		{
			FP_GunMeshes[Rifle] = StaticMeshComponent;
		}
		else if (StaticMeshComponent->GetName() == TEXT("TP_Revolver"))
		{
			TP_GunMeshes[Revolver] = StaticMeshComponent;
		}
		else if (StaticMeshComponent->GetName() == TEXT("TP_Shotgun"))
		{
			TP_GunMeshes[Shotgun] = StaticMeshComponent;
		}
		else if (StaticMeshComponent->GetName() == TEXT("TP_Rifle"))
		{
			TP_GunMeshes[Rifle] = StaticMeshComponent;
		}
	}

	// Initializing guns
	weapons[Melee].Initialize(Melee);
	weapons[Revolver].Initialize(Revolver);
	OnEquipWeapon(Revolver);
}

void AFirstPersonCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFirstPersonCharacter, LookRotation);
	DOREPLIFETIME(AFirstPersonCharacter, isCrouched);
}

void AFirstPersonCharacter::OnOverLapBegin(UPrimitiveComponent* OverLappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIntdex, bool bFromSweep, const FHitResult& SweepResult)
{
	//Pickup Gun
	if (OtherActor->IsA(AWeapon::StaticClass()))
	{
		AWeapon* gun = Cast<AWeapon>(OtherActor);
		gun->OnWeaponPickup();

		if (IsLocallyControlled())
		{
			EWeaponType type = gun->weaponType;
			if (weapons[type].active == false)
			{
				weapons[type].clipAmmo = gun->clipAmmo;
				weapons[type].maxClipAmmo = gun->maxClipAmmo;
				weapons[type].reloadTime = gun->reloadTime;
				weapons[type].damage = gun->damage;
				weapons[type].active = true;
				OnEquipWeapon(type);
			}
			else if (totalAmmo[type] <= 0)
			{
				totalAmmo[type] = gun->clipAmmo;
			}
			else if (totalAmmo[type] + gun->clipAmmo > maxTotalAmmo[type])
			{
				totalAmmo[type] = maxTotalAmmo[type];
			}
			else totalAmmo[type] += gun->clipAmmo;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFirstPersonCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind crouch events
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AFirstPersonCharacter::StartCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AFirstPersonCharacter::StopCrouch);

	// Bind fire events
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFirstPersonCharacter::OnFire);
	//PlayerInputComponent->BindAction("SecondaryFire", IE_Pressed, this, &AFirstPersonCharacter::OnSecondaryFire);

	// Bind sprint events
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AFirstPersonCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AFirstPersonCharacter::Walk);

	// Bind Weapon Events
	PlayerInputComponent->BindAction("Melee", IE_Pressed, this, &AFirstPersonCharacter::OnMelee);
	PlayerInputComponent->BindAction("Revolver", IE_Pressed, this, &AFirstPersonCharacter::OnRevolver);
	PlayerInputComponent->BindAction("Shotgun", IE_Pressed, this, &AFirstPersonCharacter::OnShotgun);
	PlayerInputComponent->BindAction("Rifle", IE_Pressed, this, &AFirstPersonCharacter::OnRifle);
	PlayerInputComponent->BindAction("SwitchWeapon", IE_Pressed, this, &AFirstPersonCharacter::OnSwitchWeapon);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AFirstPersonCharacter::OnReload);
	PlayerInputComponent->BindAction("DropWeapon", IE_Pressed, this, &AFirstPersonCharacter::OnDropWeapon);

	//PlayerInputComponent->BindAction("Lamp", IE_Pressed, this, &AFirstPersonCharacter::OnLamp);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AFirstPersonCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFirstPersonCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFirstPersonCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFirstPersonCharacter::LookUpAtRate);
}

void AFirstPersonCharacter::OnFire()
{
	if (ProjectileClass != nullptr)
	{
		if (EquippedGun != Melee)
		{
			if (weapons[EquippedGun].clipAmmo > 0)
			{
				UWorld* const World = GetWorld();
				if (World != nullptr)
				{
					const FRotator SpawnRotation = GetControlRotation();
					// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
					const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

					// Replicate fire event
					if (!World->IsServer()) Server_OnFire(SpawnLocation, SpawnRotation);
					else Multi_OnFire(SpawnLocation, SpawnRotation);

					weapons[EquippedGun].clipAmmo -= 1;
				}

				// try and play a firing animation if specified
				if (FireAnimation != nullptr)
				{
					// Get the animation object for the arms mesh
					UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
					if (AnimInstance != nullptr)
					{
						AnimInstance->Montage_Play(FireAnimation, 1.f);
					}
				}
			}
			else //Not enough ammo in the weapon
			{
				//TODO: just play 'click' sound from trigger
			}
		}
		else // Melee Weapon Equipped
		{
			//TODO
		}
	}
}

bool AFirstPersonCharacter::Server_OnFire_Validate(FVector Location, FRotator Rotation)
{
	return true;
}

void AFirstPersonCharacter::Server_OnFire_Implementation(FVector Location, FRotator Rotation)
{
	Multi_OnFire(Location, Rotation);
}

bool AFirstPersonCharacter::Multi_OnFire_Validate(FVector Location, FRotator Rotation)
{
	return true;
}

void AFirstPersonCharacter::Multi_OnFire_Implementation(FVector Location, FRotator Rotation)
{
	//Set Spawn Collision Handling Override
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	// spawn the projectile at the muzzle
	GetWorld()->SpawnActor<AFirstPersonProjectile>(ProjectileClass, Location, Rotation, ActorSpawnParams);

	// try and play the sound if specified
	if (FireSound != nullptr) UGameplayStatics::PlaySoundAtLocation(this, FireSound, Location);
}

void AFirstPersonCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AFirstPersonCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AFirstPersonCharacter::Sprint()
{
	if (!isCrouched)
	{
		if (!GetWorld()->IsServer()) Server_SetMaxWalkSpeed(1500);
		else GetCharacterMovement()->MaxWalkSpeed = 1500;
	}
	//TODO: stamina system
}

void AFirstPersonCharacter::Walk()
{
	if (!GetWorld()->IsServer()) Server_SetMaxWalkSpeed(600);
	else GetCharacterMovement()->MaxWalkSpeed = 600;
}

bool AFirstPersonCharacter::Server_SetMaxWalkSpeed_Validate(float speed)
{
	return true;
}

void AFirstPersonCharacter::Server_SetMaxWalkSpeed_Implementation(float speed)
{
	GetCharacterMovement()->MaxWalkSpeed = speed;
}

void AFirstPersonCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFirstPersonCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());

	// replicate LookRotation
	if (!GetWorld()->IsServer()) Server_LookUp(GetControlRotation());
	else LookRotation = GetControlRotation();
}

bool AFirstPersonCharacter::Server_LookUp_Validate(FRotator Rotation)
{
	return true;
}

void AFirstPersonCharacter::Server_LookUp_Implementation(FRotator Rotation)
{
	LookRotation = Rotation;
}

void AFirstPersonCharacter::StartCrouch()
{
	//Stop Sprinting if player crouches while sprint
	if (GetCharacterMovement()->MaxWalkSpeed > 600) Walk();

	Crouch();

	//Replicate crouch
	if (!GetWorld()->IsServer()) Server_isCrouch(true);
	else isCrouched = true;
}

void AFirstPersonCharacter::StopCrouch()
{
	UnCrouch();

	//Replicate uncrouch
	if (!GetWorld()->IsServer()) Server_isCrouch(false);
	else isCrouched = false;
}

bool AFirstPersonCharacter::Server_isCrouch_Validate(bool val)
{
	return true;
}

void AFirstPersonCharacter::Server_isCrouch_Implementation(bool val)
{
	isCrouched = val;
}

void AFirstPersonCharacter::OnMelee()
{
	if (EquippedGun != Melee && weapons[Melee].active != false)
	{
		//TODO
	}
}

void AFirstPersonCharacter::OnRevolver()
{
	if (EquippedGun != Revolver && weapons[Revolver].active != false)
	{
		CachedGun = EquippedGun;
		EquippedGun = Revolver;
	}
}

void AFirstPersonCharacter::OnShotgun()
{
	if (EquippedGun != Shotgun && weapons[Shotgun].active != false)
	{
		CachedGun = EquippedGun;
		EquippedGun = Shotgun;
	}
}

void AFirstPersonCharacter::OnRifle()
{
	if (EquippedGun != Rifle && weapons[Rifle].active != false)
	{
		CachedGun = EquippedGun;
		EquippedGun = Rifle;
	}
}

void AFirstPersonCharacter::OnSwitchWeapon()
{
	auto tmp = EquippedGun;
	EquippedGun = CachedGun;
	CachedGun = tmp;
}

void AFirstPersonCharacter::OnEquipWeapon(EWeaponType weapontype)
{
	FP_GunMeshes[EquippedGun]->SetHiddenInGame(true);
	TP_GunMeshes[EquippedGun]->SetHiddenInGame(true);

	FP_GunMeshes[weapontype]->SetHiddenInGame(false);
	TP_GunMeshes[weapontype]->SetHiddenInGame(false);

	CachedGun = EquippedGun;
	EquippedGun = weapontype;
}

void AFirstPersonCharacter::OnReload()
{
	if (EquippedGun != Melee)
	{
		int missingAmmoCount = weapons[EquippedGun].maxClipAmmo - weapons[EquippedGun].clipAmmo;
		if (missingAmmoCount > 0)
		{
			for (int i = 0; i < missingAmmoCount; i++)
			{
				if (totalAmmo[EquippedGun] > 0)
				{
					weapons[EquippedGun].clipAmmo++;
					totalAmmo[EquippedGun]--;
				}
				else break; // out of ammo
			}
		}
	}
}

void AFirstPersonCharacter::OnDropWeapon()
{
}