#include "Teaching/AssemblySessionManager.h"

#include "Assembly/AssemblyPartComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InputComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "Sound/SoundBase.h"
#include "Teaching/AssemblyTeachingWidget.h"
#include "Teaching/JointSelectionWidget.h"
#include "Teaching/JointDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogGraduationTeaching, Log, All);

AAssemblySessionManager::AAssemblySessionManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AAssemblySessionManager::BeginPlay()
{
	Super::BeginPlay();
	BindAssemblyEvents();
	for (FJointSessionBinding& Binding : JointBindings)
	{
		SetBindingInteraction(Binding, false, false);
		SetBindingVisible(Binding, true);
	}

	CreateJointSelectionUI();
	// The world-space panel is the primary VR UI. Keep the screen-space widget as
	// a fallback only when the XR panel could not be created.
	if (!JointSelectionWidget && bCreateTeachingWidget && TeachingWidgetClass)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			TeachingWidget = CreateWidget<UAssemblyTeachingWidget>(PlayerController, TeachingWidgetClass);
			if (TeachingWidget)
			{
				TeachingWidget->AddToViewport();
				TeachingWidget->SetSessionManager(this);
			}
		}
	}

	BindControllerInput();
	SetTeachingState(EAssemblyTeachingState::JointSelection);
	UE_LOG(LogGraduationTeaching, Log, TEXT("Assembly teaching session initialized with %d joint bindings."), JointBindings.Num());
}

void AAssemblySessionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopObservationAudio();
	UnbindAssemblyEvents();
	Super::EndPlay(EndPlayReason);
}

void AAssemblySessionManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateSelectionPointerInput();
	UpdateObservationPresentation(DeltaSeconds, false);
	if (bObservationSliderDragActive && bSelectionPointerPressed && JointSelectionPointer && JointSelectionWidget)
	{
		JointSelectionWidget->UpdateObservationSliderDrag(JointSelectionPointer->Get2DHitLocation());
	}
	UpdatePointerVisibility();
	ProcessTimedTeachingEvents();
}

void AAssemblySessionManager::SelectJointByIndex(int32 JointIndex)
{
	if (!JointBindings.IsValidIndex(JointIndex) || !ResolveAssemblyPart(JointBindings[JointIndex]) || !JointBindings[JointIndex].Definition)
	{
		UE_LOG(LogGraduationTeaching, Warning, TEXT("Ignored invalid joint selection index %d."), JointIndex);
		return;
	}

	for (int32 Index = 0; Index < JointBindings.Num(); ++Index)
	{
		FJointSessionBinding& Binding = JointBindings[Index];
		const bool bIsSelected = Index == JointIndex;
		SetBindingVisible(Binding, bIsSelected);
		if (bIsSelected)
		{
			if (UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(Binding))
			{
				AssemblyPart->ResetAssembly();
			}
		}
		SetBindingInteraction(Binding, false, false);
	}

	ActiveJointIndex = JointIndex;
	if (JointSelectionWidget)
	{
		JointSelectionWidget->SetStartGuidedAvailable(false);
	}
	CurrentResult = FJointTeachingResult();
	CurrentResult.JointId = JointBindings[JointIndex].Definition->JointId;
	bAwaitingStageAdvance = false;
	bObservationStartUnlocked = false;
	bHintOfferAvailable = false;
	bActualHintShown = false;
	bPendingCompletionTransition = false;
	GuidedStepIndex = 0;
	SetTeachingState(EAssemblyTeachingState::StructureObservation);
	BeginObservationPresentation();
	UE_LOG(LogGraduationTeaching, Log, TEXT("Selected teaching joint '%s' for observation."), *CurrentResult.JointId.ToString());
}

void AAssemblySessionManager::SetObservationExplodeProgress(float ExplodeProgress)
{
	if (TeachingState != EAssemblyTeachingState::StructureObservation || !JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	ObservationExplodeProgress = FMath::Clamp(ExplodeProgress, 0.0f, 1.0f);
}

void AAssemblySessionManager::SelectNextJoint()
{
	if (JointBindings.IsEmpty())
	{
		return;
	}

	// In the initial two-joint selector, the alternate command is the second
	// listed definition (currently YanWei). Once a joint is active it cycles as
	// before, so additional content remains data ordered rather than name based.
	const int32 NextIndex = ActiveJointIndex == INDEX_NONE ? FMath::Min(1, JointBindings.Num() - 1) : (ActiveJointIndex + 1) % JointBindings.Num();
	SelectJointByIndex(NextIndex);
}

void AAssemblySessionManager::AdvanceTeaching()
{
	switch (TeachingState)
	{
	case EAssemblyTeachingState::JointSelection:
		SelectJointByIndex(ActiveJointIndex == INDEX_NONE ? 0 : ActiveJointIndex);
		break;
	case EAssemblyTeachingState::StructureObservation:
		if (bObservationStartUnlocked)
		{
			StartIndependentChallenge(false);
		}
		break;
	case EAssemblyTeachingState::GuidedAssembly:
		if (bAwaitingStageAdvance)
		{
			StartIndependentChallenge(false);
		}
		else if (!bPendingCompletionTransition)
		{
			if (const UJointDefinition* Definition = GetActiveJointDefinition())
			{
				GuidedStepIndex = FMath::Min(GuidedStepIndex + 1, FMath::Max(Definition->GuidedSteps.Num() - 1, 0));
				RefreshTeachingWidget();
			}
		}
		break;
	case EAssemblyTeachingState::ResultFeedback:
		RetryCurrentJoint();
		break;
	default:
		break;
	}
}

void AAssemblySessionManager::RetryCurrentJoint()
{
	if (TeachingState == EAssemblyTeachingState::ResultFeedback && JointBindings.IsValidIndex(ActiveJointIndex))
	{
		StartIndependentChallenge(true);
	}
}

void AAssemblySessionManager::RequestHint()
{
	if (TeachingState == EAssemblyTeachingState::IndependentChallenge && !bPendingCompletionTransition)
	{
		RecordActualHint();
	}
}

void AAssemblySessionManager::RequestAlignmentAssist()
{
	if (TeachingState != EAssemblyTeachingState::IndependentChallenge || bPendingCompletionTransition || !JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(JointBindings[ActiveJointIndex]);
	if (AssemblyPart && AssemblyPart->ApplyTeachingAlignmentAssist())
	{
		CurrentResult.bUsedAlignmentAssist = true;
		AssemblyPart->ShowTeachingHint();
		RefreshTeachingWidget();
		OnTeachingPromptChanged.Broadcast();
		UE_LOG(LogGraduationTeaching, Log, TEXT("Alignment assist used for '%s'."), *CurrentResult.JointId.ToString());
	}
}

void AAssemblySessionManager::ExitChallenge()
{
	if (TeachingState == EAssemblyTeachingState::IndependentChallenge && !bPendingCompletionTransition)
	{
		FinalizeChallenge(false, true);
	}
	else if (TeachingState == EAssemblyTeachingState::ResultFeedback)
	{
		ReturnToJointSelection();
	}
}

UJointDefinition* AAssemblySessionManager::GetActiveJointDefinition() const
{
	return JointBindings.IsValidIndex(ActiveJointIndex) ? JointBindings[ActiveJointIndex].Definition : nullptr;
}

float AAssemblySessionManager::GetCurrentStageElapsedSeconds() const
{
	return GetWorld() ? FMath::Max(0.0f, static_cast<float>(GetWorld()->GetTimeSeconds() - StateStartedAtSeconds)) : 0.0f;
}

FText AAssemblySessionManager::GetCurrentObservationText() const
{
	const UJointDefinition* Definition = GetActiveJointDefinition();
	if (!Definition)
	{
		return FText::GetEmpty();
	}

	FString Text = Definition->ObservationDescription.ToString();
	for (const FText& Step : Definition->ObservationSteps)
	{
		if (!Text.IsEmpty())
		{
			Text += TEXT("\n");
		}
		Text += Step.ToString();
	}
	return FText::FromString(Text);
}

FText AAssemblySessionManager::GetCurrentGuidedInstruction() const
{
	const UJointDefinition* Definition = GetActiveJointDefinition();
	if (!Definition)
	{
		return FText::GetEmpty();
	}

	return Definition->GuidedSteps.IsValidIndex(GuidedStepIndex) ? Definition->GuidedSteps[GuidedStepIndex] : Definition->GuidedInstruction;
}

void AAssemblySessionManager::StartGuidedAssembly()
{
	if (!JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	FJointSessionBinding& Binding = JointBindings[ActiveJointIndex];
	UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(Binding);
	if (!AssemblyPart)
	{
		return;
	}
	StopObservationAudio();
	AssemblyPart->ResetAssembly();
	AssemblyPart->SetTeachingCompletionFeedbackHoldSeconds(GetActiveJointDefinition()->CompletionFeedbackSeconds);
	SetBindingInteraction(Binding, true, true);
	StageBaselineStats = AssemblyPart->GetRunStats();
	bAwaitingStageAdvance = false;
	bHintOfferAvailable = false;
	bActualHintShown = false;
	bPendingCompletionTransition = false;
	GuidedStepIndex = 0;
	SetTeachingState(EAssemblyTeachingState::GuidedAssembly);
	UE_LOG(LogGraduationTeaching, Log, TEXT("Guided assembly started for '%s'."), *CurrentResult.JointId.ToString());
}

void AAssemblySessionManager::BeginObservationPresentation()
{
	if (!JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	ObservationExplodeProgress = 0.0f;
	ObservationAppliedExplodeProgress = 0.0f;
	bObservationNarrationFinished = false;
	ObservationNarrationFinishedAtSeconds = 0.0;
	UpdateObservationPresentation(0.0f, true);
	if (JointSelectionWidget)
	{
		const UJointDefinition* Definition = GetActiveJointDefinition();
		const FText NarrationText = Definition && !Definition->ObservationNarrationText.IsEmpty()
			? Definition->ObservationNarrationText : GetCurrentObservationText();
		JointSelectionWidget->SetObservationMode(Definition ? Definition->DisplayName : FText::GetEmpty(), NarrationText);
		JointSelectionWidget->SetStartGuidedAvailable(false);
	}
	SetJointSelectionVisible(true);
	StartObservationAudio();
}

void AAssemblySessionManager::StartObservationAudio()
{
	StopObservationAudio();
	const UJointDefinition* Definition = GetActiveJointDefinition();
	if (!Definition)
	{
		HandleObservationNarrationFinished();
		return;
	}

	if (Definition->ObservationAmbientSound)
	{
		ObservationAmbientAudio = UGameplayStatics::SpawnSound2D(this, Definition->ObservationAmbientSound);
	}
	if (Definition->ObservationNarrationSound)
	{
		ObservationNarrationAudio = UGameplayStatics::SpawnSound2D(this, Definition->ObservationNarrationSound);
		if (ObservationNarrationAudio)
		{
			ObservationNarrationAudio->OnAudioFinished.AddUniqueDynamic(this, &AAssemblySessionManager::HandleObservationNarrationFinished);
			UE_LOG(LogGraduationTeaching, Log, TEXT("Observation narration started for '%s'."), *CurrentResult.JointId.ToString());
			return;
		}
	}

	UE_LOG(LogGraduationTeaching, Warning, TEXT("No observation narration sound is configured for '%s'; using the configured post-narration delay from observation start."), *CurrentResult.JointId.ToString());
	HandleObservationNarrationFinished();
}

void AAssemblySessionManager::StopObservationAudio()
{
	if (ObservationNarrationAudio)
	{
		ObservationNarrationAudio->Stop();
		ObservationNarrationAudio->DestroyComponent();
		ObservationNarrationAudio = nullptr;
	}
	if (ObservationAmbientAudio)
	{
		ObservationAmbientAudio->Stop();
		ObservationAmbientAudio->DestroyComponent();
		ObservationAmbientAudio = nullptr;
	}
}

void AAssemblySessionManager::StartIndependentChallenge(bool bIsRetry)
{
	if (!JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	if (!bIsRetry)
	{
		CurrentResult.bChallengeCompleted = false;
		CurrentResult.bChallengeExited = false;
		CurrentResult.HintDisplayCount = 0;
		CurrentResult.RetryCount = 0;
		CurrentResult.bUsedHint = false;
		CurrentResult.bUsedAlignmentAssist = false;
		CurrentResult.ChallengeDurationSeconds = 0.0f;
		CurrentResult.ChallengeStageStats = FAssemblyRunStats();
	}
	else
	{
		++CurrentResult.RetryCount;
		CurrentResult.bChallengeExited = false;
	}

	StopObservationAudio();
	FJointSessionBinding& Binding = JointBindings[ActiveJointIndex];
	UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(Binding);
	if (!AssemblyPart)
	{
		return;
	}
	AssemblyPart->ResetAssembly();
	AssemblyPart->SetTeachingCompletionFeedbackHoldSeconds(GetActiveJointDefinition()->CompletionFeedbackSeconds);
	SetBindingInteraction(Binding, true, false);
	StageBaselineStats = AssemblyPart->GetRunStats();
	ChallengeAttemptStartedAtSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	bAwaitingStageAdvance = false;
	bHintOfferAvailable = false;
	bActualHintShown = false;
	bPendingCompletionTransition = false;
	GuidedStepIndex = 0;
	SetTeachingState(EAssemblyTeachingState::IndependentChallenge);
	UE_LOG(LogGraduationTeaching, Log, TEXT("Independent challenge %s for '%s'."), bIsRetry ? TEXT("retried") : TEXT("started"), *CurrentResult.JointId.ToString());
}

void AAssemblySessionManager::ReturnToJointSelection()
{
	StopObservationAudio();
	for (FJointSessionBinding& Binding : JointBindings)
	{
		SetBindingInteraction(Binding, false, false);
		SetBindingVisible(Binding, true);
	}
	ActiveJointIndex = INDEX_NONE;
	bAwaitingStageAdvance = false;
	bObservationStartUnlocked = false;
	bHintOfferAvailable = false;
	bActualHintShown = false;
	bPendingCompletionTransition = false;
	if (JointSelectionWidget)
	{
		JointSelectionWidget->SetSelectionMode();
	}
	SetJointSelectionVisible(true);
	SetTeachingState(EAssemblyTeachingState::JointSelection);
}

void AAssemblySessionManager::SetTeachingState(EAssemblyTeachingState NewState)
{
	TeachingState = NewState;
	StateStartedAtSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	OnTeachingStateChanged.Broadcast(TeachingState, GetActiveJointDefinition());
	RefreshTeachingWidget();
}

void AAssemblySessionManager::SetBindingVisible(FJointSessionBinding& Binding, bool bVisible)
{
	for (AActor* Actor : Binding.PresentationActors)
	{
		if (Actor)
		{
			Actor->SetActorHiddenInGame(!bVisible);
			Actor->SetActorEnableCollision(bVisible);
		}
	}
}

void AAssemblySessionManager::SetBindingInteraction(FJointSessionBinding& Binding, bool bEnabled, bool bGuidanceEnabled)
{
	if (UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(Binding))
	{
		AssemblyPart->SetGuidanceEnabled(bGuidanceEnabled);
		AssemblyPart->SetAssemblyInteractionEnabled(bEnabled);
	}
}

void AAssemblySessionManager::CreateJointSelectionUI()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UCameraComponent* Camera = PlayerPawn ? PlayerPawn->FindComponentByClass<UCameraComponent>() : nullptr;
	if (!PlayerPawn || !Camera)
	{
		UE_LOG(LogGraduationTeaching, Warning, TEXT("Joint selection UI could not find the XR pawn camera."));
		return;
	}

	JointSelectionWidgetComponent = NewObject<UWidgetComponent>(this, TEXT("JointSelectionWidgetComponent"));
	JointSelectionWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	JointSelectionWidgetComponent->SetDrawSize(FVector2D(760.0f, 580.0f));
	JointSelectionWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	JointSelectionWidgetComponent->SetTwoSided(true);
	JointSelectionWidgetComponent->SetWorldScale3D(FVector(0.15f));
	JointSelectionWidgetComponent->SetWidgetClass(UJointSelectionWidget::StaticClass());
	JointSelectionWidgetComponent->SetupAttachment(Camera);
	JointSelectionWidgetComponent->SetRelativeLocation(FVector(130.0f, 0.0f, 0.0f));
	JointSelectionWidgetComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	JointSelectionWidgetComponent->RegisterComponent();
	JointSelectionWidgetComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	JointSelectionWidget = Cast<UJointSelectionWidget>(JointSelectionWidgetComponent->GetUserWidgetObject());
	if (JointSelectionWidget)
	{
		JointSelectionWidget->SetSessionManager(this);
		const FText FirstOption = JointBindings.IsValidIndex(0) && JointBindings[0].Definition ? JointBindings[0].Definition->DisplayName : FText::FromString(TEXT("榫卯一"));
		const FText SecondOption = JointBindings.IsValidIndex(1) && JointBindings[1].Definition ? JointBindings[1].Definition->DisplayName : FText::FromString(TEXT("榫卯二"));
		JointSelectionWidget->SetOptions(FirstOption, SecondOption);
		JointSelectionWidget->SetSelectionMode();
	}

	// The XR template already owns a right-hand Widget Interaction Component with
	// the pointer identity registered by OpenXR. Reuse it instead of creating a
	// second PointerIndex=0 component that conflicts with the template's left hand.
	TArray<UWidgetInteractionComponent*> ExistingPointers;
	PlayerPawn->GetComponents(ExistingPointers);
	for (UWidgetInteractionComponent* ExistingPointer : ExistingPointers)
	{
		if (ExistingPointer && ExistingPointer->GetName().Contains(TEXT("Right"), ESearchCase::IgnoreCase))
		{
			JointSelectionPointer = ExistingPointer;
			break;
		}
	}

	if (!JointSelectionPointer)
	{
		TArray<UMotionControllerComponent*> MotionControllers;
		PlayerPawn->GetComponents(MotionControllers);
		for (UMotionControllerComponent* MotionController : MotionControllers)
		{
			if (MotionController && MotionController->GetTrackingMotionSource().ToString().Equals(TEXT("RightAim"), ESearchCase::IgnoreCase))
			{
				RightAimPointerSource = MotionController;
				break;
			}
		}
		if (!RightAimPointerSource)
		{
			RightAimPointerSource = NewObject<UMotionControllerComponent>(PlayerPawn, TEXT("RightAimPointerSource"));
			RightAimPointerSource->SetTrackingMotionSource(TEXT("RightAim"));
			RightAimPointerSource->SetupAttachment(PlayerPawn->GetRootComponent());
			RightAimPointerSource->RegisterComponent();
		}

		JointSelectionPointer = NewObject<UWidgetInteractionComponent>(PlayerPawn, TEXT("JointSelectionPointer"));
		JointSelectionPointer->PointerIndex = 7;
		JointSelectionPointer->SetupAttachment(RightAimPointerSource);
		JointSelectionPointer->RegisterComponent();
		UE_LOG(LogGraduationTeaching, Warning, TEXT("XR template right Widget Interaction Component was unavailable; using isolated fallback PointerIndex=7."));
	}
	else
	{
		UE_LOG(LogGraduationTeaching, Log, TEXT("Reusing XR template right Widget Interaction Component '%s' (PointerIndex=%d)."),
			*JointSelectionPointer->GetName(), JointSelectionPointer->PointerIndex);
	}

	JointSelectionPointer->InteractionSource = EWidgetInteractionSource::World;
	JointSelectionPointer->TraceChannel = ECC_Visibility;
	JointSelectionPointer->InteractionDistance = 1000.0f;
	JointSelectionPointer->bEnableHitTesting = true;
	JointSelectionPointer->SetActive(true);
	JointSelectionPointer->Activate(true);
	JointSelectionPointer->bShowDebug = false;
	JointSelectionPointer->DebugColor = FLinearColor::White;
	JointSelectionPointer->DebugLineThickness = 0.25f;
	JointSelectionPointer->DebugSphereLineThickness = 0.0f;
	UE_LOG(LogGraduationTeaching, Log, TEXT("World-space joint selector and right-hand UI pointer initialized."));
}

void AAssemblySessionManager::SetJointSelectionVisible(bool bVisible)
{
	if (JointSelectionWidgetComponent)
	{
		JointSelectionWidgetComponent->SetVisibility(bVisible, true);
		JointSelectionWidgetComponent->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

UAssemblyPartComponent* AAssemblySessionManager::ResolveAssemblyPart(const FJointSessionBinding& Binding) const
{
	if (Binding.AssemblyPart)
	{
		return Binding.AssemblyPart;
	}

	return Binding.AssemblyActor ? Binding.AssemblyActor->FindComponentByClass<UAssemblyPartComponent>() : nullptr;
}

void AAssemblySessionManager::HandleAssemblyCompleted()
{
	if (!JointBindings.IsValidIndex(ActiveJointIndex) || bPendingCompletionTransition)
	{
		return;
	}

	FJointSessionBinding& Binding = JointBindings[ActiveJointIndex];
	UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(Binding);
	if (!AssemblyPart || AssemblyPart->GetAssemblyState() != EAssemblyPartState::Completed)
	{
		return;
	}

	if (TeachingState == EAssemblyTeachingState::GuidedAssembly)
	{
		CaptureStageStats(CurrentResult.GuidedStageStats);
		CurrentResult.bGuidedAssemblyCompleted = true;
		bPendingCompletionTransition = true;
		bPendingCompletionWasChallenge = false;
		PendingTransitionAtSeconds = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) + (GetActiveJointDefinition() ? GetActiveJointDefinition()->CompletionFeedbackSeconds : 1.0f);
		RefreshTeachingWidget();
		UE_LOG(LogGraduationTeaching, Log, TEXT("Guided assembly complete; preserving feedback before transition."));
		return;
	}

	if (TeachingState == EAssemblyTeachingState::IndependentChallenge)
	{
		bPendingCompletionTransition = true;
		bPendingCompletionWasChallenge = true;
		PendingTransitionAtSeconds = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) + (GetActiveJointDefinition() ? GetActiveJointDefinition()->CompletionFeedbackSeconds : 1.0f);
		RefreshTeachingWidget();
		UE_LOG(LogGraduationTeaching, Log, TEXT("Independent challenge complete; preserving feedback before results."));
	}
}

void AAssemblySessionManager::BindAssemblyEvents()
{
	for (FJointSessionBinding& Binding : JointBindings)
	{
		if (UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(Binding))
		{
			AssemblyPart->OnAssemblyCompleted.AddUniqueDynamic(this, &AAssemblySessionManager::HandleAssemblyCompleted);
		}
	}
}

void AAssemblySessionManager::UnbindAssemblyEvents()
{
	for (FJointSessionBinding& Binding : JointBindings)
	{
		if (UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(Binding))
		{
			AssemblyPart->OnAssemblyCompleted.RemoveDynamic(this, &AAssemblySessionManager::HandleAssemblyCompleted);
		}
	}
}

void AAssemblySessionManager::CaptureStageStats(FAssemblyRunStats& OutStageStats) const
{
	if (!JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	const UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(JointBindings[ActiveJointIndex]);
	if (!AssemblyPart)
	{
		return;
	}

	const FAssemblyRunStats CurrentStats = AssemblyPart->GetRunStats();
	OutStageStats.GuidanceEntryCount = CurrentStats.GuidanceEntryCount - StageBaselineStats.GuidanceEntryCount;
	OutStageStats.HintCount = CurrentStats.HintCount - StageBaselineStats.HintCount;
	OutStageStats.ErrorCount = CurrentStats.ErrorCount - StageBaselineStats.ErrorCount;
	OutStageStats.CompletionCount = CurrentStats.CompletionCount - StageBaselineStats.CompletionCount;
	OutStageStats.ResetCount = CurrentStats.ResetCount - StageBaselineStats.ResetCount;
	OutStageStats.LatestCompletionSeconds = CurrentStats.LatestCompletionSeconds;
}

void AAssemblySessionManager::FinalizeChallenge(bool bCompleted, bool bExited)
{
	if (!JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	FAssemblyRunStats AttemptStats;
	CaptureStageStats(AttemptStats);
	CurrentResult.ChallengeStageStats.GuidanceEntryCount += AttemptStats.GuidanceEntryCount;
	CurrentResult.ChallengeStageStats.ErrorCount += AttemptStats.ErrorCount;
	CurrentResult.ChallengeStageStats.CompletionCount += AttemptStats.CompletionCount;
	CurrentResult.ChallengeStageStats.ResetCount += AttemptStats.ResetCount;
	CurrentResult.ChallengeStageStats.HintCount = CurrentResult.HintDisplayCount;
	CurrentResult.ChallengeDurationSeconds += GetWorld() ? FMath::Max(0.0f, static_cast<float>(GetWorld()->GetTimeSeconds() - ChallengeAttemptStartedAtSeconds)) : 0.0f;
	CurrentResult.ChallengeStageStats.LatestCompletionSeconds = CurrentResult.ChallengeDurationSeconds;
	CurrentResult.bChallengeCompleted = bCompleted;
	CurrentResult.bChallengeExited = bExited;
	UpdatePerformanceLabel();

	SetBindingInteraction(JointBindings[ActiveJointIndex], false, false);
	bPendingCompletionTransition = false;
	SetTeachingState(EAssemblyTeachingState::ResultFeedback);
	OnResultReady.Broadcast(CurrentResult);
	UE_LOG(LogGraduationTeaching, Log, TEXT("Challenge finalized for '%s': completed=%s exited=%s label=%d."), *CurrentResult.JointId.ToString(),
		bCompleted ? TEXT("true") : TEXT("false"), bExited ? TEXT("true") : TEXT("false"), static_cast<int32>(CurrentResult.Performance));
}

void AAssemblySessionManager::ProcessTimedTeachingEvents()
{
	const UJointDefinition* Definition = GetActiveJointDefinition();
	if (!Definition || !GetWorld())
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (bPendingCompletionTransition && Now >= PendingTransitionAtSeconds)
	{
		ProcessCompletedAssemblyAfterFeedback();
		return;
	}

	if (TeachingState == EAssemblyTeachingState::StructureObservation && !bObservationStartUnlocked
		&& bObservationNarrationFinished
		&& Now >= ObservationNarrationFinishedAtSeconds + Definition->ObservationPostNarrationDelaySeconds)
	{
		bObservationStartUnlocked = true;
		if (JointSelectionWidget)
		{
			JointSelectionWidget->SetStartGuidedAvailable(true);
		}
		SetJointSelectionVisible(true);
		RefreshTeachingWidget();
		OnTeachingPromptChanged.Broadcast();
		UE_LOG(LogGraduationTeaching, Log, TEXT("Observation start unlocked for '%s'."), *CurrentResult.JointId.ToString());
	}

	if (TeachingState == EAssemblyTeachingState::IndependentChallenge)
	{
		const float Elapsed = GetCurrentStageElapsedSeconds();
		if (!bHintOfferAvailable && Elapsed >= Definition->HintOfferSeconds)
		{
			bHintOfferAvailable = true;
			RefreshTeachingWidget();
			OnTeachingPromptChanged.Broadcast();
			UE_LOG(LogGraduationTeaching, Log, TEXT("Hint offer shown for '%s' without incrementing hint count."), *CurrentResult.JointId.ToString());
		}
		if (!bActualHintShown && Elapsed >= Definition->AutomaticHintSeconds)
		{
			RecordActualHint();
		}
	}
}

void AAssemblySessionManager::UpdateObservationPresentation(float DeltaSeconds, bool bApplyImmediately)
{
	if (TeachingState != EAssemblyTeachingState::StructureObservation || !JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	const float NewAppliedProgress = bApplyImmediately
		? ObservationExplodeProgress
		: FMath::FInterpTo(ObservationAppliedExplodeProgress, ObservationExplodeProgress, DeltaSeconds, 18.0f);
	if (!bApplyImmediately && FMath::IsNearlyEqual(NewAppliedProgress, ObservationAppliedExplodeProgress, KINDA_SMALL_NUMBER))
	{
		return;
	}

	if (UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(JointBindings[ActiveJointIndex]))
	{
		ObservationAppliedExplodeProgress = NewAppliedProgress;
		AssemblyPart->SetTeachingObservationExplodeProgress(ObservationAppliedExplodeProgress);
	}
}

void AAssemblySessionManager::HandleObservationNarrationFinished()
{
	if (TeachingState != EAssemblyTeachingState::StructureObservation || bObservationNarrationFinished)
	{
		return;
	}

	bObservationNarrationFinished = true;
	ObservationNarrationFinishedAtSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	UE_LOG(LogGraduationTeaching, Log, TEXT("Observation narration completed for '%s'; start unlock delay has begun."), *CurrentResult.JointId.ToString());
}

void AAssemblySessionManager::UpdatePointerVisibility()
{
	if (JointSelectionPointer)
	{
		const bool bShowPointer = JointSelectionWidgetComponent && JointSelectionWidgetComponent->IsVisible()
			&& (bSelectionPointerPressed || JointSelectionPointer->IsOverHitTestVisibleWidget());
		JointSelectionPointer->bShowDebug = bShowPointer;
	}
}

void AAssemblySessionManager::ProcessCompletedAssemblyAfterFeedback()
{
	if (!JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	if (bPendingCompletionWasChallenge)
	{
		FinalizeChallenge(true, false);
		return;
	}

	bPendingCompletionTransition = false;
	StartIndependentChallenge(false);
	UE_LOG(LogGraduationTeaching, Log, TEXT("Guided assembly feedback complete; independent challenge started automatically."));
}

void AAssemblySessionManager::RecordActualHint()
{
	if (!JointBindings.IsValidIndex(ActiveJointIndex))
	{
		return;
	}

	if (UAssemblyPartComponent* AssemblyPart = ResolveAssemblyPart(JointBindings[ActiveJointIndex]); AssemblyPart && AssemblyPart->ShowTeachingHint())
	{
		if (bActualHintShown)
		{
			if (const UJointDefinition* Definition = GetActiveJointDefinition(); Definition && !Definition->GuidedSteps.IsEmpty())
			{
				GuidedStepIndex = FMath::Min(GuidedStepIndex + 1, Definition->GuidedSteps.Num() - 1);
			}
		}
		++CurrentResult.HintDisplayCount;
		CurrentResult.bUsedHint = true;
		bActualHintShown = true;
		RefreshTeachingWidget();
		OnTeachingPromptChanged.Broadcast();
		UE_LOG(LogGraduationTeaching, Log, TEXT("Actual teaching hint displayed for '%s'. Hints=%d."), *CurrentResult.JointId.ToString(), CurrentResult.HintDisplayCount);
	}
}

void AAssemblySessionManager::UpdatePerformanceLabel()
{
	if (!CurrentResult.bChallengeCompleted)
	{
		CurrentResult.Performance = EAssemblyChallengePerformance::Incomplete;
	}
	else if (CurrentResult.bUsedHint || CurrentResult.bUsedAlignmentAssist)
	{
		CurrentResult.Performance = EAssemblyChallengePerformance::AssistedCompletion;
	}
	else if (CurrentResult.ChallengeStageStats.ErrorCount > 0 || CurrentResult.RetryCount > 0)
	{
		CurrentResult.Performance = EAssemblyChallengePerformance::SelfCorrectedCompletion;
	}
	else
	{
		CurrentResult.Performance = EAssemblyChallengePerformance::IndependentCompletion;
	}
}

void AAssemblySessionManager::RefreshTeachingWidget()
{
	if (TeachingWidget)
	{
		TeachingWidget->RefreshTeachingState(TeachingState, GetActiveJointDefinition(), CurrentResult, bAwaitingStageAdvance,
			bObservationStartUnlocked, bHintOfferAvailable, bActualHintShown, bPendingCompletionTransition);
	}

	if (!JointSelectionWidget)
	{
		return;
	}

	const UJointDefinition* Definition = GetActiveJointDefinition();
	const FText JointName = Definition ? Definition->DisplayName : FText::GetEmpty();
	switch (TeachingState)
	{
	case EAssemblyTeachingState::JointSelection:
		JointSelectionWidget->SetSelectionMode();
		break;
	case EAssemblyTeachingState::StructureObservation:
		JointSelectionWidget->SetObservationMode(JointName,
			Definition && !Definition->ObservationNarrationText.IsEmpty() ? Definition->ObservationNarrationText : GetCurrentObservationText());
		JointSelectionWidget->SetStartGuidedAvailable(bObservationStartUnlocked);
		break;
	case EAssemblyTeachingState::GuidedAssembly:
		JointSelectionWidget->SetGuidedMode(JointName, GetCurrentGuidedInstruction(), bPendingCompletionTransition);
		break;
	case EAssemblyTeachingState::IndependentChallenge:
		JointSelectionWidget->SetChallengeMode(JointName,
			bActualHintShown ? GetCurrentGuidedInstruction() : (Definition ? Definition->ChallengeInstruction : FText::GetEmpty()),
			bHintOfferAvailable, bActualHintShown);
		break;
	case EAssemblyTeachingState::ResultFeedback:
	{
		FString PerformanceText;
		switch (CurrentResult.Performance)
		{
		case EAssemblyChallengePerformance::IndependentCompletion: PerformanceText = TEXT("独立完成"); break;
		case EAssemblyChallengePerformance::SelfCorrectedCompletion: PerformanceText = TEXT("自行修正"); break;
		case EAssemblyChallengePerformance::AssistedCompletion: PerformanceText = TEXT("提示完成"); break;
		default: PerformanceText = TEXT("本次挑战未完成"); break;
		}
		const FText StatsText = FText::FromString(FString::Printf(TEXT("%s｜用时 %.1f 秒｜错误 %d｜提示 %d｜重试 %d｜辅助 %s"),
			*PerformanceText, CurrentResult.ChallengeDurationSeconds, CurrentResult.ChallengeStageStats.ErrorCount,
			CurrentResult.HintDisplayCount, CurrentResult.RetryCount, CurrentResult.bUsedAlignmentAssist ? TEXT("是") : TEXT("否")));
		JointSelectionWidget->SetResultMode(JointName, Definition ? Definition->ResultDescription : FText::GetEmpty(), StatsText);
		break;
	}
	default:
		break;
	}
	SetJointSelectionVisible(true);
}

void AAssemblySessionManager::BindControllerInput()
{
	// The XR template owns the pawn's Enhanced Input component.  Keep asset
	// assignments optional: a session placed without editor-authored input assets
	// still receives the teaching controls on a Quest right controller.
	const bool bUseRuntimeTeachingInput = !TeachingInputMappingContext;
	if (bUseRuntimeTeachingInput)
	{
		TeachingInputMappingContext = NewObject<UInputMappingContext>(this, TEXT("RuntimeTeachingInputContext"));
	}
	if (!TeachingAdvanceAction)
	{
		TeachingAdvanceAction = NewObject<UInputAction>(this, TEXT("RuntimeTeachingAdvance"));
	}
	if (!TeachingAlternateAction)
	{
		TeachingAlternateAction = NewObject<UInputAction>(this, TEXT("RuntimeTeachingAlternate"));
	}
	if (bUseRuntimeTeachingInput && TeachingInputMappingContext)
	{
		TeachingInputMappingContext->MapKey(TeachingAdvanceAction, EKeys::OculusTouch_Right_A_Click);
		TeachingInputMappingContext->MapKey(TeachingAdvanceAction, EKeys::Gamepad_FaceButton_Bottom);
		TeachingInputMappingContext->MapKey(TeachingAdvanceAction, EKeys::A);
		TeachingInputMappingContext->MapKey(TeachingAlternateAction, EKeys::OculusTouch_Right_B_Click);
		TeachingInputMappingContext->MapKey(TeachingAlternateAction, EKeys::OculusTouch_Right_Thumbstick_Click);
		TeachingInputMappingContext->MapKey(TeachingAlternateAction, EKeys::B);
		UE_LOG(LogGraduationTeaching, Log, TEXT("Created runtime Enhanced Input mapping for Quest right A/B teaching controls."));
	}

	// IMC_Menu is registered in DefaultInput.ini before the OpenXR session starts.
	// Runtime-created controller actions are not part of that OpenXR action set.
	TemplateMenuInputMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/XRFramework/Input/IMC_Menu.IMC_Menu"));
	TemplateMenuInteractAction = LoadObject<UInputAction>(nullptr,
		TEXT("/Game/XRFramework/Input/Actions/IA_Menu_Interact_Right_Pressed.IA_Menu_Interact_Right_Pressed"));
	UInputAction* TemplateMenuInteractReleasedAction = LoadObject<UInputAction>(nullptr,
		TEXT("/Game/XRFramework/Input/Actions/IA_Menu_Interact_Right_Released.IA_Menu_Interact_Right_Released"));

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (TeachingInputMappingContext)
				{
					InputSubsystem->AddMappingContext(TeachingInputMappingContext, 100);
				}
				else
				{
					UE_LOG(LogGraduationTeaching, Warning, TEXT("Teaching Enhanced Input mapping context is not configured."));
				}

				if (TemplateMenuInputMappingContext)
				{
					InputSubsystem->AddMappingContext(TemplateMenuInputMappingContext, 150);
					UE_LOG(LogGraduationTeaching, Log, TEXT("Registered XR template IMC_Menu for the teaching pointer."));
				}
				else
				{
					UE_LOG(LogGraduationTeaching, Error, TEXT("XR template IMC_Menu could not be loaded; teaching pointer input is unavailable."));
				}
			}
		}

		if (APawn* PlayerPawn = PlayerController->GetPawn())
		{
			if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerPawn->InputComponent))
			{
				if (TeachingAdvanceAction)
				{
					EnhancedInput->BindAction(TeachingAdvanceAction, ETriggerEvent::Started, this, &AAssemblySessionManager::HandleAdvanceInput);
				}
				if (TeachingAlternateAction)
				{
					EnhancedInput->BindAction(TeachingAlternateAction, ETriggerEvent::Started, this, &AAssemblySessionManager::HandleSelectNextInput);
				}
				if (TemplateMenuInteractAction)
				{
					// The template uses separate edge actions for pointer press and release.
					// Keeping the pointer captured between them gives widgets a real drag.
					EnhancedInput->BindAction(TemplateMenuInteractAction, ETriggerEvent::Triggered, this, &AAssemblySessionManager::HandleSelectionPointerPressed);
				}
				if (TemplateMenuInteractReleasedAction)
				{
					EnhancedInput->BindAction(TemplateMenuInteractReleasedAction, ETriggerEvent::Triggered, this, &AAssemblySessionManager::HandleSelectionPointerReleased);
				}
				UE_LOG(LogGraduationTeaching, Log, TEXT("Teaching Enhanced Input actions bound on XR pawn."));
			}
			else
			{
				UE_LOG(LogGraduationTeaching, Warning, TEXT("XR pawn has no Enhanced Input component for teaching actions."));
			}
		}

		EnableInput(PlayerController);
		if (InputComponent)
		{
			InputComponent->Priority = 200;
			InputComponent->bBlockInput = false;
			// Controller A/B are handled by Enhanced Input above. Binding them a
			// second time here would advance twice and make a selection appear inert.
			InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AAssemblySessionManager::HandleAdvanceInput);
			InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AAssemblySessionManager::HandleSelectNextInput);
			InputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &AAssemblySessionManager::RequestHint);
			InputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AAssemblySessionManager::RequestAlignmentAssist);
			InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &AAssemblySessionManager::ExitChallenge);
			UE_LOG(LogGraduationTeaching, Log, TEXT("Teaching input bound: Oculus Touch right A/B and generic gamepad fallbacks."));
		}
		else
		{
			UE_LOG(LogGraduationTeaching, Warning, TEXT("Teaching input could not be created for the player controller."));
		}
	}
	else
	{
		UE_LOG(LogGraduationTeaching, Warning, TEXT("Teaching input could not find player controller 0."));
	}
}

void AAssemblySessionManager::UpdateSelectionPointerInput()
{
	// Input edges are delivered by the template's mapped press/release actions.
	// Only release here as a safety net when the pointer UI is hidden mid-drag.
	if (bSelectionPointerPressed
		&& (!JointSelectionPointer || !JointSelectionWidgetComponent || !JointSelectionWidgetComponent->IsVisible()))
	{
		HandleSelectionPointerReleased();
	}
}

void AAssemblySessionManager::HandleAdvanceInput()
{
	UE_LOG(LogGraduationTeaching, Log, TEXT("Teaching advance input received in state %d."), static_cast<int32>(TeachingState));
	if (TeachingState == EAssemblyTeachingState::IndependentChallenge)
	{
		RequestAlignmentAssist();
	}
	else
	{
		AdvanceTeaching();
	}
}

void AAssemblySessionManager::HandleSelectNextInput()
{
	UE_LOG(LogGraduationTeaching, Log, TEXT("Teaching alternate input received in state %d."), static_cast<int32>(TeachingState));
	if (TeachingState == EAssemblyTeachingState::IndependentChallenge)
	{
		RequestHint();
	}
	else if (TeachingState == EAssemblyTeachingState::JointSelection || TeachingState == EAssemblyTeachingState::ResultFeedback)
	{
		SelectNextJoint();
	}
}

void AAssemblySessionManager::HandleSelectionPointerPressed()
{
	const bool bCanUseSelector = TeachingState != EAssemblyTeachingState::GuidedAssembly;
	if (bCanUseSelector && JointSelectionPointer && JointSelectionWidgetComponent
		&& JointSelectionWidgetComponent->IsVisible() && !bSelectionPointerPressed)
	{
		SetSelectionPointerPressedVisual(true);
		bObservationSliderDragActive = TeachingState == EAssemblyTeachingState::StructureObservation && JointSelectionWidget
			&& JointSelectionPointer->IsOverHitTestVisibleWidget()
			&& JointSelectionWidget->BeginObservationSliderDrag(JointSelectionPointer->Get2DHitLocation());
		if (!bObservationSliderDragActive)
		{
			JointSelectionPointer->PressPointerKey(EKeys::LeftMouseButton);
		}
		UE_LOG(LogGraduationTeaching, Log, TEXT("Joint selector pointer pressed (slider drag=%s)."), bObservationSliderDragActive ? TEXT("true") : TEXT("false"));
	}
}

void AAssemblySessionManager::HandleSelectionPointerReleased()
{
	if (!bSelectionPointerPressed)
	{
		return;
	}

	if (bObservationSliderDragActive && JointSelectionWidget)
	{
		JointSelectionWidget->EndObservationSliderDrag();
	}
	else if (bSelectionPointerPressed && JointSelectionPointer)
	{
		JointSelectionPointer->ReleasePointerKey(EKeys::LeftMouseButton);
	}
	bObservationSliderDragActive = false;
	SetSelectionPointerPressedVisual(false);
	UE_LOG(LogGraduationTeaching, Log, TEXT("Joint selector pointer released."));
}

void AAssemblySessionManager::SetSelectionPointerPressedVisual(bool bPressed)
{
	bSelectionPointerPressed = bPressed;
	if (JointSelectionPointer)
	{
		JointSelectionPointer->DebugColor = bPressed
			? FLinearColor(0.08f, 0.43f, 1.0f, 1.0f)
			: FLinearColor::White;
	}
}
