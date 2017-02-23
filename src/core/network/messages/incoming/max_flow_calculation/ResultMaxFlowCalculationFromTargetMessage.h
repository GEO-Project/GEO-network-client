//
// Created by mc on 15.02.17.
//

#ifndef GEO_NETWORK_CLIENT_RESULTMAXFLOWCALCULATIONFROMTARGETMESSAGE_H
#define GEO_NETWORK_CLIENT_RESULTMAXFLOWCALCULATIONFROMTARGETMESSAGE_H

#include "../../base/transaction/TransactionMessage.h"
#include "../../result/MessageResult.h"
#include "../../../../common/multiprecision/MultiprecisionUtils.h"

class ResultMaxFlowCalculationFromTargetMessage : public TransactionMessage {

public:
    typedef shared_ptr<ResultMaxFlowCalculationFromTargetMessage> Shared;

public:
    ResultMaxFlowCalculationFromTargetMessage(
        BytesShared buffer);

    const MessageType typeID() const;

    pair<BytesShared, size_t> serializeToBytes();

    static const size_t kRequestedBufferSize();

    static const size_t kRequestedBufferSize(unsigned char* buffer);

    map<NodeUUID, TrustLineAmount> getIncomingFlows();

private:

    void deserializeFromBytes(
        BytesShared buffer);

private:
    map<NodeUUID, TrustLineAmount> mIncomingFlows;

};


#endif //GEO_NETWORK_CLIENT_RESULTMAXFLOWCALCULATIONFROMTARGETMESSAGE_H
