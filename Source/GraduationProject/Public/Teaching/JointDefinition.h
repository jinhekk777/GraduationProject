#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JointDefinition.generated.h"

class UStaticMesh;
class USoundBase;

/** Content definition shared by each selectable joint teaching experience. */
UCLASS(BlueprintType)
class GRADUATIONPROJECT_API UJointDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Joint")
	FName JointId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Joint")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Joint")
	FText SelectionDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching")
	FText ObservationDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching")
	TArray<FText> ObservationSteps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching")
	FText GuidedInstruction;

	/** Optional ordered guidance lines shown by the placeholder UI without changing assembly validation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching")
	TArray<FText> GuidedSteps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching")
	FText ChallengeInstruction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching")
	FText ResultDescription;

	/** Script used for narration generation and accessible text presentation. Falls back to ObservationDescription when empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|Observation")
	FText ObservationNarrationText;

	/** Optional voice narration played while the player explores the assembly/explosion slider. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|Observation")
	TObjectPtr<USoundBase> ObservationNarrationSound;

	/** Optional looping ambience played only during observation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|Observation")
	TObjectPtr<USoundBase> ObservationAmbientSound;

	/** The Start action is unlocked only this long after narration playback completes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|Timing", meta = (ClampMin = "0.0"))
	float ObservationPostNarrationDelaySeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|Timing", meta = (ClampMin = "0.0"))
	float HintOfferSeconds = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|Timing", meta = (ClampMin = "0.0"))
	float AutomaticHintSeconds = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|Timing", meta = (ClampMin = "0.0"))
	float CompletionFeedbackSeconds = 1.0f;

	/** Display resources; world actor bindings remain on the session manager. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources")
	TObjectPtr<UStaticMesh> TenonMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources")
	TObjectPtr<UStaticMesh> MortiseMesh;
};
