#include "Teaching/AssemblyTeachingWidget.h"

#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Teaching/JointDefinition.h"
#include "Blueprint/WidgetTree.h"

namespace
{
	void AddToCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D Position, const FVector2D Size)
	{
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
	}
}

void UAssemblyTeachingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildPlaceholderLayoutIfNeeded();
	if (AdvanceButton) AdvanceButton->OnClicked.AddUniqueDynamic(this, &UAssemblyTeachingWidget::HandleAdvanceClicked);
	if (SelectNextButton) SelectNextButton->OnClicked.AddUniqueDynamic(this, &UAssemblyTeachingWidget::HandleSelectNextClicked);
	if (RetryButton) RetryButton->OnClicked.AddUniqueDynamic(this, &UAssemblyTeachingWidget::HandleRetryClicked);
	if (HintButton) HintButton->OnClicked.AddUniqueDynamic(this, &UAssemblyTeachingWidget::HandleHintClicked);
	if (AlignmentAssistButton) AlignmentAssistButton->OnClicked.AddUniqueDynamic(this, &UAssemblyTeachingWidget::HandleAlignmentAssistClicked);
	if (ExitChallengeButton) ExitChallengeButton->OnClicked.AddUniqueDynamic(this, &UAssemblyTeachingWidget::HandleExitChallengeClicked);
}

void UAssemblyTeachingWidget::BuildPlaceholderLayoutIfNeeded()
{
	if (PhaseText || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TeachingRoot"));
	WidgetTree->RootWidget = Canvas;
	auto AddText = [this, Canvas](const FName Name, const FVector2D Position, const FVector2D Size) -> UTextBlock*
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		AddToCanvas(Canvas, TextBlock, Position, Size);
		return TextBlock;
	};
	auto AddButton = [this, Canvas](const FName Name, const TCHAR* Caption, const FVector2D Position) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("%sLabel"), *Name.ToString()));
		TextBlock->SetText(FText::FromString(Caption));
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = 16;
		TextBlock->SetFont(Font);
		TextBlock->SetAutoWrapText(false);
		TextBlock->SetJustification(ETextJustify::Center);
		Button->SetContent(TextBlock);
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(TextBlock->Slot))
		{
			ContentSlot->SetHorizontalAlignment(HAlign_Center);
			ContentSlot->SetVerticalAlignment(VAlign_Center);
			ContentSlot->SetPadding(FMargin(6.0f, 2.0f));
		}
		AddToCanvas(Canvas, Button, Position, FVector2D(150.0f, 42.0f));
		return Button;
	};

	PhaseText = AddText(TEXT("PhaseText"), FVector2D(32.0f, 32.0f), FVector2D(560.0f, 32.0f));
	JointNameText = AddText(TEXT("JointNameText"), FVector2D(32.0f, 68.0f), FVector2D(560.0f, 32.0f));
	InstructionText = AddText(TEXT("InstructionText"), FVector2D(32.0f, 106.0f), FVector2D(680.0f, 128.0f));
	StatsText = AddText(TEXT("StatsText"), FVector2D(32.0f, 240.0f), FVector2D(680.0f, 28.0f));
	InputHintText = AddText(TEXT("InputHintText"), FVector2D(32.0f, 272.0f), FVector2D(680.0f, 28.0f));
	AdvanceButton = AddButton(TEXT("AdvanceButton"), TEXT("继续 / A"), FVector2D(32.0f, 314.0f));
	SelectNextButton = AddButton(TEXT("SelectNextButton"), TEXT("切换 / B"), FVector2D(190.0f, 314.0f));
	RetryButton = AddButton(TEXT("RetryButton"), TEXT("独立重试"), FVector2D(348.0f, 314.0f));
	HintButton = AddButton(TEXT("HintButton"), TEXT("提示 / B"), FVector2D(32.0f, 360.0f));
	AlignmentAssistButton = AddButton(TEXT("AlignmentAssistButton"), TEXT("对准 / A"), FVector2D(190.0f, 360.0f));
	ExitChallengeButton = AddButton(TEXT("ExitChallengeButton"), TEXT("退出挑战"), FVector2D(348.0f, 360.0f));
}

void UAssemblyTeachingWidget::SetSessionManager(AAssemblySessionManager* InSessionManager)
{
	SessionManager = InSessionManager;
	if (InSessionManager)
	{
		RefreshTeachingState(InSessionManager->GetTeachingState(), InSessionManager->GetActiveJointDefinition(), InSessionManager->GetCurrentResult(),
			false, InSessionManager->IsObservationStartUnlocked(), InSessionManager->IsHintOfferAvailable(), InSessionManager->IsActualHintVisible(), false);
	}
}

void UAssemblyTeachingWidget::RefreshTeachingState(EAssemblyTeachingState State, const UJointDefinition* Definition, const FJointTeachingResult& Result,
	bool bAwaitingAdvance, bool bObservationUnlocked, bool bHintOfferAvailable, bool bActualHintShown, bool bFeedbackPending)
{
	if (JointNameText)
	{
		JointNameText->SetText(Definition ? Definition->DisplayName : FText::FromString(TEXT("请选择榫卯")));
	}

	FText Phase;
	FText Instruction;
	FText InputHint;
	switch (State)
	{
	case EAssemblyTeachingState::JointSelection:
		Phase = FText::FromString(TEXT("选择榫卯"));
		Instruction = Definition ? Definition->SelectionDescription : FText::FromString(TEXT("请选择想学习的榫卯。"));
		InputHint = FText::FromString(TEXT("用面前的选择卡开始"));
		break;
	case EAssemblyTeachingState::StructureObservation:
		Phase = FText::FromString(TEXT("结构观察"));
		Instruction = SessionManager.IsValid() ? SessionManager->GetCurrentObservationText() : FText::GetEmpty();
		InputHint = bObservationUnlocked ? FText::FromString(TEXT("观察与讲解完成，可按开始按钮进入独立挑战")) : FText::FromString(TEXT("拖动面前滑块观察结构；讲解结束后解锁开始按钮"));
		break;
	case EAssemblyTeachingState::GuidedAssembly:
		Phase = FText::FromString(TEXT("引导装配"));
		Instruction = bFeedbackPending ? FText::FromString(TEXT("装配成功，正在保留完成反馈。"))
			: (bAwaitingAdvance ? FText::FromString(TEXT("引导装配完成，准备进入独立挑战。"))
			: (SessionManager.IsValid() ? SessionManager->GetCurrentGuidedInstruction() : FText::GetEmpty()));
		InputHint = bFeedbackPending ? FText::GetEmpty() : (bAwaitingAdvance ? FText::FromString(TEXT("A：开始独立挑战")) : FText::FromString(TEXT("A：查看下一条引导；按提示完成装配")));
		break;
	case EAssemblyTeachingState::IndependentChallenge:
		Phase = FText::FromString(TEXT("独立挑战"));
		Instruction = bActualHintShown && SessionManager.IsValid()
			? SessionManager->GetCurrentGuidedInstruction()
			: (Definition ? Definition->ChallengeInstruction : FText::GetEmpty());
		InputHint = bActualHintShown ? FText::FromString(TEXT("已显示提示。B：再次提示  A：对准辅助  菜单：退出挑战"))
			: (bHintOfferAvailable ? FText::FromString(TEXT("需要提示吗？B：显示提示（将计入结果）  A：对准辅助"))
			: FText::FromString(TEXT("B：主动请求提示  A：对准辅助  菜单：退出挑战")));
		break;
	case EAssemblyTeachingState::ResultFeedback:
		Phase = FText::FromString(TEXT("结果反馈"));
		Instruction = Definition ? Definition->ResultDescription : FText::GetEmpty();
		InputHint = FText::FromString(TEXT("A：独立重试    B：切换榫卯    菜单：返回选择"));
		break;
	default:
		break;
	}

	if (PhaseText) PhaseText->SetText(Phase);
	if (InstructionText) InstructionText->SetText(Instruction);
	if (InputHintText) InputHintText->SetText(InputHint);
	if (StatsText)
	{
		FString PerformanceText;
		switch (Result.Performance)
		{
		case EAssemblyChallengePerformance::IndependentCompletion: PerformanceText = TEXT("独立完成"); break;
		case EAssemblyChallengePerformance::SelfCorrectedCompletion: PerformanceText = TEXT("自行修正"); break;
		case EAssemblyChallengePerformance::AssistedCompletion: PerformanceText = TEXT("提示完成"); break;
		default: PerformanceText = TEXT("未完成"); break;
		}
		StatsText->SetText(State == EAssemblyTeachingState::ResultFeedback
			? FText::FromString(FString::Printf(TEXT("%s｜用时 %.1f 秒｜错误 %d｜提示 %d｜重试 %d"), *PerformanceText,
				Result.ChallengeDurationSeconds, Result.ChallengeStageStats.ErrorCount, Result.HintDisplayCount, Result.RetryCount))
			: FText::GetEmpty());
	}

	if (AdvanceButton)
	{
		const bool bShowAdvance = State == EAssemblyTeachingState::JointSelection || State == EAssemblyTeachingState::ResultFeedback
			|| (State == EAssemblyTeachingState::StructureObservation && bObservationUnlocked)
			|| (State == EAssemblyTeachingState::GuidedAssembly && !bFeedbackPending);
		AdvanceButton->SetVisibility(bShowAdvance ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (SelectNextButton)
	{
		SelectNextButton->SetVisibility((State == EAssemblyTeachingState::JointSelection || State == EAssemblyTeachingState::ResultFeedback)
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (RetryButton) RetryButton->SetVisibility(State == EAssemblyTeachingState::ResultFeedback ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (HintButton) HintButton->SetVisibility(State == EAssemblyTeachingState::IndependentChallenge ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (AlignmentAssistButton) AlignmentAssistButton->SetVisibility(State == EAssemblyTeachingState::IndependentChallenge ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ExitChallengeButton) ExitChallengeButton->SetVisibility(State == EAssemblyTeachingState::IndependentChallenge ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UAssemblyTeachingWidget::HandleAdvanceClicked()
{
	if (SessionManager.IsValid()) SessionManager->AdvanceTeaching();
}

void UAssemblyTeachingWidget::HandleSelectNextClicked()
{
	if (SessionManager.IsValid()) SessionManager->SelectNextJoint();
}

void UAssemblyTeachingWidget::HandleRetryClicked()
{
	if (SessionManager.IsValid()) SessionManager->RetryCurrentJoint();
}

void UAssemblyTeachingWidget::HandleHintClicked()
{
	if (SessionManager.IsValid()) SessionManager->RequestHint();
}

void UAssemblyTeachingWidget::HandleAlignmentAssistClicked()
{
	if (SessionManager.IsValid()) SessionManager->RequestAlignmentAssist();
}

void UAssemblyTeachingWidget::HandleExitChallengeClicked()
{
	if (SessionManager.IsValid()) SessionManager->ExitChallenge();
}
