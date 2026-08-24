#include "KnowledgeSubsystem.h"

FGuid UKnowledgeSubsystem::PublishMessage(FWorldMessage Message)
{
    if (!Message.MessageId.IsValid())
    {
        Message.MessageId = FGuid::NewGuid();
    }
    Message.Confidence = FMath::Clamp(Message.Confidence, 0.0f, 1.0f);
    Messages.Add(Message);
    return Message.MessageId;
}

TArray<FWorldMessage> UKnowledgeSubsystem::GetMessagesForRecipient(const FGuid RecipientId) const
{
    TArray<FWorldMessage> Result;
    for (const FWorldMessage& Message : Messages)
    {
        if (Message.RecipientId == RecipientId)
        {
            Result.Add(Message);
        }
    }
    return Result;
}

TArray<FWorldMessage> UKnowledgeSubsystem::GetMessagesAboutEvent(const FGuid RelatedEventId) const
{
    TArray<FWorldMessage> Result;
    for (const FWorldMessage& Message : Messages)
    {
        if (Message.RelatedEventId == RelatedEventId)
        {
            Result.Add(Message);
        }
    }
    return Result;
}
