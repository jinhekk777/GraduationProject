#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InputCoreTypes.h"
#include "AssemblyTargetComponent.generated.h"

/**
 * Scene anchor that represents the final transform of a movable assembly part.
 * Its transform is authored by blueprint or level placement; no asset-name lookup is performed.
 */
UCLASS(ClassGroup = (Assembly), meta = (BlueprintSpawnableComponent))
class GRADUATIONPROJECT_API UAssemblyTargetComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UAssemblyTargetComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Assembly")
	FTransform GetAssemblyTargetTransform() const;

	/** Optional content-provided mesh that represents the final occupied volume. */
	UFUNCTION(BlueprintCallable, Category = "Assembly|Feedback")
	void SetGuidanceMesh(class UStaticMeshComponent* InGuidanceMesh);

	UFUNCTION(BlueprintCallable, Category = "Assembly|Feedback")
	void SetGuidanceFeedbackActive(bool bInGuidanceActive);

	UFUNCTION(BlueprintCallable, Category = "Assembly|Feedback")
	void PlayCompletionFeedback(EControllerHand HapticHand = EControllerHand::AnyHand);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback")
	float GuidanceOpacity = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback", meta = (ClampMin = "0.0"))
	float CompletionCollapseSeconds = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback", meta = (ClampMin = "1.0"))
	float CompletionStartScale = 1.025f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback", meta = (ClampMin = "1.0"))
	float CompletionEndScale = 1.075f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CompletionOpacity = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback")
	TObjectPtr<class USoundBase> CompletionSound;

	/** Confirmation cues are head-relative so they remain audible even when the target is outside the listener's focus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback", meta = (ClampMin = "0.0"))
	float CompletionSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback")
	bool bPlayCompletionHaptics = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CompletionHapticFrequency = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CompletionHapticAmplitude = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback", meta = (ClampMin = "0.0"))
	float CompletionHapticSeconds = 0.06f;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UStaticMeshComponent> GuidanceMesh;

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> GuidanceMaterial;

	FVector GuidanceMeshBaseScale = FVector::OneVector;
	float CompletionElapsedSeconds = 0.0f;
	bool bCompletionCollapseActive = false;
	FTimerHandle HapticStopTimer;

	void SetGuidanceOpacity(float Opacity);
	void PlayCompletionHaptics(EControllerHand HapticHand);
	void StopCompletionHaptics();
	void ClearFeedback();
};
