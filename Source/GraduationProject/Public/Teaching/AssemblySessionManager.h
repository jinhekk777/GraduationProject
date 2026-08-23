#pragma once

#include "CoreMinimal.h"
#include "Assembly/AssemblyTypes.h"
#include "GameFramework/Actor.h"
#include "AssemblySessionManager.generated.h"

class UAssemblyPartComponent;
class UAssemblyTeachingWidget;
class UAudioComponent;
class UJointSelectionWidget;
class UInputAction;
class UInputMappingContext;
class UJointDefinition;
class UMotionControllerComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;

UENUM(BlueprintType)
enum class EAssemblyTeachingState : uint8
{
	JointSelection,
	StructureObservation,
	GuidedAssembly,
	IndependentChallenge,
	ResultFeedback
};

UENUM(BlueprintType)
enum class EAssemblyChallengePerformance : uint8
{
	IndependentCompletion,
	SelfCorrectedCompletion,
	AssistedCompletion,
	Incomplete
};

USTRUCT(BlueprintType)
struct FJointSessionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Session")
	TObjectPtr<UJointDefinition> Definition;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Session", meta = (UseComponentPicker))
	TObjectPtr<UAssemblyPartComponent> AssemblyPart;

	/** Owner actor for the movable tenon. Used to resolve its assembly component if a component reference is not serialized by the level. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Session")
	TObjectPtr<AActor> AssemblyActor;

	/** Actors shown only while this joint is selected. Include both movable and fixed pieces. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Session")
	TArray<TObjectPtr<AActor>> PresentationActors;
};

USTRUCT(BlueprintType)
struct FJointTeachingResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	FName JointId;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bGuidedAssemblyCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bChallengeCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bChallengeExited = false;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 HintDisplayCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 RetryCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bUsedHint = false;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bUsedAlignmentAssist = false;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	float ChallengeDurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	EAssemblyChallengePerformance Performance = EAssemblyChallengePerformance::Incomplete;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	FAssemblyRunStats GuidedStageStats;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	FAssemblyRunStats ChallengeStageStats;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAssemblyTeachingStateChanged, EAssemblyTeachingState, NewState, UJointDefinition*, ActiveJoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAssemblyTeachingResultReady, FJointTeachingResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAssemblyTeachingPromptChanged);

/** C++ owner for the selected-joint teaching state machine and two-stage performance results. */
UCLASS(Blueprintable)
class GRADUATIONPROJECT_API AAssemblySessionManager : public AActor
{
	GENERATED_BODY()

public:
	AAssemblySessionManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Teaching")
	void SelectJointByIndex(int32 JointIndex);

	UFUNCTION(BlueprintCallable, Category = "Teaching")
	void SelectNextJoint();

	UFUNCTION(BlueprintCallable, Category = "Teaching")
	void AdvanceTeaching();

	/** 0 keeps the selected joint assembled; 1 restores its authored exploded/reset pose. */
	UFUNCTION(BlueprintCallable, Category = "Teaching|Observation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	void SetObservationExplodeProgress(float ExplodeProgress);

	UFUNCTION(BlueprintPure, Category = "Teaching|Observation")
	float GetObservationExplodeProgress() const { return ObservationExplodeProgress; }

	UFUNCTION(BlueprintCallable, Category = "Teaching")
	void RetryCurrentJoint();

	UFUNCTION(BlueprintCallable, Category = "Teaching|Challenge")
	void RequestHint();

	UFUNCTION(BlueprintCallable, Category = "Teaching|Challenge")
	void RequestAlignmentAssist();

	UFUNCTION(BlueprintCallable, Category = "Teaching|Challenge")
	void ExitChallenge();

	UFUNCTION(BlueprintCallable, Category = "Teaching")
	void ReturnToJointSelection();

	UFUNCTION(BlueprintPure, Category = "Teaching")
	EAssemblyTeachingState GetTeachingState() const { return TeachingState; }

	UFUNCTION(BlueprintPure, Category = "Teaching")
	UJointDefinition* GetActiveJointDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Teaching")
	FJointTeachingResult GetCurrentResult() const { return CurrentResult; }

	UFUNCTION(BlueprintPure, Category = "Teaching")
	bool IsObservationStartUnlocked() const { return bObservationStartUnlocked; }

	UFUNCTION(BlueprintPure, Category = "Teaching|Challenge")
	bool IsHintOfferAvailable() const { return bHintOfferAvailable; }

	UFUNCTION(BlueprintPure, Category = "Teaching|Challenge")
	bool IsActualHintVisible() const { return bActualHintShown; }

	UFUNCTION(BlueprintPure, Category = "Teaching")
	float GetCurrentStageElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Teaching")
	FText GetCurrentObservationText() const;

	UFUNCTION(BlueprintPure, Category = "Teaching")
	FText GetCurrentGuidedInstruction() const;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Teaching")
	TArray<FJointSessionBinding> JointBindings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|UI")
	TSubclassOf<UAssemblyTeachingWidget> TeachingWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teaching|UI")
	bool bCreateTeachingWidget = true;

	/** Dedicated Enhanced Input mapping context for the teaching flow. Content config supplies controller bindings. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Teaching|Input")
	TObjectPtr<UInputMappingContext> TeachingInputMappingContext;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Teaching|Input")
	TObjectPtr<UInputAction> TeachingAdvanceAction;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Teaching|Input")
	TObjectPtr<UInputAction> TeachingAlternateAction;

	UPROPERTY(BlueprintAssignable, Category = "Teaching|Events")
	FAssemblyTeachingStateChanged OnTeachingStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Teaching|Events")
	FAssemblyTeachingResultReady OnResultReady;

	UPROPERTY(BlueprintAssignable, Category = "Teaching|Events")
	FAssemblyTeachingPromptChanged OnTeachingPromptChanged;

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teaching", meta = (AllowPrivateAccess = "true"))
	EAssemblyTeachingState TeachingState = EAssemblyTeachingState::JointSelection;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Teaching", meta = (AllowPrivateAccess = "true"))
	int32 ActiveJointIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<UAssemblyTeachingWidget> TeachingWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> JointSelectionWidgetComponent;

	UPROPERTY(Transient)
	TObjectPtr<UJointSelectionWidget> JointSelectionWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetInteractionComponent> JointSelectionPointer;

	/** OpenXR aim pose used by the selector ray; grip pose does not point along the controller's UI ray. */
	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> RightAimPointerSource;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> TemplateMenuInputMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> TemplateMenuInteractAction;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ObservationNarrationAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ObservationAmbientAudio;

	FAssemblyRunStats StageBaselineStats;
	FJointTeachingResult CurrentResult;
	bool bAwaitingStageAdvance = false;
	bool bObservationStartUnlocked = false;
	bool bHintOfferAvailable = false;
	bool bActualHintShown = false;
	bool bPendingCompletionTransition = false;
	bool bPendingCompletionWasChallenge = false;
	bool bObservationNarrationFinished = false;
	int32 GuidedStepIndex = 0;
	float ObservationExplodeProgress = 0.0f;
	float ObservationAppliedExplodeProgress = 0.0f;
	bool bSelectionPointerPressed = false;
	bool bObservationSliderDragActive = false;
	double StateStartedAtSeconds = 0.0;
	double ObservationNarrationFinishedAtSeconds = 0.0;
	double ChallengeAttemptStartedAtSeconds = 0.0;
	double PendingTransitionAtSeconds = 0.0;

	void StartGuidedAssembly();
	void BeginObservationPresentation();
	void StartObservationAudio();
	void StopObservationAudio();
	void StartIndependentChallenge(bool bIsRetry);
	void SetTeachingState(EAssemblyTeachingState NewState);
	void SetBindingVisible(FJointSessionBinding& Binding, bool bVisible);
	void SetBindingInteraction(FJointSessionBinding& Binding, bool bEnabled, bool bGuidanceEnabled);
	void CreateJointSelectionUI();
	void SetJointSelectionVisible(bool bVisible);
	UAssemblyPartComponent* ResolveAssemblyPart(const FJointSessionBinding& Binding) const;

	UFUNCTION()
	void HandleAssemblyCompleted();

	void BindAssemblyEvents();
	void UnbindAssemblyEvents();
	void CaptureStageStats(FAssemblyRunStats& OutStageStats) const;
	void FinalizeChallenge(bool bCompleted, bool bExited);
	void ProcessTimedTeachingEvents();
	void UpdateObservationPresentation(float DeltaSeconds, bool bApplyImmediately);
	void ProcessCompletedAssemblyAfterFeedback();
	void RecordActualHint();
	void UpdatePerformanceLabel();
	void RefreshTeachingWidget();
	void BindControllerInput();
	void HandleAdvanceInput();
	void HandleSelectNextInput();
	void HandleSelectionPointerPressed();
	void HandleSelectionPointerReleased();
	void UpdateSelectionPointerInput();
	void UpdatePointerVisibility();
	void SetSelectionPointerPressedVisual(bool bPressed);

	UFUNCTION()
	void HandleObservationNarrationFinished();
};
