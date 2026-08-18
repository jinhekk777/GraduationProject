#include "Assembly/AssemblyPartComponent.h"

#include "Assembly/AssemblyTargetComponent.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "MotionControllerComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogGraduationAssembly, Log, All);

UAssemblyPartComponent::UAssemblyPartComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAssemblyPartComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!HasValidConfiguration())
	{
		LogConfigurationWarningOnce();
		return;
	}

	InitialTransform = ResolveControlledComponent()->GetComponentTransform();
	bHasInitialTransform = true;
	RunStats = FAssemblyRunStats();
	RunStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	UpdateEntryApproachState();
	SetComponentTickEnabled(bAutoSnapWhenInRange || bHideXRHandWhenHeld);
	BindExternalGrabEvents();
	BroadcastRunStats();
	UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly part '%s' initialized. Target='%s', AutoSnap=%s."),
		*GetNameSafe(GetOwner()), *GetNameSafe(TargetComponent), bAutoSnapWhenInRange ? TEXT("true") : TEXT("false"));
}

void UAssemblyPartComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreActiveXRHandVisibility();
	Super::EndPlay(EndPlayReason);
}

void UAssemblyPartComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateExternalGrabHandVisibility();
	UpdateEntryApproachState();
	if (AssemblyState == EAssemblyPartState::Completed)
	{
		if (!bAllowReassemblyAfterCompletion || EvaluateCurrentPose().bIsAtFinalPose)
		{
			return;
		}

		RestoreFreeAssemblyPhysics();
		SetAssemblyState(EAssemblyPartState::Idle);
		UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly part '%s' rearmed after being moved from its completed pose."), *GetNameSafe(GetOwner()));
	}

	if (bIsHeld || bAutoSnapWhenInRange)
	{
		UpdateGuidanceState();
	}
}

void UAssemblyPartComponent::NotifyGrabStarted()
{
	if (!HasValidConfiguration())
	{
		LogConfigurationWarningOnce();
		return;
	}

	if (AssemblyState == EAssemblyPartState::Completed)
	{
		UE_LOG(LogGraduationAssembly, Verbose, TEXT("Ignored grab on completed assembly part '%s'."), *GetNameSafe(GetOwner()));
		return;
	}

	bIsHeld = true;
	SetComponentTickEnabled(true);
	SetAssemblyState(EAssemblyPartState::Held);
	UpdateGuidanceState();
}

void UAssemblyPartComponent::NotifyGrabEnded()
{
	if (!bIsHeld)
	{
		return;
	}

	bIsHeld = false;
	SetComponentTickEnabled(bAutoSnapWhenInRange || bHideXRHandWhenHeld);
	UpdateEntryApproachState();

	const FAssemblyPoseEvaluation Evaluation = EvaluateCurrentPose();
	if (bSnapOnRelease && bEntryApproachArmed && Evaluation.bIsWithinSnapRange && SnapToTarget())
	{
		return;
	}

	if (bGuidanceWasAvailable)
	{
		bGuidanceWasAvailable = false;
		OnGuidanceChanged.Broadcast(false);
	}

	SetAssemblyState(EAssemblyPartState::Idle);
}

void UAssemblyPartComponent::ResetAssembly()
{
	RestoreActiveXRHandVisibility();
	SetAssemblyState(EAssemblyPartState::Resetting);
	bIsHeld = false;
	bWasExternallyHeld = false;
	bAttemptTimerActive = false;
	bEntryApproachArmed = false;
	SetComponentTickEnabled(false);
	RestoreFreeAssemblyPhysics();

	if (bRestoreInitialTransformOnReset && bHasInitialTransform)
	{
		if (USceneComponent* Component = ResolveControlledComponent())
		{
			Component->SetWorldTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	if (bGuidanceWasAvailable)
	{
		bGuidanceWasAvailable = false;
		OnGuidanceChanged.Broadcast(false);
	}

	SetAssemblyState(EAssemblyPartState::Idle);
	SetComponentTickEnabled(bAutoSnapWhenInRange || bHideXRHandWhenHeld);
	++RunStats.ResetCount;
	BroadcastRunStats();
	OnAssemblyReset.Broadcast();
}

FAssemblyPoseEvaluation UAssemblyPartComponent::EvaluateCurrentPose() const
{
	FAssemblyPoseEvaluation Result;
	if (!HasValidConfiguration())
	{
		return Result;
	}

	const FTransform CurrentTransform = ResolveControlledComponent()->GetComponentTransform();
	const FTransform TargetTransform = TargetComponent->GetAssemblyTargetTransform();
	const FVector PositionDelta = CurrentTransform.GetLocation() - TargetTransform.GetLocation();
	const FVector EntryDirection = GetEntryAxis(TargetTransform);

	Result.bHasValidConfiguration = true;
	Result.PositionErrorCm = PositionDelta.Size();
	Result.DistanceAlongEntryAxisCm = FVector::DotProduct(PositionDelta, EntryDirection);
	Result.LateralErrorCm = (PositionDelta - EntryDirection * Result.DistanceAlongEntryAxisCm).Size();
	Result.RotationErrorDegrees = FMath::RadiansToDegrees(CurrentTransform.GetRotation().AngularDistance(TargetTransform.GetRotation()));
	Result.bHasCompatibleRotation = Result.RotationErrorDegrees <= RotationToleranceDegrees;

	const bool bHasMatchingScale = CurrentTransform.GetScale3D().Equals(TargetTransform.GetScale3D(), ScaleTolerance);
	const bool bHasFinalRotation = Result.RotationErrorDegrees <= FinalRotationToleranceDegrees;
	Result.bIsAtFinalPose = Result.PositionErrorCm <= FinalPositionToleranceCm && bHasFinalRotation && bHasMatchingScale;
	const bool bIsOnEntrySide = Result.DistanceAlongEntryAxisCm >= -FinalPositionToleranceCm;
	const bool bIsWithinGuidanceLength = Result.DistanceAlongEntryAxisCm <= GuidanceLengthCm;
	Result.bIsWithinGuidanceVolume = Result.bHasCompatibleRotation
		&& bHasMatchingScale
		&& Result.LateralErrorCm <= LateralToleranceCm
		&& bIsOnEntrySide
		&& bIsWithinGuidanceLength;
	Result.bIsWithinSnapRange = Result.bIsWithinGuidanceVolume && Result.PositionErrorCm <= SnapDistanceCm;

	return Result;
}

USceneComponent* UAssemblyPartComponent::ResolveControlledComponent() const
{
	if (ControlledComponent)
	{
		return ControlledComponent;
	}

	return GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
}

bool UAssemblyPartComponent::HasValidConfiguration() const
{
	return ResolveControlledComponent() != nullptr && TargetComponent != nullptr;
}

FVector UAssemblyPartComponent::GetEntryAxis(const FTransform& TargetTransform) const
{
	const float Direction = EntrySide == EAssemblyEntrySide::PositiveLocalZ ? 1.0f : -1.0f;
	EAxis::Type LocalAxis = EAxis::Z;
	switch (EntryAxis)
	{
	case EAssemblyEntryAxis::LocalX:
		LocalAxis = EAxis::X;
		break;
	case EAssemblyEntryAxis::LocalY:
		LocalAxis = EAxis::Y;
		break;
	case EAssemblyEntryAxis::LocalZ:
	default:
		break;
	}

	return TargetTransform.GetUnitAxis(LocalAxis) * Direction;
}

void UAssemblyPartComponent::UpdateEntryApproachState()
{
	if (!HasValidConfiguration())
	{
		return;
	}

	const FAssemblyPoseEvaluation Evaluation = EvaluateCurrentPose();
	if (Evaluation.DistanceAlongEntryAxisCm < -FinalPositionToleranceCm)
	{
		if (bEntryApproachArmed)
		{
			UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly approach for '%s' disarmed: part crossed the invalid entry side."), *GetNameSafe(GetOwner()));
		}
		bEntryApproachArmed = false;
		return;
	}

	if (!bEntryApproachArmed && Evaluation.DistanceAlongEntryAxisCm >= ApproachRearmDistanceCm)
	{
		bEntryApproachArmed = true;
		UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly approach for '%s' armed on the valid entry side."), *GetNameSafe(GetOwner()));
	}
}

void UAssemblyPartComponent::UpdateGuidanceState()
{
	const FAssemblyPoseEvaluation Evaluation = EvaluateCurrentPose();
	if (bAutoSnapWhenInRange && bEntryApproachArmed && Evaluation.bIsWithinSnapRange && SnapToTarget())
	{
		return;
	}

	const bool bGuidanceAvailable = Evaluation.bIsWithinGuidanceVolume;
	if (bGuidanceAvailable != bGuidanceWasAvailable)
	{
		bGuidanceWasAvailable = bGuidanceAvailable;
		OnGuidanceChanged.Broadcast(bGuidanceAvailable);
		TargetComponent->SetGuidanceFeedbackActive(bGuidanceAvailable);
		if (bGuidanceAvailable)
		{
			++RunStats.GuidanceEntryCount;
			++RunStats.HintCount;
			BroadcastRunStats();
		}
		UE_LOG(LogGraduationAssembly, Log,
			TEXT("Assembly guidance for '%s': %s (Position=%.2fcm, Lateral=%.2fcm, Along=%.2fcm, Rotation=%.2fdeg, Snap=%s)."),
			*GetNameSafe(GetOwner()), bGuidanceAvailable ? TEXT("entered") : TEXT("left"), Evaluation.PositionErrorCm,
			Evaluation.LateralErrorCm, Evaluation.DistanceAlongEntryAxisCm, Evaluation.RotationErrorDegrees,
			Evaluation.bIsWithinSnapRange ? TEXT("true") : TEXT("false"));
	}

	SetAssemblyState(bGuidanceAvailable ? EAssemblyPartState::Guided : EAssemblyPartState::Held);
}

void UAssemblyPartComponent::SetAssemblyState(EAssemblyPartState NewState)
{
	if (AssemblyState == NewState)
	{
		return;
	}

	AssemblyState = NewState;
	OnAssemblyStateChanged.Broadcast(AssemblyState);
}

bool UAssemblyPartComponent::SnapToTarget()
{
	if (!HasValidConfiguration())
	{
		LogConfigurationWarningOnce();
		return false;
	}
	if (bSnapInProgress)
	{
		return false;
	}
	bSnapInProgress = true;

	USceneComponent* ControlledSceneComponent = ResolveControlledComponent();
	const EControllerHand CompletionHand = bHasActiveGrabHand ? ActiveGrabHand : EControllerHand::AnyHand;
	const FAssemblyPoseEvaluation Evaluation = EvaluateCurrentPose();
	ReleaseExternalGrabBeforeSnap(ControlledSceneComponent);
	LockAssemblyPhysics();
	ControlledSceneComponent->SetWorldTransform(TargetComponent->GetAssemblyTargetTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	SetComponentTickEnabled(bAutoSnapWhenInRange && bAllowReassemblyAfterCompletion);
	SetAssemblyState(EAssemblyPartState::Completed);
	TargetComponent->PlayCompletionFeedback(CompletionHand);
	++RunStats.CompletionCount;
	if (UWorld* World = GetWorld())
	{
		RunStats.LatestCompletionSeconds = static_cast<float>(World->GetTimeSeconds() - (bAttemptTimerActive ? AttemptStartTimeSeconds : RunStartTimeSeconds));
	}
	else
	{
		RunStats.LatestCompletionSeconds = -1.0f;
	}
	bAttemptTimerActive = false;
	BroadcastRunStats();
	OnAssemblyCompleted.Broadcast();
	bSnapInProgress = false;
	UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly completed for '%s' (Position=%.2fcm, Along=%.2fcm, Rotation=%.2fdeg)."),
		*GetNameSafe(GetOwner()), Evaluation.PositionErrorCm, Evaluation.DistanceAlongEntryAxisCm, Evaluation.RotationErrorDegrees);
	return true;
}

void UAssemblyPartComponent::ReleaseExternalGrabBeforeSnap(USceneComponent* ControlledSceneComponent)
{
	if (!ControlledSceneComponent || !ControlledSceneComponent->GetAttachParent())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		UFunction* ReleaseFunction = Component ? Component->FindFunction(TEXT("TryRelease")) : nullptr;
		if (!ReleaseFunction)
		{
			continue;
		}

		TArray<uint8> Parameters;
		Parameters.SetNumZeroed(ReleaseFunction->ParmsSize);
		Component->ProcessEvent(ReleaseFunction, Parameters.GetData());
		break;
	}

	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ControlledSceneComponent))
	{
		PrimitiveComponent->SetSimulatePhysics(false);
	}
}

void UAssemblyPartComponent::LockAssemblyPhysics()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
		PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		PrimitiveComponent->SetEnableGravity(false);
		PrimitiveComponent->SetSimulatePhysics(false);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void UAssemblyPartComponent::RestoreFreeAssemblyPhysics()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PrimitiveComponent->SetEnableGravity(true);
	}
}

void UAssemblyPartComponent::BindExternalGrabEvents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (!Component || !Component->FindFunction(TEXT("GetHeldByHand"))
			|| !FindFProperty<FObjectPropertyBase>(Component->GetClass(), TEXT("MotionControllerRef")))
		{
			continue;
		}

		ExternalGrabComponent = Component;
		UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly part '%s' found grab state source '%s'."), *GetNameSafe(Owner), *GetNameSafe(Component));
		return;
	}

	UE_LOG(LogGraduationAssembly, Warning, TEXT("Assembly part '%s' could not find a compatible external grab component."), *GetNameSafe(Owner));
}

bool UAssemblyPartComponent::TryGetExternalHeldHand(EControllerHand& OutHand) const
{
	if (!ExternalGrabComponent)
	{
		return false;
	}

	const FObjectPropertyBase* MotionControllerProperty = FindFProperty<FObjectPropertyBase>(ExternalGrabComponent->GetClass(), TEXT("MotionControllerRef"));
	if (!MotionControllerProperty || !MotionControllerProperty->GetObjectPropertyValue_InContainer(ExternalGrabComponent))
	{
		return false;
	}

	UMotionControllerComponent* MotionController = Cast<UMotionControllerComponent>(MotionControllerProperty->GetObjectPropertyValue_InContainer(ExternalGrabComponent));
	if (!MotionController)
	{
		UE_LOG(LogGraduationAssembly, Warning, TEXT("Assembly part '%s' received a non-motion-controller grab reference."), *GetNameSafe(GetOwner()));
		return false;
	}

	USceneComponent* ControlledSceneComponent = ResolveControlledComponent();
	if (!ControlledSceneComponent || !ControlledSceneComponent->IsAttachedTo(MotionController))
	{
		return false;
	}

	const FString MotionSource = MotionController->GetTrackingMotionSource().ToString();
	if (MotionSource.Contains(TEXT("Right"), ESearchCase::IgnoreCase))
	{
		OutHand = EControllerHand::Right;
		return true;
	}
	if (MotionSource.Contains(TEXT("Left"), ESearchCase::IgnoreCase))
	{
		OutHand = EControllerHand::Left;
		return true;
	}

	UE_LOG(LogGraduationAssembly, Warning, TEXT("Assembly part '%s' received an unsupported motion source '%s'."), *GetNameSafe(GetOwner()), *MotionSource);
	return false;
}

void UAssemblyPartComponent::UpdateExternalGrabHandVisibility()
{
	EControllerHand HeldHand = EControllerHand::Left;
	bool bIsExternallyHeld = TryGetExternalHeldHand(HeldHand);
	if (bIsExternallyHeld)
	{
		bWasExternallyHeld = true;
		if (!bIsHeld && AssemblyState != EAssemblyPartState::Completed)
		{
			// Do not start a new measured attempt merely by grabbing a part that is
			// still locked at its completed pose. The timer starts once it has been
			// moved far enough to rearm and therefore becomes a real assembly attempt.
			StartAttemptTimerIfNeeded();
			NotifyGrabStarted();
		}
	}

	if (!bIsExternallyHeld && bWasExternallyHeld)
	{
		bWasExternallyHeld = false;
		if (AssemblyState != EAssemblyPartState::Completed)
		{
			NotifyGrabEnded();
			if (AssemblyState != EAssemblyPartState::Completed)
			{
				RecordFailedAttempt();
			}
		}
		else
		{
			bIsHeld = false;
		}
	}

	if (!bHideXRHandWhenHeld || !bIsExternallyHeld)
	{
		RestoreActiveXRHandVisibility();
		return;
	}

	if (!bHasActiveGrabHand || ActiveGrabHand != HeldHand)
	{
		RestoreActiveXRHandVisibility();
		ActiveGrabHand = HeldHand;
		bHasActiveGrabHand = true;
		SetXRHandVisibility(true, ActiveGrabHand);
	}
}

void UAssemblyPartComponent::StartAttemptTimerIfNeeded()
{
	if (bAttemptTimerActive)
	{
		return;
	}

	AttemptStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	bAttemptTimerActive = true;
	UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly attempt started for '%s'."), *GetNameSafe(GetOwner()));
}

void UAssemblyPartComponent::RecordFailedAttempt()
{
	++RunStats.ErrorCount;
	BroadcastRunStats();
	UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly attempt failed for '%s'. Errors=%d."), *GetNameSafe(GetOwner()), RunStats.ErrorCount);
}

void UAssemblyPartComponent::SetXRHandVisibility(bool bHide, EControllerHand Hand) const
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	UFunction* HideUnhideFunction = PlayerPawn ? PlayerPawn->FindFunction(TEXT("HideUnhideHand")) : nullptr;
	if (!HideUnhideFunction)
	{
		UE_LOG(LogGraduationAssembly, Warning, TEXT("Assembly part '%s' could not access XR pawn hand visibility."), *GetNameSafe(GetOwner()));
		return;
	}

	TArray<uint8> Parameters;
	Parameters.SetNumZeroed(HideUnhideFunction->ParmsSize);
	for (TFieldIterator<FBoolProperty> It(HideUnhideFunction); It; ++It)
	{
		if (!It->HasAnyPropertyFlags(CPF_Parm))
		{
			continue;
		}

		if (It->GetFName() == TEXT("bHide"))
		{
			It->SetPropertyValue_InContainer(Parameters.GetData(), bHide);
		}
		else if (It->GetFName() == TEXT("bRightHand"))
		{
			It->SetPropertyValue_InContainer(Parameters.GetData(), Hand == EControllerHand::Right);
		}
	}

	PlayerPawn->ProcessEvent(HideUnhideFunction, Parameters.GetData());
	UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly part '%s' set %s hand visibility to %s."), *GetNameSafe(GetOwner()),
		Hand == EControllerHand::Right ? TEXT("right") : TEXT("left"), bHide ? TEXT("hidden") : TEXT("visible"));
}

void UAssemblyPartComponent::RestoreActiveXRHandVisibility()
{
	if (!bHasActiveGrabHand)
	{
		return;
	}

	SetXRHandVisibility(false, ActiveGrabHand);
	bHasActiveGrabHand = false;
}

void UAssemblyPartComponent::BroadcastRunStats()
{
	OnRunStatsChanged.Broadcast(RunStats);
	UE_LOG(LogGraduationAssembly, Log, TEXT("Assembly stats for '%s': Time=%.2fs, Errors=%d, Hints=%d, Guidance=%d, Completions=%d, Resets=%d."),
		*GetNameSafe(GetOwner()), RunStats.LatestCompletionSeconds, RunStats.ErrorCount, RunStats.HintCount,
		RunStats.GuidanceEntryCount, RunStats.CompletionCount, RunStats.ResetCount);
}

void UAssemblyPartComponent::LogConfigurationWarningOnce()
{
	if (!bConfigurationWarningEmitted)
	{
		bConfigurationWarningEmitted = true;
		UE_LOG(LogGraduationAssembly, Warning, TEXT("Assembly part '%s' requires a controlled scene component and an assembly target component."), *GetNameSafe(GetOwner()));
	}
}
