#include "KeyframeExtensionDetail.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Extensions/KeyframeExtension.h"
#include "Engine/SCS_Node.h"
#include "ActorSequenceComponent.h"

#define LOCTEXT_NAMESPACE "KeyframeExtensionDetail"

TSharedRef<IDetailCustomization> FKeyframeExtensionDetail::MakeInstance()
{
    return MakeShared<FKeyframeExtensionDetail>();
}

void FKeyframeExtensionDetail::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray< TWeakObjectPtr<UObject> > Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);
    if (Objects.Num() != 1)
    {
        return;
    }

    AECameraBase* Camera = (AECameraBase*)Objects[0].Get();
    IDetailCategoryBuilder& KeyframeCategory = DetailBuilder.EditCategory("ECamera Actions", FText(), ECategoryPriority::Important);

    KeyframeCategory.AddCustomRow(FText::GetEmpty())
        .NameContent()
        [
            SNew(STextBlock)
            .Text(FText::FromString("Keyframe Extension"))
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        .ValueContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .Padding(0)
            .AutoWidth()
            [
                SNew(SButton)
                .ToolTipText(FText::FromString("Procedurally randomize current sequence. Twist the parameters in KeyframExtension to guide generation."))
                .Content()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Toss Sequence"))
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                ]
                .IsEnabled_Lambda([this, Camera]()->bool
                {
                    return true;
                })
                .OnClicked_Lambda([this, Camera]()
                {
                    if (Camera)
                    {
                        UKeyframeExtension* KeyframeExtension = GetKeyframeExtension(Camera);
                        UActorSequenceComponent* ActorSequenceComponent = GetActorSequenceComponent(Camera);

                        if (Camera->IsTemplate())
                        {
                            KeyframeExtension->TossSequence(ActorSequenceComponent);
                        }
                        else
                        {
                            KeyframeExtension->TossSequence();
                        }
                    }
                    return (FReply::Handled());
                })
            ]
        ];

    KeyframeCategory.AddCustomRow(FText::GetEmpty())
        .NameContent()
        [
            SNew(STextBlock)
            .Text(FText::FromString("Keyframe Extension"))
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        .ValueContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .Padding(0)
            .AutoWidth()
            [
                SNew(SButton)
                .ToolTipText(FText::FromString("Recover the original sequence."))
                .Content()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Recover"))
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                ]
                .IsEnabled_Lambda([this, Camera]()->bool
                {
                    return true;
                })
                .OnClicked_Lambda([this, Camera]()
                {
                    if (Camera)
                    {
                        UKeyframeExtension* KeyframeExtension = GetKeyframeExtension(Camera);
                        UActorSequenceComponent* ActorSequenceComponent = GetActorSequenceComponent(Camera);
                        
                        if (Camera->IsTemplate())
                        {
                            KeyframeExtension->Recover(ActorSequenceComponent);
                        }
                        else
                        {
                            KeyframeExtension->Recover();
                        }
                    }
                    return (FReply::Handled());
                })
            ]
        ];

    KeyframeCategory.AddCustomRow(FText::GetEmpty())
        .NameContent()
        [
            SNew(STextBlock)
            .Text(FText::FromString("Keyframe Extension"))
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        .ValueContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .Padding(0)
            .AutoWidth()
            [
                SNew(SButton)
                .ToolTipText(FText::FromString("Save the current sequence."))
                .Content()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Save"))
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                ]
                .IsEnabled_Lambda([this, Camera]()->bool
                {
                    return true;
                })
                .OnClicked_Lambda([this, Camera]()
                {
                    if (Camera)
                    {
                        UKeyframeExtension* KeyframeExtension = GetKeyframeExtension(Camera);
                        UActorSequenceComponent* ActorSequenceComponent = GetActorSequenceComponent(Camera);

                        if (KeyframeExtension)
                        {
                            if (Camera->IsTemplate())
                            {
                                KeyframeExtension->Save(ActorSequenceComponent);
                            }
                            else
                            {
                                KeyframeExtension->Save();
                            }
                        }
                    }
                    return (FReply::Handled());
                })
            ]
        ];
}

UActorSequenceComponent* FKeyframeExtensionDetail::GetActorSequenceComponent(AECameraBase* Camera)
{
    UActorSequenceComponent* ActorSequenceComponent = nullptr;
    if (Camera)
    {
        UBlueprintGeneratedClass* CurrentBPClass = Cast<UBlueprintGeneratedClass>(Camera->GetClass());
        if (CurrentBPClass && CurrentBPClass->SimpleConstructionScript)
        {
            for (const USCS_Node* Component : CurrentBPClass->SimpleConstructionScript->GetAllNodes())
            {
                UActorSequenceComponent* TemporaryComponent = Cast<UActorSequenceComponent>(Component->ComponentTemplate);
                if (TemporaryComponent != nullptr)
                {
                    ActorSequenceComponent = TemporaryComponent;
                    break;
                }
            }
        }
    }
    return ActorSequenceComponent;
}

UKeyframeExtension* FKeyframeExtensionDetail::GetKeyframeExtension(AECameraBase* Camera)
{
    UKeyframeExtension* KeyframeExtension = nullptr;
    TArray<UECameraExtensionBase*> Extensions = Camera->GetSettingsComponent()->GetExtensions();
    for (UECameraExtensionBase* Extension : Extensions)
    {
        if (Extension->IsA<UKeyframeExtension>())
        {
            KeyframeExtension = (UKeyframeExtension*)Extension;
            break;
        }
    }
    return KeyframeExtension;
}

#undef LOCTEXT_NAMESPACE