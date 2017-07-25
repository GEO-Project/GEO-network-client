﻿#include "ReceiverPaymentTransaction.h"


ReceiverPaymentTransaction::ReceiverPaymentTransaction(
    const NodeUUID &currentNodeUUID,
    ReceiverInitPaymentRequestMessage::ConstShared message,
    TrustLinesManager *trustLines,
    StorageHandler *storageHandler,
    MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
    Logger &log,
    TestingController *testingController) :

    BasePaymentTransaction(
        BaseTransaction::ReceiverPaymentTransaction,
        message->transactionUUID(),
        currentNodeUUID,
        trustLines,
        storageHandler,
        maxFlowCalculationCacheManager,
        log,
        testingController),
    mMessage(message),
    mTotalReserved(0),
    mTransactionShouldBeRejected(false)
{
    mStep = Stages::Receiver_CoordinatorRequestApproving;
}

ReceiverPaymentTransaction::ReceiverPaymentTransaction(
    BytesShared buffer,
    const NodeUUID &nodeUUID,
    TrustLinesManager *trustLines,
    StorageHandler *storageHandler,
    MaxFlowCalculationCacheManager *maxFlowCalculationCacheManager,
    Logger &log,
    TestingController *testingController) :

    BasePaymentTransaction(
        buffer,
        nodeUUID,
        trustLines,
        storageHandler,
        maxFlowCalculationCacheManager,
        log,
        testingController)
{
    deserializeFromBytes(buffer);
}

TransactionResult::SharedConst ReceiverPaymentTransaction::run()
    noexcept
{
    try {
        debug() << "mStep: " << mStep;
        switch (mStep) {
            case Stages::Receiver_CoordinatorRequestApproving:
                return runInitialisationStage();

            case Stages::Receiver_AmountReservationsProcessing:
                return runAmountReservationStage();

            case Stages::Common_VotesChecking:
                return runVotesCheckingStageWithCoordinatorClarification();

            case Stages::Common_ClarificationTransaction:
                return runClarificationOfTransaction();

            case Stages::Common_Recovery:
                return runVotesRecoveryParentStage();


            default:
                throw RuntimeError(
                    "ReceiverPaymentTransaction::run(): "
                        "invalid stage number occurred");
        }
    } catch (Exception &e) {
        error() << e.what();
        recover("Something happens wrong in method run(). Transaction will be recovered");
    }
}

const string ReceiverPaymentTransaction::logHeader() const
{
    stringstream s;
    s << "[ReceiverPaymentTA: " << currentTransactionUUID() << "] ";
    return s.str();
}

TransactionResult::SharedConst ReceiverPaymentTransaction::runInitialisationStage()
{
    const auto kCoordinator = mMessage->senderUUID;
    debug() << "Operation for " << mMessage->amount() << " initialised by the (" << kCoordinator << ")";

    // Check if total incoming possibilities of the node are <= of the payment amount.
    // If not - there is no reason to process the operation at all.
    // (reject operation)
    const auto kTotalAvailableIncomingAmount = *(mTrustLines->totalIncomingAmount());
    if (kTotalAvailableIncomingAmount < mMessage->amount()) {
        sendMessage<ReceiverInitPaymentResponseMessage>(
            kCoordinator,
            currentNodeUUID(),
            currentTransactionUUID(),
            mMessage->pathUUID(),
            ReceiverInitPaymentResponseMessage::Rejected);

        return exitWithResult(
            resultDone(),
            "Operation rejected due to insufficient funds.");
    }

    sendMessage<ReceiverInitPaymentResponseMessage>(
        kCoordinator,
        currentNodeUUID(),
        currentTransactionUUID(),
        mMessage->pathUUID(),
        ReceiverInitPaymentResponseMessage::Accepted);

    // Begin waiting for amount reservation requests.
    // There is non-zero probability, that first couple of paths will fail.
    // So receiver will wait for time, that is approximatyle neede for several nodes for processing.
    //
    // TODO: enhancement: send aproximate paths count to receiver, so it will be able to wait correct timeout.
    mStep = Stages::Receiver_AmountReservationsProcessing;
    return resultWaitForMessageTypes(
        {Message::Payments_IntermediateNodeReservationRequest,
        Message::Payments_TTLProlongation},
        maxNetworkDelay((kMaxPathLength - 1) * 4));
}

TransactionResult::SharedConst ReceiverPaymentTransaction::runAmountReservationStage()
{
    debug() << "runAmountReservationStage";
    if (contextIsValid(Message::Payments_TTLProlongation, false)) {
        // current path was rejected and need reset delay time
        // TODO check if message sender is coordinator
        debug() << "Receive TTL prolongation message";
        clearContext();
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest,
             Message::Payments_TTLProlongation},
            maxNetworkDelay((kMaxPathLength - 1) * 4));
    }
    if (! contextIsValid(Message::Payments_IntermediateNodeReservationRequest))
        return reject("No amount reservation request was received. Rolled back.");

    const auto kMessage = popNextMessage<IntermediateNodeReservationRequestMessage>();
    const auto kNeighbor = kMessage->senderUUID;

    if (! mTrustLines->isNeighbor(kNeighbor)) {
        sendMessage<IntermediateNodeReservationResponseMessage>(
            kNeighbor,
            currentNodeUUID(),
            currentTransactionUUID(),
            kMessage->pathUUID(),
            ResponseMessage::Rejected);
        error() << "Path is not valid: previous node is not neighbor of current one. Rejected.";
        // Message was sent from node, that is not listed in neighbors list.
        //
        // TODO: enhance this check
        // Neighbor public key must be used here.

        // Ignore received message.
        // Begin waiting for another one.
        //
        // TODO: enhancement: send aproximate paths count to receiver, so it will be able to wait correct timeout.
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest,
            Message::Payments_TTLProlongation},
            maxNetworkDelay((kMaxPathLength - 1) * 4));
    }

    debug() << "Amount reservation for " << kMessage->amount() << " request received from " << kNeighbor;
    // Note: copy of shared pointer is required.
    const auto kAvailableAmount = mTrustLines->availableIncomingAmount(kNeighbor);
    if (*kAvailableAmount == TrustLine::kZeroAmount()) {
        debug() << "Available amount equals zero. Reservation reject.";
        sendMessage<IntermediateNodeReservationResponseMessage>(
            kNeighbor,
            currentNodeUUID(),
            currentTransactionUUID(),
            kMessage->pathUUID(),
            ResponseMessage::Rejected);

        // Begin accepting other reservation messages
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest,
             Message::Payments_TTLProlongation},
            maxNetworkDelay((kMaxPathLength - 1) * 4));
    }

    debug() << "Available amount " << *kAvailableAmount;
    const auto kReservationAmount = min(
        kMessage->amount(),
        *kAvailableAmount);
    if (! reserveIncomingAmount(kNeighbor, kReservationAmount, kMessage->pathUUID())) {
        // Receiver must not confirm reservation in case if requested amount is less than available.
        // Intermediate nodes may decrease requested reservation amount, but receiver must not do this.
        // It must stay synchronised with previous node.
        // So, in case if requested amount is less than available -
        // previous node must report about it to the coordinator.
        // In this case, receiver should even not receive reservation request at all.
        //
        // Also, this kind of problem may appear when two nodes are involved
        // in several COUNTER transactions.
        // In this case, balances may be reserved on the nodes,
        // but neighbor node may reject reservation,
        // because it already reserved amount or other transactions,
        // that wasn't approved by the current node yet.
        //
        // In this case, reservation request must be rejected.
        debug() << "Can't reserve incoming amount. Reservation reject.";
        sendMessage<IntermediateNodeReservationResponseMessage>(
            kNeighbor,
            currentNodeUUID(),
            currentTransactionUUID(),
            kMessage->pathUUID(),
            ResponseMessage::Rejected);

        // Begin accepting other reservation messages
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest,
            Message::Payments_TTLProlongation},
            maxNetworkDelay((kMaxPathLength - 1) * 4));
    }

    const auto kTotalTransactionAmount = mMessage->amount();
    mTotalReserved += kReservationAmount;
    if (mTotalReserved > kTotalTransactionAmount){
        sendMessage<IntermediateNodeReservationResponseMessage>(
            kNeighbor,
            currentNodeUUID(),
            currentTransactionUUID(),
            kMessage->pathUUID(),
            ResponseMessage::Closed);

        mTransactionShouldBeRejected = true;
        error() << "Reserved amount is greater than requested. It indicates protocol or realisation error.";
        // We should waiting for possible new messages and close transaction on timeout
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest,
             Message::Payments_TTLProlongation},
            maxNetworkDelay((kMaxPathLength - 1) * 4));
    }

    debug() << "Reserved locally: " << kReservationAmount;
    sendMessage<IntermediateNodeReservationResponseMessage>(
        kNeighbor,
        currentNodeUUID(),
        currentTransactionUUID(),
        kMessage->pathUUID(),
        ResponseMessage::Accepted,
        kReservationAmount);

    if (mTotalReserved == mMessage->amount()) {
        // Reserved amount is enough to move to votes processing stage.

        // TODO: receiver must know, how many paths are involved into the transaction.
        // This info helps to calculate max timeout,
        // that would be used for waiting for votes list message.

        mStep = Stages::Common_VotesChecking;
        return resultWaitForMessageTypes(
            {Message::Payments_ParticipantsVotes,
            Message::Payments_IntermediateNodeReservationRequest},
            maxNetworkDelay(kMaxPathLength - 1));

    } else {
        // Waiting for another reservation request
        return resultWaitForMessageTypes(
            {Message::Payments_IntermediateNodeReservationRequest,
            Message::Payments_TTLProlongation},
            maxNetworkDelay((kMaxPathLength - 1) * 4));
    }
}

TransactionResult::SharedConst ReceiverPaymentTransaction::runVotesCheckingStageWithCoordinatorClarification()
{
    debug() << "runVotesCheckingStageWithCoordinatorClarification";
    if (contextIsValid(Message::Payments_IntermediateNodeReservationRequest, false)) {
        // This case can happens when on previous stages Receiver reserved some amount
        // and Coordinator didn't receive approve message. Coordinator try reserve it again.
        // We should reject this transaction on voting stage
        error() << "Receiver already reserved all requested amount. "
                      "It indicates protocol error.";
        mTransactionShouldBeRejected = true;
        const auto kMessage = popNextMessage<IntermediateNodeReservationRequestMessage>();

        sendMessage<IntermediateNodeReservationResponseMessage>(
            kMessage->senderUUID,
            currentNodeUUID(),
            currentTransactionUUID(),
            kMessage->pathUUID(),
            ResponseMessage::Closed);

        return resultWaitForMessageTypes(
            {Message::Payments_ParticipantsVotes,
             Message::Payments_IntermediateNodeReservationRequest},
            maxNetworkDelay(kMaxPathLength - 1));
    }
    if (contextIsValid(Message::Payments_ParticipantsVotes, false)) {
        if (mTransactionShouldBeRejected) {
            // this case can happens only with Receiver,
            // when coordinator wants to reserve greater then command amount
            reject("Receiver rejected transaction because of discrepancy reservations with Coordinator. Rolling back.");
        }
        return runVotesCheckingStage();
    }
    debug() << "Send TTLTransaction message to coordinator " << mMessage->senderUUID;
    sendMessage<TTLPolongationMessage>(
        mMessage->senderUUID,
        currentNodeUUID(),
        currentTransactionUUID());
    mStep = Stages::Common_ClarificationTransaction;
    return resultWaitForMessageTypes(
        {Message::Payments_ParticipantsVotes,
         Message::Payments_TTLProlongation},
        maxNetworkDelay(2));
}

TransactionResult::SharedConst ReceiverPaymentTransaction::runClarificationOfTransaction()
{
    // on this stage we can also receive and ParticipantsVotes messages
    // and on this cases we process it properly
    debug() << "runClarificationOfTransaction";
    if (contextIsValid(Message::MessageType::Payments_ParticipantsVotes, false)) {
        mStep = Stages::Common_VotesChecking;
        return runVotesCheckingStage();
    }
    if (!contextIsValid(Message::MessageType::Payments_TTLProlongation)) {
        if (mTransactionIsVoted) {
            return recover("No participants votes message with all votes received.");
        } else {
            return reject("No participants votes message received. Transaction was closed. Rolling Back");
        }
    }
    // transactions is still alive and we continue waiting for messages
    clearContext();
    debug() << "Transactions is still alive. Continue waiting for messages";
    mStep = Stages::Common_VotesChecking;
    return resultWaitForMessageTypes(
        {Message::Payments_ParticipantsVotes,
         Message::Payments_IntermediateNodeReservationRequest},
        maxNetworkDelay(kMaxPathLength));
}

TransactionResult::SharedConst ReceiverPaymentTransaction::approve()
{
    BasePaymentTransaction::approve();
    runBuildThreeNodesCyclesSignal();
    return resultDone();
}

void ReceiverPaymentTransaction::savePaymentOperationIntoHistory()
{
    debug() << "savePaymentOperationIntoHistory";
    auto ioTransaction = mStorageHandler->beginTransaction();
    ioTransaction->historyStorage()->savePaymentRecord(
        make_shared<PaymentRecord>(
            currentTransactionUUID(),
            PaymentRecord::PaymentOperationType::IncomingPaymentType,
            mMessage->senderUUID,
            mMessage->amount(),
            *mTrustLines->totalBalance().get()));
}

void ReceiverPaymentTransaction::runBuildThreeNodesCyclesSignal()
{
    vector<NodeUUID> contractorsUUID;
    contractorsUUID.reserve(mReservations.size());
    for (auto const nodeUUIDAndReservations : mReservations) {
        contractorsUUID.push_back(
            nodeUUIDAndReservations.first);
    }
    mBuildCycleThreeNodesSignal(
        contractorsUUID);
}