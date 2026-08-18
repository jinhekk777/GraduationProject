#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Assembly/AssemblyTypes.h"
#include "AssemblyPartComponent.generated.h"

class UAssemblyTargetComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAssemblyPartStateChanged, EAssemblyPartState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAssemblyGuidanceChanged, bool, bGuidanceAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAssemblyCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAssemblyReset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAssemblyRunStatsChanged, FAssemblyRunStats, RunStats);

/**
 * Generic validation and controlled-snap behaviour for one movable assembly part.
 * A grab system remains responsible for attachment and input; it calls NotifyGrabStarted/Ended.
 */
UCLASS(ClassGroup = (Assembly), meta = (BlueprintSpawnableComponent))
class GRADUATIONPROJECT_API UAssemblyPartComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAssemblyPartComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Call after the external grab system has attached the controlled component. */
	UFUNCTION(BlueprintCallable, Category = "Assembly|Interaction")
	void NotifyGrabStarted();

	/** Call after the external grab system has released the controlled component. */
	UFUNCTION(BlueprintCallable, Category = "Assembly|Interaction")
	void NotifyGrabEnded();

	UFUNCTION(BlueprintCallable, Category = "Assembly")
	void ResetAssembly();

	UFUNCTION(BlueprintPure, Category = "Assembly")
	FAssemblyPoseEvaluation EvaluateCurrentPose() const;

	UFUNCTION(BlueprintPure, Category = "Assembly")
	EAssemblyPartState GetAssemblyState() const { return AssemblyState; }

	UFUNCTION(BlueprintPure, Category = "Assembly|Analytics")
	FAssemblyRunStats GetRunStats() const { return RunStats; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|References", meta = (UseComponentPicker))
	TObjectPtr<USceneComponent> ControlledComponent;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Assembly|References", meta = (UseComponentPicker))
	TObjectPtr<UAssemblyTargetComponent> TargetComponent;

	/** Valid approach side expressed in the target's local Z axis. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Guidance")
	EAssemblyEntrySide EntrySide = EAssemblyEntrySide::PositiveLocalZ;

	/** Target-local axis used for approach, guidance, and snap validation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Guidance")
	EAssemblyEntryAxis EntryAxis = EAssemblyEntryAxis::LocalZ;

	/** Maximum distance from final pose along the entry side that enables guidance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Guidance", meta = (ClampMin = "0.0", Units = "cm"))
	float GuidanceLengthCm = 30.0f;

	/** Distance on the valid side that re-arms insertion after the part was observed on the wrong side. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Guidance", meta = (ClampMin = "0.0", Units = "cm"))
	float ApproachRearmDistanceCm = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Guidance", meta = (ClampMin = "0.0", Units = "cm"))
	float LateralToleranceCm = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Guidance", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float RotationToleranceDegrees = 15.0f;

	/** Maximum total positional error at release that permits controlled snapping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Snap", meta = (ClampMin = "0.0", Units = "cm"))
	float SnapDistanceCm = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Validation", meta = (ClampMin = "0.0", Units = "cm"))
	float FinalPositionToleranceCm = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Validation", meta = (ClampMin = "0.0", Units = "deg"))
	float FinalRotationToleranceDegrees = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Validation", meta = (ClampMin = "0.0"))
	float ScaleTolerance = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Snap")
	bool bSnapOnRelease = true;

	/** Enables a controlled snap as soon as the part reaches the configured snap range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Snap")
	bool bAutoSnapWhenInRange = false;

	/** Allows a completed test part to be picked up and assembled again without resetting the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Snap")
	bool bAllowReassemblyAfterCompletion = true;

	/** Hide the XR template hand mesh while this part is held in a snapped pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Interaction")
	bool bHideXRHandWhenHeld = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Reset")
	bool bRestoreInitialTransformOnReset = true;

	UPROPERTY(BlueprintAssignable, Category = "Assembly|Events")
	FAssemblyPartStateChanged OnAssemblyStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Assembly|Events")
	FAssemblyGuidanceChanged OnGuidanceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Assembly|Events")
	FAssemblyCompleted OnAssemblyCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Assembly|Events")
	FAssemblyReset OnAssemblyReset;

	UPROPERTY(BlueprintAssignable, Category = "Assembly|Analytics")
	FAssemblyRunStatsChanged OnRunStatsChanged;

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Assembly", meta = (AllowPrivateAccess = "true"))
	EAssemblyPartState AssemblyState = EAssemblyPartState::Idle;

	bool bIsHeld = false;
	bool bGuidanceWasAvailable = false;
	bool bConfigurationWarningEmitted = false;
	bool bHasInitialTransform = false;
	bool bEntryApproachArmed = false;
	bool bSnapInProgress = false;
	bool bWasExternallyHeld = false;
	bool bAttemptTimerActive = false;
	FTransform InitialTransform;
	FAssemblyRunStats RunStats;
	double RunStartTimeSeconds = 0.0;
	double AttemptStartTimeSeconds = 0.0;
	TObjectPtr<UActorComponent> ExternalGrabComponent;
	bool bHasActiveGrabHand = false;
	EControllerHand ActiveGrabHand = EControllerHand::Left;

	USceneComponent* ResolveControlledComponent() const;
	bool HasValidConfiguration() const;
	FVector GetEntryAxis(const FTransform& TargetTransform) const;
	void UpdateEntryApproachState();
	void UpdateGuidanceState();
	void SetAssemblyState(EAssemblyPartState NewState);
	bool SnapToTarget();
	void ReleaseExternalGrabBeforeSnap(USceneComponent* ControlledSceneComponent);
	void LockAssemblyPhysics();
	void RestoreFreeAssemblyPhysics();
	void BindExternalGrabEvents();
	bool TryGetExternalHeldHand(EControllerHand& OutHand) const;
	void UpdateExternalGrabHandVisibility();
	void StartAttemptTimerIfNeeded();
	void RecordFailedAttempt();
	void SetXRHandVisibility(bool bHide, EControllerHand Hand) const;
	void RestoreActiveXRHandVisibility();

	void BroadcastRunStats();
	void LogConfigurationWarningOnce();
};
