#include "Assembly/AssemblyTargetComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UAssemblyTargetComponent::UAssemblyTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAssemblyTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bCompletionCollapseActive || !GuidanceMesh)
	{
		SetComponentTickEnabled(false);
		return;
	}

	CompletionElapsedSeconds += DeltaTime;
	if (CompletionElapsedSeconds < CompletionFeedbackHoldSeconds)
	{
		SetGuidanceOpacity(CompletionOpacity);
		GuidanceMesh->SetRelativeScale3D(GuidanceMeshBaseScale * CompletionStartScale);
		return;
	}

	const float CollapseElapsedSeconds = CompletionElapsedSeconds - CompletionFeedbackHoldSeconds;
	const float Alpha = CompletionCollapseSeconds > 0.0f
		? FMath::Clamp(CollapseElapsedSeconds / CompletionCollapseSeconds, 0.0f, 1.0f)
		: 1.0f;
	SetGuidanceOpacity(FMath::Lerp(CompletionOpacity, 0.0f, Alpha));
	GuidanceMesh->SetRelativeScale3D(GuidanceMeshBaseScale * FMath::Lerp(CompletionStartScale, CompletionEndScale, Alpha));

	if (Alpha >= 1.0f)
	{
		bCompletionCollapseActive = false;
		ClearFeedback();
	}
}

void UAssemblyTargetComponent::SetCompletionFeedbackHoldSeconds(float InHoldSeconds)
{
	CompletionFeedbackHoldSeconds = FMath::Max(0.0f, InHoldSeconds);
}

FTransform UAssemblyTargetComponent::GetAssemblyTargetTransform() const
{
	return GetComponentTransform();
}

void UAssemblyTargetComponent::SetGuidanceMesh(UStaticMeshComponent* InGuidanceMesh)
{
	GuidanceMesh = InGuidanceMesh;
	GuidanceMaterial = GuidanceMesh ? GuidanceMesh->CreateDynamicMaterialInstance(0) : nullptr;
	GuidanceMeshBaseScale = GuidanceMesh ? GuidanceMesh->GetRelativeScale3D() : FVector::OneVector;
	ClearFeedback();
}

void UAssemblyTargetComponent::SetGuidanceFeedbackActive(bool bInGuidanceActive)
{
	if (!GuidanceMesh)
	{
		return;
	}

	bCompletionCollapseActive = false;
	GuidanceMesh->SetRelativeScale3D(GuidanceMeshBaseScale);
	SetGuidanceOpacity(GuidanceOpacity);
	GuidanceMesh->SetVisibility(bInGuidanceActive);
}

void UAssemblyTargetComponent::PlayCompletionFeedback(EControllerHand HapticHand)
{
	if (CompletionSound)
	{
		UE_LOG(LogTemp, Log, TEXT("Assembly completion sound: '%s' as head-relative confirmation for '%s'."),
			*GetNameSafe(CompletionSound), *GetNameSafe(GetOwner()));
		UGameplayStatics::PlaySound2D(this, CompletionSound, CompletionSoundVolume);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Assembly completion feedback has no sound configured on '%s'."), *GetNameSafe(GetOwner()));
	}

	PlayCompletionHaptics(HapticHand);

	if (!GuidanceMesh)
	{
		return;
	}

	CompletionElapsedSeconds = 0.0f;
	bCompletionCollapseActive = true;
	GuidanceMesh->SetRelativeScale3D(GuidanceMeshBaseScale * CompletionStartScale);
	SetGuidanceOpacity(CompletionOpacity);
	GuidanceMesh->SetVisibility(true);
	SetComponentTickEnabled(true);
}

void UAssemblyTargetComponent::ClearFeedback()
{
	if (GuidanceMesh)
	{
		SetGuidanceOpacity(0.0f);
		GuidanceMesh->SetRelativeScale3D(GuidanceMeshBaseScale);
		GuidanceMesh->SetVisibility(false);
	}
}

void UAssemblyTargetComponent::SetGuidanceOpacity(float Opacity)
{
	if (GuidanceMaterial)
	{
		GuidanceMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
}

void UAssemblyTargetComponent::PlayCompletionHaptics(EControllerHand HapticHand)
{
	if (!bPlayCompletionHaptics)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	if (HapticHand == EControllerHand::Left || HapticHand == EControllerHand::AnyHand)
	{
		PlayerController->SetHapticsByValue(CompletionHapticFrequency, CompletionHapticAmplitude, EControllerHand::Left);
	}
	if (HapticHand == EControllerHand::Right || HapticHand == EControllerHand::AnyHand)
	{
		PlayerController->SetHapticsByValue(CompletionHapticFrequency, CompletionHapticAmplitude, EControllerHand::Right);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HapticStopTimer, this, &UAssemblyTargetComponent::StopCompletionHaptics, CompletionHapticSeconds, false);
	}
}

void UAssemblyTargetComponent::StopCompletionHaptics()
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->SetHapticsByValue(0.0f, 0.0f, EControllerHand::Left);
		PlayerController->SetHapticsByValue(0.0f, 0.0f, EControllerHand::Right);
	}
}
