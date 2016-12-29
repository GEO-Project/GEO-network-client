#ifndef GEO_NETWORK_CLIENT_COMMANDSRECEIVER_H
#define GEO_NETWORK_CLIENT_COMMANDSRECEIVER_H

#include "../commands/BaseUserCommand.h"
#include "../commands/OpenTrustLineCommand.h"
#include "../commands/CloseTrustLineCommand.h"
#include "../commands/UpdateTrustLineCommand.h"
#include "../commands/UseCreditCommand.h"
#include "../commands/MaximalTransactionAmountCommand.h"
#include "../commands/ContractorsListCommand.h"
#include "../commands/TotalBalanceCommand.h"
#include "../../BaseFIFOInterface.h"
#include "../../../transactions/manager/TransactionsManager.h"
#include "../../../common/exceptions/IOError.h"
#include "../../../common/exceptions/ValueError.h"
#include "../../../common/exceptions/MemoryError.h"
#include "../../../common/exceptions/RuntimeError.h"
#include "../../../logger/Logger.h"

#include <boost/bind.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <vector>
#include <chrono>
#include <exception>


#define COMMANDS_INTERFACE_DEBUG


using namespace std;
namespace uuids = boost::uuids;

/*!
 * User commands are transmitted via text protocol.
 * CommandsParser is used for parsing received user input
 * and deserialising them into commands instances.
 *
 *
 * Note: shares almost the same logic as "MessagesParser"
 * (see IncomingMessagesHandler.h for details).
 *
 * todo: HSC: review this class
 */
class CommandsParser {
    friend class CommandsParserTests;

public:
    static const size_t kUUIDHexRepresentationSize = 36;
    static const size_t kMinCommandSize = kUUIDHexRepresentationSize + 2;
    static const char kCommandsSeparator = '\n';
    static const char kTokensSeparator = '\t';

public:
    CommandsParser(Logger *log);
    void appendReadData(
        as::streambuf *buffer,
        const size_t receivedBytesCount);
    pair<bool, BaseUserCommand::Shared> processReceivedCommands();

protected:
    string mBuffer;
    Logger *mLog;


protected:
    inline pair<bool, BaseUserCommand::Shared> tryDeserializeCommand();

    inline pair<bool, BaseUserCommand::Shared> tryParseCommand(
        const CommandUUID &uuid,
        const string &identifier,
        const string &buffer);

    inline pair<bool, BaseUserCommand::Shared> commandIsInvalidOrIncomplete();

    void cutBufferUpToNextCommand();
};

/*!
 * User commands are transmitted via named pipe (FIFO on Linux).
 * This class is used to asynchronously receive them, parse,
 * and transfer for the further execution.
 */
class CommandsInterface:
    public BaseFIFOInterface {

public:
    static const constexpr char *kFIFOName = "commands.fifo";
    static const constexpr unsigned int kPermissionsMask = 0755;

public:
    explicit CommandsInterface(
        as::io_service &ioService,
        TransactionsManager *transactionsManager,
        Logger *logger);

    ~CommandsInterface();

    void beginAcceptCommands();

protected:
    static const constexpr size_t kCommandBufferSize = 1024;

protected:
    // External
    as::io_service &mIOService;
    TransactionsManager *mTransactionsManager;
    Logger *mLog;

    // Internal
    as::streambuf mCommandBuffer;
    as::posix::stream_descriptor *mFIFOStreamDescriptor;
    as::deadline_timer *mReadTimeoutTimer;
    CommandsParser *mCommandsParser;

//    vector<char> mCommandBuffer;

protected:
    virtual const char* FIFOname() const;
    void asyncReceiveNextCommand();
    void handleReceivedInfo(
        const boost::system::error_code &error,
        const size_t bytesTransferred);
    void handleTimeout(
        const boost::system::error_code &error);
};

#endif //GEO_NETWORK_CLIENT_COMMANDSRECEIVER_H
