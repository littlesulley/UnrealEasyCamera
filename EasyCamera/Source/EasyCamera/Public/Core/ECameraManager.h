// Copyright 2023 by Sulley. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ECameraManager.generated.h"

class UECameraSettingsComponent;
class ACameraActor;
class AECameraBase;

UCLASS(BlueprintType, Blueprintable, classGroup = "ECamera")
class EASYCAMERA_API AECameraManager : public AActor
{
	GENERATED_BODY()

public:
	AECameraManager();

protected:
	/** A container of all cameras in level. */
	TArray<TObjectPtr<AECameraBase>> CameraContainer;

	/** Current active setting component. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AECameraBase> ActiveCamera;

public:
	/** Get the current active camera in level. */
	AECameraBase* GetActiveCamera() { return ActiveCamera; }

public:
	void AddCamera(AECameraBase* Camera);
	bool TerminateActiveCamera();
};
