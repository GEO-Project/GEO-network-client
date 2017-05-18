#ifndef GEO_NETWORK_CLIENT_CYCLECLOSERINTERMEDIATENODETRANSACTION_H
#define GEO_NETWORK_CLIENT_CYCLECLOSERINTERMEDIATENODETRANSACTION_H

#include "base/BasePaymentTransaction.h"

class CycleCloserIntermediateNodeTransaction : public BasePaymentTransaction {

public:
    typedef shared_ptr<CycleCloserIntermediateNodeTransaction> Shared;
    typedef shared_ptr<const CycleCloserIntermediateNodeTransaction> ConstShared;

public:
    CycleCloserIntermediateNodeTransaction(
        const NodeUUID &currentNodeUUID,
        IntermediateNodeCycleReservationRequestMessage::ConstShared message,
        TrustLinesManager *trustLines,
        StorageHandler *storageHandler,
        MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
        Logger *log);

    CycleCloserIntermediateNodeTransaction(
        BytesShared buffer,
        const NodeUUID &nodeUUID,
        TrustLinesManager* trustLines,
        StorageHandler *storageHandler,
        MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
        Logger* log);

    TransactionResult::SharedConst run()
    noexcept;

    pair<BytesShared, size_t> serializeToBytes();

protected:
    TransactionResult::SharedConst runPreviousNeighborRequestProcessingStage();
    TransactionResult::SharedConst runCoordinatorRequestProcessingStage();
    TransactionResult::SharedConst runNextNeighborResponseProcessingStage();
    TransactionResult::SharedConst runReservationProlongationStage();
    TransactionResult::SharedConst runFinalPathConfigurationProcessingStage();

protected:
    void deserializeFromBytes(
        BytesShared buffer);

    const string logHeader() const;

protected:
    IntermediateNodeCycleReservationRequestMessage::ConstShared mMessage;

    TrustLineAmount mLastReservedAmount;
    NodeUUID mCoordinator;
    uint8_t mCycleLength;
};


#endif //GEO_NETWORK_CLIENT_CYCLECLOSERINTERMEDIATENODETRANSACTION_H
