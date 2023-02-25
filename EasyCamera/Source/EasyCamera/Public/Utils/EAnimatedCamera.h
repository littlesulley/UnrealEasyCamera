// Copyright 2023 by Sulley. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ECameraBase.h"
#include "EAnimatedCamera.generated.h"

class UAnimatedCameraExtension;

/**
 * This is a special camera used to be animated. It does nothing but contains a EAnimatedCameraExtension component by default.
 * When you use the CallAnimatedCamera node, an instance of this camera will be invoked and drive camera transform and FOV.
 * DO NOT change its properties!
 */
UCLASS(BlueprintType, NotBlueprintable, classGroup = "ECamera")
class EASYCAMERA_API AEAnimatedCamera : public AECameraBase
{
	GENERATED_BODY()

public:
	AEAnimatedCamera();

protected:
	UAnimatedCameraExtension* AnimatedCameraExtension;
};
