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
TArray<FWorldMessage> UKnowledgeSubsystem::GetPublicMessages() const
{
    TArray<FWorldMessage> Result;
    for (const FWorldMessage& Message : Messages)
    {
        if (!Message.RecipientId.IsValid())
        {
            Result.Add(Message);
        }
    }
    return Result;
}

bool UKnowledgeSubsystem::LearnMessage(
    const FGuid KnowerId,
    const FGuid MessageId,
    const FWorldTime LearnedAt,
    const float BeliefConfidence)
{
    if (!KnowerId.IsValid() || !MessageId.IsValid())
    {
        return false;
    }

    const bool bMessageExists = Messages.ContainsByPredicate([MessageId](const FWorldMessage& Message)
    {
        return Message.MessageId == MessageId;
    });
    if (!bMessageExists)
    {
        return false;
    }

    for (FMessageKnowledge& Record : KnowledgeRecords)
    {
        if (Record.KnowerId == KnowerId && Record.MessageId == MessageId)
        {
            Record.BeliefConfidence = FMath::Max(
                Record.BeliefConfidence,
                FMath::Clamp(BeliefConfidence, 0.0f, 1.0f));
            return true;
        }
    }

    FMessageKnowledge Record;
    Record.KnowerId = KnowerId;
    Record.MessageId = MessageId;
    Record.LearnedAt = LearnedAt;
    Record.BeliefConfidence = FMath::Clamp(BeliefConfidence, 0.0f, 1.0f);
    KnowledgeRecords.Add(Record);
    return true;
}

TArray<FWorldMessage> UKnowledgeSubsystem::GetKnownMessages(const FGuid KnowerId) const
{
    TArray<FWorldMessage> Result;
    if (!KnowerId.IsValid())
    {
        return Result;
    }

    for (const FMessageKnowledge& Record : KnowledgeRecords)
    {
        if (Record.KnowerId != KnowerId)
        {
            continue;
        }

        const FWorldMessage* Message = Messages.FindByPredicate([&Record](const FWorldMessage& Candidate)
        {
            return Candidate.MessageId == Record.MessageId;
        });
        if (Message != nullptr)
        {
            Result.Add(*Message);
        }
    }
    return Result;
}

bool UKnowledgeSubsystem::KnowsMessage(const FGuid KnowerId, const FGuid MessageId) const
{
    return KnowledgeRecords.ContainsByPredicate([KnowerId, MessageId](const FMessageKnowledge& Record)
    {
        return Record.KnowerId == KnowerId && Record.MessageId == MessageId;
    });
}

bool UKnowledgeSubsystem::TryGetMessage(const FGuid MessageId, FWorldMessage& OutMessage) const
{
    if (!MessageId.IsValid())
    {
        return false;
    }

    for (const FWorldMessage& Message : Messages)
    {
        if (Message.MessageId == MessageId)
        {
            OutMessage = Message;
            return true;
        }
    }
    return false;
}

TArray<FMessageKnowledge> UKnowledgeSubsystem::GetKnowledgeRecords(
    const FGuid KnowerId,
    const int32 MaxResults) const
{
    TArray<FMessageKnowledge> Result;
    if (!KnowerId.IsValid())
    {
        return Result;
    }

    for (const FMessageKnowledge& Record : KnowledgeRecords)
    {
        if (Record.KnowerId == KnowerId)
        {
            Result.Add(Record);
        }
    }

    Result.Sort([](const FMessageKnowledge& A, const FMessageKnowledge& B)
    {
        if (A.LearnedAt.Minute != B.LearnedAt.Minute)
        {
            return A.LearnedAt.Minute > B.LearnedAt.Minute;
        }
        return A.MessageId.ToString(EGuidFormats::Digits)
            < B.MessageId.ToString(EGuidFormats::Digits);
    });

    const int32 BoundedMaxResults = FMath::Clamp(MaxResults, 1, 1000);
    if (Result.Num() > BoundedMaxResults)
    {
        Result.SetNum(BoundedMaxResults);
    }
    return Result;
}
