#include "Core.h"

Core::Core() {

    zeroPointers();
}

Core::~Core() {

    cleanupMemory();
}

int Core::run() {

    auto initCode = initCoreComponents();
    if (initCode != 0) {
        mLog.logFatal("Core", "Core components can't be initialised. Process will now be closed.");
        return initCode;
    }

    try {
        mCommunicator->beginAcceptMessages();
        mCommandsInterface->beginAcceptCommands();

        mLog.logSuccess("Core", "Processing started.");
        mIOService.run();
        return 0;

    } catch (const std::exception &e) {
        mLog.logException("Core", e);
        return -1;
    }
}

int Core::initCoreComponents() {

    int initCode;

    initCode = initSettings();
    if (initCode != 0)
        return initCode;

    json conf;
    try {
        // Optimised conf read.
        // (several params at once)
        // For the details - see settings realisation.
        conf = mSettings->loadParsedJSON();

    } catch (std::exception &e) {
        mLog.logException("Settings", e);
        return -1;
    }

    try {
        mNodeUUID = mSettings->nodeUUID(&conf);

    } catch (RuntimeError &) {
        mLog.logFatal("Core", "Can't read uuid of the node from the settings.");
        return -1;
    }

    initCode = initCommunicator(conf);
    if (initCode != 0)
        return initCode;

    initCode = initResultsInterface();
    if (initCode != 0)
        return initCode;

    initCode = initTrustLinesManager();
    if (initCode != 0)
        return initCode;

    initCode = initTransactionsManager();
    if (initCode != 0)
        return initCode;

    initCode = initCommandsInterface();
    if (initCode != 0)
        return initCode;

    initSlots();
    connectSignalsToSlots();

    return 0;
}

int Core::initSettings() {

    try {
        mSettings = new Settings();
        mLog.logSuccess("Core", "Settings are successfully initialised");
        return 0;

    } catch (const std::exception &e) {
        mLog.logException("Core", e);
        return -1;
    }
}

int Core::initCommunicator(
    const json &conf) {

    try {
        mCommunicator = new Communicator(
            mIOService,
            mNodeUUID,
            mSettings->interface(&conf),
            mSettings->port(&conf),
            mSettings->uuid2addressHost(&conf),
            mSettings->uuid2addressPort(&conf),
            &mLog
        );
        mLog.logSuccess("Core", "NetworkSlots communicator is successfully initialised");
        return 0;

    } catch (const std::exception &e) {
        mLog.logException("Core", e);
        return -1;
    }
}

int Core::initResultsInterface() {

    try {
        mResultsInterface = new ResultsInterface(&mLog);
        mLog.logSuccess("Core", "Results parseInterface is successfully initialised");
        return 0;

    } catch (const std::exception &e) {
        mLog.logException("Core", e);
        return -1;
    }
}

int Core::initTrustLinesManager() {

    try{
        mTrustLinesManager = new TrustLinesManager();
        mLog.logSuccess("Core", "Trust lines manager is successfully initialised");
        return 0;

    }catch(const std::exception &e) {
        mLog.logException("Core", e);
        return -1;
    }
}

int Core::initTransactionsManager() {

    try {
        mTransactionsManager = new TransactionsManager(
            mIOService,
            mCommunicator,
            mTrustLinesManager,
            mResultsInterface,
            &mLog
        );
        mLog.logSuccess("Core", "Transactions handler is successfully initialised");
        return 0;

    } catch (const std::exception &e) {
        mLog.logException("Core", e);
        return -1;
    }
}

int Core::initCommandsInterface() {

    try {
        mCommandsInterface = new CommandsInterface(
            mIOService,
            mTransactionsManager,
            &mLog
        );
        mLog.logSuccess("Core", "Commands parseInterface is successfully initialised");
        return 0;

    } catch (const std::exception &e) {
        mLog.logException("Core", e);
        return -1;
    }
}

int Core::initSlots() {

    try {
        networkSlots = new NetworkSlots(
            mTransactionsManager,
            &mLog
        );
        mLog.logSuccess("Core", "Network slot is successfully initialised");
        return 0;
    } catch (bad_alloc &e) {
        mLog.logException("Core", e);
        return -1;
    }
}

void Core::connectCommunicatorSignals() {

    //communicator's signal to transactions manager slot
    mCommunicator->messageReceivedSignal.connect(
        boost::bind(
            &Core::NetworkSlots::onMessageReceivedSlot,
            networkSlots
        )
    );
}

void Core::connectSignalsToSlots() {

    connectCommunicatorSignals();
}

void Core::cleanupMemory() {

    if (mSettings != nullptr) {
        delete mSettings;
    }

    if (mCommunicator != nullptr) {
        delete mCommunicator;
    }

    if (mResultsInterface != nullptr) {
        delete mResultsInterface;
    }

    if (mTrustLinesManager != nullptr) {
        delete mTrustLinesManager;
    }

    if (mTransactionsManager != nullptr) {
        delete mTransactionsManager;
    }

    if (mCommandsInterface != nullptr) {
        delete mCommandsInterface;
    }
}

void Core::zeroPointers() {

    mSettings = nullptr;
    mCommunicator = nullptr;
    mCommandsInterface = nullptr;
    mResultsInterface = nullptr;
    mTrustLinesManager = nullptr;
    mTransactionsManager = nullptr;
}

Core::NetworkSlots::NetworkSlots(
    TransactionsManager *manager,
    Logger *logger) :

    mTransactionsManager(manager),
    mLog(logger){}

void Core::NetworkSlots::onMessageReceivedSlot(
    Message::Shared message) {

    try {
        mTransactionsManager->processMessage(message);

    } catch(exception &e) {
        mLog->logException("Core", e);
    }
}
