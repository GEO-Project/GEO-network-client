#ifndef GEO_NETWORK_CLIENT_COORDINATORCYCLERESERVATIONREQUESTMESSAGE_H
#define GEO_NETWORK_CLIENT_COORDINATORCYCLERESERVATIONREQUESTMESSAGE_H

#include "base/RequestCycleMessage.h"

class CoordinatorCycleReservationRequestMessage :
    public RequestCycleMessage{

public:
    typedef shared_ptr<CoordinatorCycleReservationRequestMessage> Shared;
    typedef shared_ptr<const CoordinatorCycleReservationRequestMessage> ConstShared;

public:
    CoordinatorCycleReservationRequestMessage(
        const NodeUUID& senderUUID,
        const TransactionUUID& transactionUUID,
        const TrustLineAmount& amount,
        const NodeUUID& nextNodeInThePath,
        uint8_t cucleLength);

    CoordinatorCycleReservationRequestMessage(
        BytesShared buffer);

    const NodeUUID& nextNodeInPathUUID() const;

    const Message::MessageType typeID() const;

    uint8_t cycleLength() const;

    virtual pair<BytesShared, size_t> serializeToBytes() const
    throw(bad_alloc);

protected:
    NodeUUID mNextPathNode;
    uint8_t mCycleLength;
};


#endif //GEO_NETWORK_CLIENT_COORDINATORCYCLERESERVATIONREQUESTMESSAGE_H
