#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Teaching/AssemblySessionManager.h"
#include "AssemblyTeachingWidget.generated.h"

class AAssemblySessionManager;
class UButton;
class UJointDefinition;
class UTextBlock;

/** Presentation-only UMG controller. State transitions remain in AAssemblySessionManager. */
UCLASS(Abstract)
class GRADUATIONPROJECT_API UAssemblyTeachingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSessionManager(AAssemblySessionManager* InSessionManager);
	void RefreshTeachingState(EAssemblyTeachingState State, const UJointDefinition* Definition, const FJointTeachingResult& Result,
		bool bAwaitingAdvance, bool bObservationUnlocked, bool bHintOfferAvailable, bool bActualHintShown, bool bFeedbackPending);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> JointNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InstructionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InputHintText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AdvanceButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SelectNextButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RetryButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HintButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AlignmentAssistButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ExitChallengeButton;

private:
	TWeakObjectPtr<AAssemblySessionManager> SessionManager;

	void BuildPlaceholderLayoutIfNeeded();

	UFUNCTION()
	void HandleAdvanceClicked();

	UFUNCTION()
	void HandleSelectNextClicked();

	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleHintClicked();

	UFUNCTION()
	void HandleAlignmentAssistClicked();

	UFUNCTION()
	void HandleExitChallengeClicked();
};
