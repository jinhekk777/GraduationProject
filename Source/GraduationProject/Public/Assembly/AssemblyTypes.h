#pragma once

#include "CoreMinimal.h"
#include "AssemblyTypes.generated.h"

/** Runtime state for one movable assembly part. */
UENUM(BlueprintType)
enum class EAssemblyPartState : uint8
{
	Idle,
	Held,
	Guided,
	Completed,
	Resetting
};

/** Which side of the target's local Z axis is the valid insertion entry. */
UENUM(BlueprintType)
enum class EAssemblyEntrySide : uint8
{
	PositiveLocalZ,
	NegativeLocalZ
};

/** Which target-local axis is used to measure the valid insertion approach. */
UENUM(BlueprintType)
enum class EAssemblyEntryAxis : uint8
{
	LocalX,
	LocalY,
	LocalZ
};

/** Per-run metrics for one assembly part. Reset clears the pose but preserves the run totals. */
USTRUCT(BlueprintType)
struct FAssemblyRunStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	int32 GuidanceEntryCount = 0;

	/** Number of times the automatic in-world guidance was displayed. */
	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	int32 HintCount = 0;

	/** A held part was released without completing an assembly. */
	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	int32 ErrorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	int32 CompletionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	int32 ResetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	float LatestCompletionSeconds = -1.0f;
};

/** Pose-validation values exposed for blueprint feedback and debugging. Distances are in centimetres. */
USTRUCT(BlueprintType)
struct FAssemblyPoseEvaluation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	bool bHasValidConfiguration = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	bool bHasCompatibleRotation = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	bool bIsWithinGuidanceVolume = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	bool bIsWithinSnapRange = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	bool bIsAtFinalPose = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	float PositionErrorCm = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	float LateralErrorCm = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	float DistanceAlongEntryAxisCm = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assembly")
	float RotationErrorDegrees = 0.0f;
};
