// Copyright 2023 by Sulley. All Rights Reserved.

#include "Core/ECameraSettingsComponent.h"
#include "Core/ECameraBase.h"
#include "Core/ECameraManager.h"
#include "Core/ECameraComponentBase.h"
#include "Components/ECameraComponentAim.h"
#include "Components/ECameraComponentFollow.h"
#include "Components/HardLockAim.h"
#include "Extensions/ECameraExtensionBase.h"
#include "Extensions/KeyframeExtension.h"
#include "Utils/ECameraTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"


UECameraSettingsComponent::UECameraSettingsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UECameraSettingsComponent::OnRegister()
{
	Super::OnRegister();
	RegisterManager();
	InitializeECameraComponents();
}

void UECameraSettingsComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UECameraSettingsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsActive())
	{
		OnPreTickComponent.Broadcast();
		for (EStage Stage : TEnumRange<EStage>())
		{
			/** Follow component and aim component first. */
			for (UECameraComponentBase* Component : ComponentContainer)
			{
				if (Component != nullptr && Component->GetStage() == Stage) Component->UpdateComponent(DeltaTime);		
			}
			/** Then extensions. */
			for (UECameraExtensionBase* Component : Extensions)
			{
				if (Component != nullptr && Component->GetStage() == Stage) Component->UpdateComponent(DeltaTime);
			}
		}
		OnPostTickComponent.Broadcast();
	}
}

APawn* UECameraSettingsComponent::GetOwningPawn() const
{
	return Cast<APawn>(GetOwner());
}

AECameraBase* UECameraSettingsComponent::GetOwningCamera() const
{
	return Cast<AECameraBase>(GetOwner());
}

APlayerController* UECameraSettingsComponent::GetPlayerController() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

APlayerCameraManager* UECameraSettingsComponent::GetPlayerCameraManager() const
{
	APlayerController* PlayerController = GetPlayerController();
	return PlayerController != nullptr ? PlayerController->PlayerCameraManager : nullptr;
}

AECameraManager* UECameraSettingsComponent::GetECameraManager() const
{
	return ECameraManager;
}

bool UECameraSettingsComponent::IsActive() const
{
	AActor* Owner = GetOwner();

	if (APlayerController* PlayerController = GetPlayerController())
	{
		return Owner == PlayerController->GetViewTarget();
	}
	return false;
}

AActor* UECameraSettingsComponent::SetFollowTarget(AActor* NewFollowTarget)
{
	FollowTarget = NewFollowTarget;
	if (FollowComponent != nullptr)
		FollowComponent->SetFollowTarget(NewFollowTarget);
	return NewFollowTarget;
}

AActor* UECameraSettingsComponent::GetFollowTarget() const
{
	return FollowTarget;
}

UECameraComponentFollow* UECameraSettingsComponent::GetFollowComponent() const
{
	return FollowComponent;
}

AActor* UECameraSettingsComponent::SetAimTarget(AActor* NewAimTarget)
{
	AimTarget = NewAimTarget;
	if (AimComponent != nullptr)
		AimComponent->SetAimTarget(NewAimTarget);
	return NewAimTarget;
}

UECameraExtensionBase* UECameraSettingsComponent::GetExtensionOfClass(TSubclassOf<UECameraExtensionBase> ExtensionClass)
{
	UECameraExtensionBase* Result = nullptr;

	TArray<UECameraExtensionBase*> OwningExtensions = GetExtensions();
	for (UECameraExtensionBase* Extension : OwningExtensions)
	{
		if (Extension->IsA<UKeyframeExtension>())
		{
			Result = Cast<UKeyframeExtension>(Extension);
			break;
		}
	}

	return Result;
}

AActor* UECameraSettingsComponent::GetAimTarget() const
{
	return AimTarget;
}

UECameraComponentAim* UECameraSettingsComponent::GetAimComponent() const
{
	return AimComponent;
}

TArray<UECameraExtensionBase*> UECameraSettingsComponent::GetExtensions() const
{
	return Extensions;
}

void UECameraSettingsComponent::RegisterManager()
{
	UWorld* World = GetWorld();
	AECameraManager* Manager = Cast<AECameraManager>(UGameplayStatics::GetActorOfClass(World, AECameraManager::StaticClass()));
	if (Manager == nullptr)
	{
		Manager = World->SpawnActor<AECameraManager>();
	}
	ECameraManager = Manager;
}

void UECameraSettingsComponent::InitializeECameraComponents()
{
	ComponentContainer.Empty();

	if (FollowComponent != nullptr)
	{
		InitializeECameraComponent(FollowComponent);
		FollowTarget = FollowComponent->GetFollowTarget();
		ComponentContainer.Add(FollowComponent);
	}
	if (AimComponent != nullptr)
	{
		InitializeECameraComponent(AimComponent);
		AimTarget = AimComponent->GetAimTarget();
		ComponentContainer.Add(AimComponent);
	}
	for (UECameraExtensionBase* Extension : Extensions)
		if (Extension != nullptr) 
			InitializeECameraComponent(Extension);

	/** Special case: when you are using HardLockAim and uses the follow target's rotation, TickGroup should be set to TG_PrePhysics.
	 *  This is because when you enable character's OrientRotationToMovement, the tick order would be CharacterLocation -> CameraRotation -> CharacterRotation,
	 *	which creates strong camera jitter. Settings to TG_PrePhysics defers camera component to the next frame to mitigate this problem.
	 */
	if (AimComponent != nullptr && AimComponent->IsA<UHardLockAim>() && AimTarget == FollowTarget)
		PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UECameraSettingsComponent::InitializeECameraComponent(UECameraComponentBase* Component)
{
	Component->SetOwningActor(GetOwner());
	Component->SetOwningCamera(GetOwningCamera());
	Component->SetOwningSettingComponent(this);

	/** Bind delegates. */
	OnPreTickComponent.AddUObject(Component, &UECameraComponentBase::BindToOnPreTickComponent);
	OnPostTickComponent.AddUObject(Component, &UECameraComponentBase::BindToOnPostTickComponent);
	OnBecomeViewTarget.AddUObject(Component, &UECameraComponentBase::BindToOnBecomeViewTarget);
	OnEndViewTarget.AddUObject(Component, &UECameraComponentBase::BindToOnEndViewTarget);
}

void UECameraSettingsComponent::BecomeViewTarget(APlayerController* PC, bool bPreserveState)
{
	/** If preserve state, reset the incoming camera as the state the current camera. */
	if (bPreserveState)
	{
		GetOwner()->SetActorLocation(GetPlayerController()->PlayerCameraManager->GetCameraLocation());
		/** Discard roll. */
		FRotator Rotation = GetPlayerController()->PlayerCameraManager->GetCameraRotation();
		GetOwner()->SetActorRotation(FRotator(Rotation.Pitch, Rotation.Yaw, 0));
	}
	/** Different components have different OnBecomeViewTarget behaviours. */
	if (FollowComponent) FollowComponent->ResetOnBecomeViewTarget(PC, bPreserveState);
	if (AimComponent) AimComponent->ResetOnBecomeViewTarget(PC, bPreserveState);
	for (UECameraExtensionBase* Extension : Extensions)
		if (Extension) Extension->ResetOnBecomeViewTarget(PC, bPreserveState);

	/** User customized OnBecomeViewTarget functions. */
	OnBecomeViewTarget.Broadcast(PC);
}

void UECameraSettingsComponent::EndViewTarget(APlayerController* PC)
{
	/** User customized OnEndViewTarget functions. */
	OnEndViewTarget.Broadcast(PC);

	/** Different components have different OnEndViewTarget behaviours. */
	if (FollowComponent) FollowComponent->ResetOnEndViewTarget(PC);
	if (AimComponent) AimComponent->ResetOnEndViewTarget(PC);
	for (UECameraExtensionBase* Extension : Extensions)
		if (Extension) Extension->ResetOnEndViewTarget(PC);
}

#if WITH_EDITOR
void UECameraSettingsComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property != nullptr)
	{
		// UKismetSystemLibrary::PrintString(this, "[DEBUG INFO]: " + PropertyChangedEvent.Property->GetName() + "(ECameraSettingsComponent.cpp)");
		const FName PropertyName(PropertyChangedEvent.Property->GetName());
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UECameraSettingsComponent, FollowComponent)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(UECameraSettingsComponent, AimComponent)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(UECameraSettingsComponent, Extensions))
		{
			InitializeECameraComponents();
		}
	}
}
#endif