#ifndef GEO_NETWORK_CLIENT_CLOSEINCOMINGTRUSTLINECOMMAND_H
#define GEO_NETWORK_CLIENT_CLOSEINCOMINGTRUSTLINECOMMAND_H

#include "../BaseUserCommand.h"

class CloseIncomingTrustLineCommand : public BaseUserCommand {

public:
    typedef shared_ptr<CloseIncomingTrustLineCommand> Shared;

public:
    CloseIncomingTrustLineCommand(
        const CommandUUID &commandUUID,
        const string &commandBuffer);

    static const string &identifier();

    const ContractorID contractorID() const;

    const SerializedEquivalent equivalent() const;

private:
    ContractorID mContractorID;
    SerializedEquivalent mEquivalent;
};

#endif //GEO_NETWORK_CLIENT_CLOSEINCOMINGTRUSTLINECOMMAND_H
