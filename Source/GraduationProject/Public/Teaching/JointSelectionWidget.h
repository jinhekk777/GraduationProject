#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JointSelectionWidget.generated.h"

class AAssemblySessionManager;
class UButton;
class USlider;
class UTextBlock;

/** Minimal world-space selector. Presentation only; session state remains in C++. */
UCLASS()
class GRADUATIONPROJECT_API UJointSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSessionManager(AAssemblySessionManager* InSessionManager);
	void SetOptions(const FText& FirstOption, const FText& SecondOption);
	void SetSelectionMode();
	void SetObservationMode(const FText& JointName, const FText& NarrationText);
	void SetStartGuidedAvailable(bool bAvailable);
	void SetGuidedMode(const FText& JointName, const FText& Instruction, bool bFeedbackPending);
	void SetChallengeMode(const FText& JointName, const FText& Instruction, bool bHintOfferAvailable, bool bActualHintShown);
	void SetResultMode(const FText& JointName, const FText& ResultDescription, const FText& StatsText);

	/** Starts a world-space pointer drag when the hit is on the observation slider. */
	bool BeginObservationSliderDrag(const FVector2D& WidgetLocalPosition);
	void UpdateObservationSliderDrag(const FVector2D& WidgetLocalPosition);
	void EndObservationSliderDrag();

protected:
	virtual void NativeConstruct() override;

private:
	TWeakObjectPtr<AAssemblySessionManager> SessionManager;
	FText FirstSelectionOption;
	FText SecondSelectionOption;
	UPROPERTY(Transient)
	TObjectPtr<UButton> FirstOptionButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> SecondOptionButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FirstOptionText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SecondOptionText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObservationText;
	UPROPERTY(Transient)
	TObjectPtr<USlider> ObservationSlider;
	UPROPERTY(Transient)
	TObjectPtr<UButton> StartGuidedButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StartGuidedText;
	bool bObservationSliderDragActive = false;

	void BuildLayoutIfNeeded();

	UFUNCTION()
	void SelectFirstOption();
	UFUNCTION()
	void SelectSecondOption();
	UFUNCTION()
	void StartGuidedAssembly();
	UFUNCTION()
	void HandleObservationSliderChanged(float Value);
};
