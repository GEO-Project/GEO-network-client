﻿#ifndef GEO_NETWORK_CLIENT_BASETRANSACTION_H
#define GEO_NETWORK_CLIENT_BASETRANSACTION_H

#include "TransactionUUID.h"

#include "../../../common/Types.h"
#include "../../../common/NodeUUID.h"
#include "../../../common/memory/MemoryUtils.h"
#include "../../../logger/FileLogger.h"

#include "../../../logger/Logger.h"

#include "../../../network/messages/Message.hpp"
#include "../../../db/uuid_map_block_storage/UUIDMapBlockStorage.h"

#include "../result/TransactionResult.h"
#include "../../../interface/results_interface/result/CommandResult.h"
#include "../../../network/messages/result/MessageResult.h"
#include "../result/state/TransactionState.h"

#include <boost/signals2.hpp>

#include <vector>
#include <deque>
#include <utility>
#include <cstdint>
#include <sstream>


namespace storage = db::uuid_map_block_storage;
namespace signals = boost::signals2;

class BaseTransaction {
public:
    typedef shared_ptr<BaseTransaction> Shared;
    typedef uint16_t SerializedTransactionType;
    typedef signals::signal<void(Message::Shared, const NodeUUID&)> SendMessageSignal;
    typedef signals::signal<void(BaseTransaction::Shared)> LaunchSubsidiaryTransactionSignal;

public:
    // TODO: add othe states shortcuts here
    TransactionResult::Shared resultExit();
    TransactionResult::Shared resultFlushAndContinue();
    TransactionResult::Shared resultWaitForMessageTypes(
        vector<Message::MessageTypeID> &&requiredMessagesTypes,
        uint16_t noLongerThanMilliseconds);

public:
    ~BaseTransaction() = default;

    enum TransactionType {
        OpenTrustLineTransactionType = 1,
        AcceptTrustLineTransactionType,
        SetTrustLineTransactionType,
        UpdateTrustLineTransactionType,
        CloseTrustLineTransactionType,
        RejectTrustLineTransactionType,
        PropagationRoutingTablesTransactionType,
        AcceptRoutingTablesTransactionType,
        RoutingTablesUpdatesFactoryTransactionType,
        PropagateRoutingTablesUpdatesTransactionType,
        AcceptRoutingTablesUpdatesTransactionType,
//        Cycles
        GetTopologyAndBalancesTransaction,
        SixNodesTopologyTransaction,
        FiveNodesTopologyTransaction,
        GetThreeNodesNeighborBalancesTransaction,
        GetFourNodesNeighborBalancesTransaction,
        // Payments
        CoordinatorPaymentTransaction,
        ReceiverPaymentTransaction,
        IntermediateNodePaymentTransaction,

        // Max flow calculation
        InitiateMaxFlowCalculationTransactionType,
        ReceiveMaxFlowCalculationOnTargetTransactionType,
        MaxFlowCalculationSourceFstLevelTransactionType,
        MaxFlowCalculationTargetFstLevelTransactionType,
        MaxFlowCalculationSourceSndLevelTransactionType,
        MaxFlowCalculationTargetSndLevelTransactionType,
        ReceiveResultMaxFlowCalculationTransactionType,
        MaxFlowCalculationCacheUpdateTransactionType,

        // TotalBalances
        TotalBalancesTransactionType,
        InitiateTotalBalancesFromRemoutNodeTransactionType,

        // History
        HistoryPaymentsTransactionType,
        HistoryTrustLinesTransactionType

    };

public:
    virtual const TransactionType transactionType() const;

    const TransactionUUID &UUID() const;

    const NodeUUID &nodeUUID() const;

    void pushContext(
        Message::Shared message);

    virtual pair<BytesShared, size_t> serializeToBytes() const;

    virtual TransactionResult::SharedConst run() = 0;

protected:
    // TODO: make logger REQUIRED
    BaseTransaction(
        const TransactionType type,
        Logger *log=nullptr);

    // TODO: make logger REQUIRED
    BaseTransaction(
        const TransactionType type,
        const TransactionUUID &transactionUUID,
        Logger *log=nullptr);

    [[deprecated("Use constructor with currentNodeUUID instead.")]]
    BaseTransaction(
        const TransactionType type,
        const NodeUUID &nodeUUID,
        Logger *log=nullptr);

    // TODO: make logger REQUIRED
    BaseTransaction(
        const TransactionType type,
        const TransactionUUID &transactionUUID,
        const NodeUUID &nodeUUID,
        Logger *log=nullptr);

    [[deprecated("Use sendMessage() instead.")]]
    void addMessage(
        Message::Shared message,
        const NodeUUID &nodeUUID);

    // TODO: convert to hpp?
    template <typename ContextMessageType>
    inline shared_ptr<ContextMessageType> popNextMessage()
    {
        const auto message = static_pointer_cast<ContextMessageType>(mContext.front());
        mContext.pop_front();
        return message;
    }

    // TODO: convert to hpp?
    template <typename MessageType, typename... Args>
    inline void sendMessage(
        const NodeUUID &addressee,
        Args&&... args)
    {
        const auto message = make_shared<MessageType>(args...);
        outgoingMessageIsReadySignal(
            message,
            addressee);
    }

    void launchSubsidiaryTransaction(
      BaseTransaction::Shared transaction);

    [[deprecated("Use stages enum instead. See payment operations as example")]]
    void increaseStepsCounter();

    [[deprecated("Use stages enum instead. See payment operations as example")]]
    void resetStepsCounter();

    void setExpectationResponsesCounter(
        uint16_t count);

    void resetExpectationResponsesCounter();

    void clearContext();

    virtual void deserializeFromBytes(
        BytesShared buffer);

    static const size_t kOffsetToInheritedBytes();

    TransactionResult::SharedConst transactionResultFromCommand(
        CommandResult::SharedConst result);

    TransactionResult::SharedConst transactionResultFromMessage(
        MessageResult::SharedConst result);

    TransactionResult::SharedConst transactionResultFromState(
        TransactionState::SharedConst state);

    TransactionResult::SharedConst finishTransaction();

    virtual const string logHeader() const;
    LoggerStream info() const;
    LoggerStream error() const;
    LoggerStream debug() const;

public:
    mutable SendMessageSignal outgoingMessageIsReadySignal;
    mutable LaunchSubsidiaryTransactionSignal runSubsidiaryTransactionSignal;

protected:
    TransactionType mType;
    TransactionUUID mTransactionUUID;
    NodeUUID mNodeUUID;

    uint16_t mExpectationResponsesCount = 0;
    deque<Message::Shared> mContext;

    uint16_t mStep = 1;

protected:
    Logger *mLog;

    [[deprecated("Use logger instead")]]
    unique_ptr<FileLogger> mFileLogger;
};


#endif //GEO_NETWORK_CLIENT_BASETRANSACTION_H
