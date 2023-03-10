// Copyright 2023 by Sulley. All Rights Reserved.

#include "Components/TargetingAim.h"
#include "Core/ECameraSettingsComponent.h"
#include "Components/ECameraComponentAim.h"
#include "Utils/ECameraLibrary.h"
#include "Utils/ECameraTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/HUD.h"

UTargetingAim::UTargetingAim()
{
	Stage = EStage::Aim;

	bLocalRotation = false;
	AimOffset = FVector(0.0f, 0.0f, 0.0f);
	DampParams = FDampParams();
	AimDamping = FVector(0.0f, 0.0f, 0.0f);
	ScreenOffset = FVector2f(0.0f, 0.0f);
	ScreenOffsetWidth = FVector2f(-0.1f, 0.1f);
	ScreenOffsetHeight = FVector2f(-0.1f, 0.1f);
}

void UTargetingAim::UpdateComponent_Implementation(float DeltaTime)
{
	if (AimTarget != nullptr)
	{
		/** Get the *real* aim position, based on actor's local space. */
		FVector AimPosition = GetRealAimPosition();

		/** If camera is too close to aim target, return. */
		if (CheckIfTooClose(AimPosition)) return;

		/** Temporary delta rotation before damping. */
		FRotator TempDeltaRotation = FRotator(0, 0, 0);

		/** Set delta rotation. */
		SetDeltaRotation(AimPosition, TempDeltaRotation);

		/** Get damped delta rotation. */
		FRotator DampedDeltaRotation = DampDeltaRotation(TempDeltaRotation, DeltaTime, AimPosition);

		/** Apply damped delta rotation. */
		if (!bLocalRotation)
		{
			GetOwningActor()->AddActorLocalRotation(FRotator(DampedDeltaRotation.Pitch, 0, 0));
			GetOwningActor()->AddActorWorldRotation(FRotator(0, DampedDeltaRotation.Yaw, 0));

		}
		else GetOwningActor()->AddActorLocalRotation(DampedDeltaRotation);

		/** Update variables for HUD. */
		RealAimPosition = AimPosition;
	}
}

bool UTargetingAim::CheckIfTooClose(const FVector& AimPosition)
{
	float Distance = UKismetMathLibrary::Vector_Distance(GetOwningActor()->GetActorLocation(), AimPosition);
	return UKismetMathLibrary::NearlyEqual_FloatFloat(Distance, 0, 0.001);
}

void UTargetingAim::SetDeltaRotation(const FVector& AimPosition, FRotator& TempDeltaRotation)
{
	/** Version 1: Rotate at world space. */
	if (!bLocalRotation)
	{
		FRotator CenteredDeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(GetOwningActor()->GetActorLocation(), AimPosition), GetOwningActor()->GetActorRotation());
		TempDeltaRotation.Yaw = CenteredDeltaRotation.Yaw - ScreenOffset.X * OwningCamera->GetCameraComponent()->FieldOfView;
		TempDeltaRotation.Pitch = CenteredDeltaRotation.Pitch - ScreenOffset.Y * 2.0f * UKismetMathLibrary::DegAtan(UKismetMathLibrary::DegTan(OwningCamera->GetCameraComponent()->FieldOfView / 2) / OwningCamera->GetCameraComponent()->AspectRatio);
		TempDeltaRotation.Roll = 0;
	}

	/** Version 2: Rotate at local space. */
	else
	{
		FVector Diff = AimPosition - GetOwningActor()->GetActorLocation();

		FVector ForwardVector = GetOwningActor()->GetActorForwardVector();
		FVector RightVector = GetOwningActor()->GetActorRightVector();
		FVector UpVector = GetOwningActor()->GetActorUpVector();

		FVector LocalSpaceAimPosition =
			UKismetMathLibrary::MakeVector(ForwardVector.X, RightVector.X, UpVector.X) * Diff.X +
			UKismetMathLibrary::MakeVector(ForwardVector.Y, RightVector.Y, UpVector.Y) * Diff.Y +
			UKismetMathLibrary::MakeVector(ForwardVector.Z, RightVector.Z, UpVector.Z) * Diff.Z;

		FRotator CenteredDeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(FVector(0, 0, 0), LocalSpaceAimPosition), FRotator(0, 0, 0));
		TempDeltaRotation.Yaw = CenteredDeltaRotation.Yaw - ScreenOffset.X * OwningCamera->GetCameraComponent()->FieldOfView;
		TempDeltaRotation.Pitch = CenteredDeltaRotation.Pitch - ScreenOffset.Y * 2.0f * UKismetMathLibrary::DegAtan(UKismetMathLibrary::DegTan(OwningCamera->GetCameraComponent()->FieldOfView / 2) / OwningCamera->GetCameraComponent()->AspectRatio);
		TempDeltaRotation.Roll = 0;
	}
}

FRotator UTargetingAim::DampDeltaRotation(const FRotator& TempDeltaRotation, float DeltaTime, const FVector& AimPosition)
{
	FRotator DampedDeltaRotation = FRotator(0, 0, 0);
	UECameraLibrary::DamperRotatorWithDifferentDampTime(DampParams, DeltaTime, TempDeltaRotation, AimDamping, DampedDeltaRotation);
	EnsureWithinBounds(DampedDeltaRotation, AimPosition);

	return DampedDeltaRotation;
}

void UTargetingAim::EnsureWithinBounds(FRotator& DampedDeltaRotation, const FVector& AimPosition)
{
	double VFieldOfView = 2.0f * UKismetMathLibrary::DegAtan(UKismetMathLibrary::DegTan(OwningCamera->GetCameraComponent()->FieldOfView / 2) / OwningCamera->GetCameraComponent()->AspectRatio);
	double LeftBound = (ScreenOffset.X + ScreenOffsetWidth.X) * OwningCamera->GetCameraComponent()->FieldOfView;
	double RightBound = (ScreenOffset.X + ScreenOffsetWidth.Y) * OwningCamera->GetCameraComponent()->FieldOfView;
	double BottomBound = (ScreenOffset.Y + ScreenOffsetHeight.X) * VFieldOfView;
	double TopBound = (ScreenOffset.Y + ScreenOffsetHeight.Y) * VFieldOfView;

	FQuat DesiredQuat = GetOwningActor()->GetActorRotation().Quaternion();
	DesiredQuat = FQuat(FRotator(0, DampedDeltaRotation.Yaw, 0)) * DesiredQuat * FQuat(FRotator(DampedDeltaRotation.Pitch, 0, 0));
	FRotator DesiredRotation = DesiredQuat.Rotator();

	FRotator ResultRotationDiff = UKismetMathLibrary::NormalizedDeltaRotator(UKismetMathLibrary::FindLookAtRotation(GetOwningActor()->GetActorLocation(), AimPosition), DesiredRotation);
	if (ResultRotationDiff.Yaw < LeftBound) DampedDeltaRotation.Yaw += ResultRotationDiff.Yaw - LeftBound;
	if (ResultRotationDiff.Yaw > RightBound) DampedDeltaRotation.Yaw += ResultRotationDiff.Yaw - RightBound;
	if (ResultRotationDiff.Pitch < BottomBound) DampedDeltaRotation.Pitch += ResultRotationDiff.Pitch - BottomBound;
	if (ResultRotationDiff.Pitch > TopBound) DampedDeltaRotation.Pitch += ResultRotationDiff.Pitch - TopBound;
}