#include "RejectTrustLineMessage.h"

RejectTrustLineMessage::RejectTrustLineMessage(
    NodeUUID sender,
    TransactionUUID transactionUUID,
    uint16_t resultCode) :

    Message(sender, transactionUUID) {

    mResultCode = resultCode;
}

pair<ConstBytesShared, size_t> RejectTrustLineMessage::serialize() {

    size_t dataSize = sizeof(uint16_t) +
                      NodeUUID::kUUIDSize +
                      TransactionUUID::kUUIDSize +
                      sizeof(uint16_t);
    byte * data = (byte *) malloc(dataSize);
    memset(
        data,
        0,
        dataSize
    );

    uint16_t type = typeID();
    memcpy(
        data,
        &type,
        sizeof(uint16_t)
    );

    memcpy(
        data + sizeof(uint16_t),
        mSenderUUID.data,
        NodeUUID::kUUIDSize
    );

    memcpy(
        data + sizeof(uint16_t) + NodeUUID::kUUIDSize,
        mTransactionUUID.data,
        TransactionUUID::kUUIDSize
    );

    memcpy(
        data + sizeof(uint16_t) + NodeUUID::kUUIDSize + TransactionUUID::kUUIDSize,
        &mResultCode,
        sizeof(uint16_t)
    );

    return make_pair(
        ConstBytesShared(data, free),
        dataSize
    );
}

void RejectTrustLineMessage::deserialize(byte *buffer) {

    throw NotImplementedError("RejectTrustLineMessage::deserialize: "
                                  "Method not implemented.");
}

const Message::MessageTypeID RejectTrustLineMessage::typeID() const {
    return Message::MessageTypeID::RejectTrustLineMessageType;
}

uint16_t RejectTrustLineMessage::resultCode() const {

    return mResultCode;
}
