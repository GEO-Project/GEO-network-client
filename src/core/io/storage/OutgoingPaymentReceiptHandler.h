#ifndef GEO_NETWORK_CLIENT_OUTGOINGPAYMENTRECEIPTHANDLER_H
#define GEO_NETWORK_CLIENT_OUTGOINGPAYMENTRECEIPTHANDLER_H

#include "../../logger/Logger.h"
#include "../../common/exceptions/IOError.h"
#include "../../transactions/transactions/base/TransactionUUID.h"
#include "../../common/multiprecision/MultiprecisionUtils.h"
#include "../../crypto/lamportscheme.h"

#include "../../../libs/sqlite3/sqlite3.h"

using namespace crypto::lamport;

class OutgoingPaymentReceiptHandler {
public:
    OutgoingPaymentReceiptHandler(
        sqlite3 *dbConnection,
        const string &tableName,
        Logger &logger);

    void saveRecord(
        const TrustLineID trustLineID,
        const AuditNumber auditNumber,
        const TransactionUUID &transactionUUID,
        const uint32_t ownPublicKeyHash,
        const TrustLineAmount &amount,
        const Signature::Shared ownSignature);

private:
    LoggerStream info() const;

    LoggerStream warning() const;

    const string logHeader() const;

private:
    sqlite3 *mDataBase = nullptr;
    string mTableName;
    Logger &mLog;
};


#endif //GEO_NETWORK_CLIENT_OUTGOINGPAYMENTRECEIPTHANDLER_H
