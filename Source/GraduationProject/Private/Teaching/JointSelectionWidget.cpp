#include "Teaching/JointSelectionWidget.h"

#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Teaching/AssemblySessionManager.h"
#include "Blueprint/WidgetTree.h"

namespace
{
	constexpr float ObservationSliderLeft = 40.0f;
	constexpr float ObservationSliderTop = 360.0f;
	constexpr float ObservationSliderWidth = 680.0f;
	constexpr float ObservationSliderHeight = 48.0f;
	constexpr float ObservationSliderTouchPadding = 24.0f;

	void AddToCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D Position, const FVector2D Size)
	{
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
	}

	void ConfigureText(UTextBlock* TextBlock, int32 FontSize, bool bAutoWrap, ETextJustify::Type Justification)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		TextBlock->SetAutoWrapText(bAutoWrap);
		TextBlock->SetJustification(Justification);
	}

	void SetCanvasGeometry(UWidget* Widget, const FVector2D Position, const FVector2D Size)
	{
		if (Widget)
		{
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				Slot->SetPosition(Position);
				Slot->SetSize(Size);
			}
		}
	}
}

void UJointSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayoutIfNeeded();
	if (FirstOptionButton) FirstOptionButton->OnClicked.AddUniqueDynamic(this, &UJointSelectionWidget::SelectFirstOption);
	if (SecondOptionButton) SecondOptionButton->OnClicked.AddUniqueDynamic(this, &UJointSelectionWidget::SelectSecondOption);
	if (StartGuidedButton) StartGuidedButton->OnClicked.AddUniqueDynamic(this, &UJointSelectionWidget::StartGuidedAssembly);
	if (ObservationSlider) ObservationSlider->OnValueChanged.AddUniqueDynamic(this, &UJointSelectionWidget::HandleObservationSliderChanged);
}

void UJointSelectionWidget::SetSessionManager(AAssemblySessionManager* InSessionManager)
{
	SessionManager = InSessionManager;
}

void UJointSelectionWidget::SetOptions(const FText& FirstOption, const FText& SecondOption)
{
	BuildLayoutIfNeeded();
	FirstSelectionOption = FirstOption;
	SecondSelectionOption = SecondOption;
	if (FirstOptionText) FirstOptionText->SetText(FirstOption);
	if (SecondOptionText) SecondOptionText->SetText(SecondOption);
}

void UJointSelectionWidget::SetSelectionMode()
{
	bObservationSliderDragActive = false;
	BuildLayoutIfNeeded();
	if (TitleText) TitleText->SetText(FText::FromString(TEXT("选择想学习的榫卯")));
	if (FirstOptionText) FirstOptionText->SetText(FirstSelectionOption);
	if (SecondOptionText) SecondOptionText->SetText(SecondSelectionOption);
	SetCanvasGeometry(FirstOptionButton, FVector2D(40.0f, 130.0f), FVector2D(680.0f, 104.0f));
	SetCanvasGeometry(SecondOptionButton, FVector2D(40.0f, 274.0f), FVector2D(680.0f, 104.0f));
	SetCanvasGeometry(StartGuidedButton, FVector2D(40.0f, 440.0f), FVector2D(680.0f, 104.0f));
	if (FirstOptionButton) FirstOptionButton->SetVisibility(ESlateVisibility::Visible);
	if (SecondOptionButton) SecondOptionButton->SetVisibility(ESlateVisibility::Visible);
	if (ObservationText) ObservationText->SetVisibility(ESlateVisibility::Collapsed);
	if (ObservationSlider) ObservationSlider->SetVisibility(ESlateVisibility::Collapsed);
	if (StartGuidedButton) StartGuidedButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UJointSelectionWidget::SetObservationMode(const FText& JointName, const FText& NarrationText)
{
	bObservationSliderDragActive = false;
	BuildLayoutIfNeeded();
	if (TitleText) TitleText->SetText(FText::Format(FText::FromString(TEXT("结构观察：{0}")), JointName));
	if (FirstOptionButton) FirstOptionButton->SetVisibility(ESlateVisibility::Collapsed);
	if (SecondOptionButton) SecondOptionButton->SetVisibility(ESlateVisibility::Collapsed);
	if (ObservationText)
	{
		ObservationText->SetText(NarrationText);
		ObservationText->SetVisibility(ESlateVisibility::Visible);
	}
	SetCanvasGeometry(ObservationText, FVector2D(40.0f, 110.0f), FVector2D(680.0f, 230.0f));
	if (ObservationSlider)
	{
		ObservationSlider->SetValue(SessionManager.IsValid() ? SessionManager->GetObservationExplodeProgress() : 0.0f);
		ObservationSlider->SetVisibility(ESlateVisibility::Visible);
	}
	SetCanvasGeometry(StartGuidedButton, FVector2D(40.0f, 440.0f), FVector2D(680.0f, 104.0f));
	if (StartGuidedText) StartGuidedText->SetText(FText::FromString(TEXT("开始独立挑战")));
	if (StartGuidedButton) StartGuidedButton->SetVisibility(ESlateVisibility::Collapsed);
}

bool UJointSelectionWidget::BeginObservationSliderDrag(const FVector2D& WidgetLocalPosition)
{
	BuildLayoutIfNeeded();
	if (!ObservationSlider || ObservationSlider->GetVisibility() != ESlateVisibility::Visible)
	{
		return false;
	}

	const bool bWithinHorizontalRange = WidgetLocalPosition.X >= ObservationSliderLeft - ObservationSliderTouchPadding
		&& WidgetLocalPosition.X <= ObservationSliderLeft + ObservationSliderWidth + ObservationSliderTouchPadding;
	const bool bWithinVerticalRange = WidgetLocalPosition.Y >= ObservationSliderTop - ObservationSliderTouchPadding
		&& WidgetLocalPosition.Y <= ObservationSliderTop + ObservationSliderHeight + ObservationSliderTouchPadding;
	if (!bWithinHorizontalRange || !bWithinVerticalRange)
	{
		return false;
	}

	bObservationSliderDragActive = true;
	UpdateObservationSliderDrag(WidgetLocalPosition);
	return true;
}

void UJointSelectionWidget::UpdateObservationSliderDrag(const FVector2D& WidgetLocalPosition)
{
	if (!bObservationSliderDragActive || !ObservationSlider)
	{
		return;
	}

	const float Value = FMath::Clamp((WidgetLocalPosition.X - ObservationSliderLeft) / ObservationSliderWidth, 0.0f, 1.0f);
	ObservationSlider->SetValue(Value);
	HandleObservationSliderChanged(Value);
}

void UJointSelectionWidget::EndObservationSliderDrag()
{
	bObservationSliderDragActive = false;
}

void UJointSelectionWidget::SetStartGuidedAvailable(bool bAvailable)
{
	BuildLayoutIfNeeded();
	if (StartGuidedButton) StartGuidedButton->SetVisibility(bAvailable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (TitleText && bAvailable) TitleText->SetText(FText::FromString(TEXT("可以开始装配")));
}

void UJointSelectionWidget::SetGuidedMode(const FText& JointName, const FText& Instruction, bool bFeedbackPending)
{
	bObservationSliderDragActive = false;
	BuildLayoutIfNeeded();
	if (TitleText)
	{
		TitleText->SetText(bFeedbackPending
			? FText::FromString(TEXT("装配成功"))
			: FText::Format(FText::FromString(TEXT("引导装配：{0}")), JointName));
	}
	if (ObservationText)
	{
		ObservationText->SetText(bFeedbackPending ? FText::FromString(TEXT("完成反馈保留中，即将进入独立挑战。")) : Instruction);
		ObservationText->SetVisibility(ESlateVisibility::Visible);
	}
	if (ObservationSlider) ObservationSlider->SetVisibility(ESlateVisibility::Collapsed);
	SetCanvasGeometry(ObservationText, FVector2D(40.0f, 110.0f), FVector2D(680.0f, 230.0f));
	if (FirstOptionButton) FirstOptionButton->SetVisibility(ESlateVisibility::Collapsed);
	if (SecondOptionButton) SecondOptionButton->SetVisibility(ESlateVisibility::Collapsed);
	if (StartGuidedButton) StartGuidedButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UJointSelectionWidget::SetChallengeMode(const FText& JointName, const FText& Instruction, bool bHintOfferAvailable, bool bActualHintShown)
{
	bObservationSliderDragActive = false;
	BuildLayoutIfNeeded();
	if (TitleText) TitleText->SetText(FText::Format(FText::FromString(TEXT("独立挑战：{0}")), JointName));
	if (ObservationText)
	{
		FString Body = Instruction.ToString();
		if (bActualHintShown)
		{
			Body += TEXT("\n\n提示已显示，可继续尝试或使用对准辅助。");
		}
		else if (bHintOfferAvailable)
		{
			Body += TEXT("\n\n需要提示吗？显示实际提示后才会计入结果。");
		}
		ObservationText->SetText(FText::FromString(Body));
		ObservationText->SetVisibility(ESlateVisibility::Visible);
	}
	if (ObservationSlider) ObservationSlider->SetVisibility(ESlateVisibility::Collapsed);
	SetCanvasGeometry(ObservationText, FVector2D(40.0f, 105.0f), FVector2D(680.0f, 170.0f));
	SetCanvasGeometry(FirstOptionButton, FVector2D(40.0f, 310.0f), FVector2D(210.0f, 92.0f));
	SetCanvasGeometry(SecondOptionButton, FVector2D(275.0f, 310.0f), FVector2D(210.0f, 92.0f));
	SetCanvasGeometry(StartGuidedButton, FVector2D(510.0f, 310.0f), FVector2D(210.0f, 92.0f));
	if (FirstOptionText) FirstOptionText->SetText(FText::FromString(TEXT("显示提示")));
	if (SecondOptionText) SecondOptionText->SetText(FText::FromString(TEXT("对准辅助")));
	if (StartGuidedText) StartGuidedText->SetText(FText::FromString(TEXT("退出挑战")));
	if (FirstOptionButton) FirstOptionButton->SetVisibility(ESlateVisibility::Visible);
	if (SecondOptionButton) SecondOptionButton->SetVisibility(ESlateVisibility::Visible);
	if (StartGuidedButton) StartGuidedButton->SetVisibility(ESlateVisibility::Visible);
}

void UJointSelectionWidget::SetResultMode(const FText& JointName, const FText& ResultDescription, const FText& StatsText)
{
	bObservationSliderDragActive = false;
	BuildLayoutIfNeeded();
	if (TitleText) TitleText->SetText(FText::Format(FText::FromString(TEXT("结果：{0}")), JointName));
	if (ObservationText)
	{
		ObservationText->SetText(FText::FromString(ResultDescription.ToString() + TEXT("\n\n") + StatsText.ToString()));
		ObservationText->SetVisibility(ESlateVisibility::Visible);
	}
	if (ObservationSlider) ObservationSlider->SetVisibility(ESlateVisibility::Collapsed);
	SetCanvasGeometry(ObservationText, FVector2D(40.0f, 105.0f), FVector2D(680.0f, 170.0f));
	SetCanvasGeometry(FirstOptionButton, FVector2D(40.0f, 310.0f), FVector2D(210.0f, 92.0f));
	SetCanvasGeometry(SecondOptionButton, FVector2D(275.0f, 310.0f), FVector2D(210.0f, 92.0f));
	SetCanvasGeometry(StartGuidedButton, FVector2D(510.0f, 310.0f), FVector2D(210.0f, 92.0f));
	if (FirstOptionText) FirstOptionText->SetText(FText::FromString(TEXT("再次挑战")));
	if (SecondOptionText) SecondOptionText->SetText(FText::FromString(TEXT("切换榫卯")));
	if (StartGuidedText) StartGuidedText->SetText(FText::FromString(TEXT("返回选择")));
	if (FirstOptionButton) FirstOptionButton->SetVisibility(ESlateVisibility::Visible);
	if (SecondOptionButton) SecondOptionButton->SetVisibility(ESlateVisibility::Visible);
	if (StartGuidedButton) StartGuidedButton->SetVisibility(ESlateVisibility::Visible);
}

void UJointSelectionWidget::BuildLayoutIfNeeded()
{
	if (FirstOptionButton || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("JointSelectionRoot"));
	WidgetTree->RootWidget = Canvas;
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	TitleText->SetText(FText::FromString(TEXT("选择想学习的榫卯")));
	ConfigureText(TitleText, 38, true, ETextJustify::Center);
	AddToCanvas(Canvas, TitleText, FVector2D(40.0f, 28.0f), FVector2D(680.0f, 64.0f));
	ObservationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ObservationText"));
	ConfigureText(ObservationText, 27, true, ETextJustify::Left);
	AddToCanvas(Canvas, ObservationText, FVector2D(40.0f, 110.0f), FVector2D(680.0f, 230.0f));
	ObservationText->SetVisibility(ESlateVisibility::Collapsed);
	ObservationSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ObservationSlider"));
	AddToCanvas(Canvas, ObservationSlider, FVector2D(ObservationSliderLeft, ObservationSliderTop), FVector2D(ObservationSliderWidth, ObservationSliderHeight));
	ObservationSlider->SetToolTipText(FText::FromString(TEXT("左侧为完整结构，右侧为爆炸图")));
	ObservationSlider->SetVisibility(ESlateVisibility::Collapsed);

	auto AddOption = [this, Canvas](const FName Name, const FVector2D Position, TObjectPtr<UButton>& OutButton, TObjectPtr<UTextBlock>& OutText)
	{
		OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("%sText"), *Name.ToString()));
		ConfigureText(OutText, 26, false, ETextJustify::Center);
		OutButton->SetContent(OutText);
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(OutText->Slot))
		{
			ContentSlot->SetHorizontalAlignment(HAlign_Center);
			ContentSlot->SetVerticalAlignment(VAlign_Center);
			ContentSlot->SetPadding(FMargin(10.0f, 4.0f));
		}
		AddToCanvas(Canvas, OutButton, Position, FVector2D(680.0f, 104.0f));
	};

	AddOption(TEXT("FirstOption"), FVector2D(40.0f, 130.0f), FirstOptionButton, FirstOptionText);
	AddOption(TEXT("SecondOption"), FVector2D(40.0f, 274.0f), SecondOptionButton, SecondOptionText);
	AddOption(TEXT("StartGuided"), FVector2D(40.0f, 440.0f), StartGuidedButton, StartGuidedText);
	StartGuidedText->SetText(FText::FromString(TEXT("开始独立挑战")));
	StartGuidedButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UJointSelectionWidget::SelectFirstOption()
{
	if (!SessionManager.IsValid()) return;
	switch (SessionManager->GetTeachingState())
	{
	case EAssemblyTeachingState::IndependentChallenge: SessionManager->RequestHint(); break;
	case EAssemblyTeachingState::ResultFeedback: SessionManager->RetryCurrentJoint(); break;
	default: SessionManager->SelectJointByIndex(0); break;
	}
}

void UJointSelectionWidget::SelectSecondOption()
{
	if (!SessionManager.IsValid()) return;
	switch (SessionManager->GetTeachingState())
	{
	case EAssemblyTeachingState::IndependentChallenge: SessionManager->RequestAlignmentAssist(); break;
	case EAssemblyTeachingState::ResultFeedback: SessionManager->SelectNextJoint(); break;
	default: SessionManager->SelectJointByIndex(1); break;
	}
}

void UJointSelectionWidget::StartGuidedAssembly()
{
	if (!SessionManager.IsValid()) return;
	switch (SessionManager->GetTeachingState())
	{
	case EAssemblyTeachingState::IndependentChallenge: SessionManager->ExitChallenge(); break;
	case EAssemblyTeachingState::ResultFeedback: SessionManager->ReturnToJointSelection(); break;
	default: SessionManager->AdvanceTeaching(); break;
	}
}

void UJointSelectionWidget::HandleObservationSliderChanged(float Value)
{
	if (SessionManager.IsValid()) SessionManager->SetObservationExplodeProgress(Value);
}
