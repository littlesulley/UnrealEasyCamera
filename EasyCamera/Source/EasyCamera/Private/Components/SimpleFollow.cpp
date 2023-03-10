// Copyright 2023 by Sulley. All Rights Reserved.


#include "Components/SimpleFollow.h"
#include "Utils/ECameraTypes.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"

USimpleFollow::USimpleFollow()
{
	Stage = EStage::Follow;
	FollowType = ESimpleFollowType::WorldSpace;
	FollowOffset = FVector(0.0f, 0.0f, 0.0f);
	AxisMasks = FVector(1.0f, 1.0f, 1.0f);
	DampParams = FDampParams();
	FollowDamping = FVector(0.0f, 0.0f, 0.0f);
	PreviousLocation = FVector(0.0f, 0.0f, 0.0f);
	ExactSpringVel = FVector(0.0f, 0.0f, 0.0f);
}

void USimpleFollow::UpdateComponent_Implementation(float DeltaTime)
{
	if (FollowTarget != nullptr)
	{
		/** Get the *real* follow position, depending on FollowType. */
		FVector FollowPosition = GetRealFollowLocation();

		/** Transform from world space to local space. */
		FVector LocalSpaceFollowPosition = UECameraLibrary::GetLocalSpacePosition(GetOwningActor(), FollowPosition);

		/** Temporary (before damping) delta position. */
		FVector TempDeltaPosition = LocalSpaceFollowPosition;

		/** Transform from local space back to world space. */
		FVector WorldDeltaPosition = UKismetMathLibrary::TransformDirection(GetOwningActor()->GetActorTransform(), TempDeltaPosition);

		/** Apply axis masks. */
		FVector MaskedDeltaPosition = ApplyAxisMask(WorldDeltaPosition);

		/** Get damped delta position. */
		FVector DampedDeltaPosition = DampDeltaPosition(MaskedDeltaPosition, DeltaTime);

		/** Store current position. */
		PreviousLocation = GetOwningActor()->GetActorLocation();

		/** Apply damped delta position. */
		GetOwningActor()->AddActorWorldOffset(DampedDeltaPosition);
	}
}

FVector USimpleFollow::GetRealFollowLocation()
{
	FVector FollowPosition = FVector(0, 0, 0);
	FVector TempFollowPosition = FollowTarget->GetActorLocation();
	FRotator TempRotation = FollowTarget->GetActorRotation();

	/** If SocketName is not empty, use the socket's position. */
	if (SocketName != "")
	{
		UActorComponent* ActorComponent = FollowTarget->GetComponentByClass(USkeletalMeshComponent::StaticClass());
		if (ActorComponent != nullptr)
		{
			USkeletalMeshComponent* SkeletonComponent = Cast<USkeletalMeshComponent>(ActorComponent);
			FTransform SocketTransform = SkeletonComponent->GetSocketTransform(FName(SocketName));
			TempFollowPosition = SocketTransform.GetLocation();
			TempRotation = SocketTransform.Rotator();
		}
	}

	if (FollowType == ESimpleFollowType::WorldSpace)
		FollowPosition = TempFollowPosition;
	else if (FollowType == ESimpleFollowType::LocalSpace)
		FollowPosition = TempFollowPosition + UKismetMathLibrary::GreaterGreater_VectorRotator(FollowOffset, TempRotation);

	return FollowPosition;
}

FVector USimpleFollow::DampDeltaPosition(const FVector& TempDeltaPosition, float DeltaTime)
{
	FVector DampedDeltaPosition = FVector(0, 0, 0);
	if (DampParams.DampMethod == EDampMethod::Naive || DampParams.DampMethod == EDampMethod::Simulate)
		UECameraLibrary::DamperVectorWithDifferentDampTime(DampParams, DeltaTime, TempDeltaPosition, FollowDamping, DampedDeltaPosition);
	else if (DampParams.DampMethod == EDampMethod::Spring)
		UECameraLibrary::SpringDampVector(DampParams, DeltaTime, GetOwningActor()->GetActorLocation() - PreviousLocation, TempDeltaPosition, DampedDeltaPosition);
	else if (DampParams.DampMethod == EDampMethod::ExactSpring)
	{
		FVector CurrentPos = FVector(0, 0, 0);
		FVector CurrentVel = ExactSpringVel;
		FVector TargetPos = TempDeltaPosition;
		const FVector TargetVel = FollowTarget->GetVelocity() / 1.1f;
		FVector OutPos = FVector(0, 0, 0);
		FVector& OutVel = ExactSpringVel;

		UECameraLibrary::ExactSpringDamperVector(CurrentPos, CurrentVel, TargetPos, TargetVel, DampParams.DampRatio, DampParams.HalfLife, DeltaTime, OutPos, OutVel);

		FVector BinaryAxisMasks = FVector(AxisMasks[0] != 0.0f, AxisMasks[1] != 0.0f, AxisMasks[2] != 0.0f);
		DampedDeltaPosition = BinaryAxisMasks * OutPos;
	}

	return DampedDeltaPosition;
}

FVector USimpleFollow::ApplyAxisMask(const FVector& DampedDeltaPosition)
{
	if (FollowType == ESimpleFollowType::WorldSpace)
	{
		FVector BinaryAxisMasks = FVector(AxisMasks[0] != 0.0f, AxisMasks[1] != 0.0f, AxisMasks[2] != 0.0f);
		return BinaryAxisMasks * DampedDeltaPosition + BinaryAxisMasks * FollowOffset;
	}
	/** AxisMasks is only applied to WorldSpace. */
	else return DampedDeltaPosition;
}