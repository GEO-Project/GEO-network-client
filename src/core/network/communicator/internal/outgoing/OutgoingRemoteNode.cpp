#include "OutgoingRemoteNode.h"


OutgoingRemoteNode::OutgoingRemoteNode(
    const NodeUUID &remoteNodeUUID,
    UUID2Address &uuid2addressService,
    UDPSocket &socket,
    IOService &ioService,
    Logger &logger)
    noexcept :

    mRemoteNodeUUID(remoteNodeUUID),
    mUUID2AddressService(uuid2addressService),
    mIOService(ioService),
    mSocket(socket),
    mLog(logger),
    mNextAvailableChannelIndex(0),
    mPacketsSendingTimeoutTimer(ioService)
{}

void OutgoingRemoteNode::sendMessage(
    Message::Shared message)
    noexcept
{
    try {
        // In case if queue already contains packets -
        // then async handler is already scheduled.
        // Otherwise - it must be initialised.
        auto packetsSendingAlreadyScheduled = mPacketsQueue.size() > 0;

        auto bytesAndBytesCount = message->serializeToBytes();
        populateQueueWithNewPackets(
            bytesAndBytesCount.first.get(),
            bytesAndBytesCount.second);

        if (not packetsSendingAlreadyScheduled) {
            beginPacketsSending();
        }

    } catch (exception &e) {
        mLog.error("OutgoingRemoteNode::sendMessage")
            << "Exception occured: "
            << e.what();
    }
}

bool OutgoingRemoteNode::containsPacketsInQueue() const
{
    return mPacketsQueue.size() > 0;
}

uint32_t OutgoingRemoteNode::crc32Checksum(
    byte *data,
    size_t bytesCount) const
    noexcept
{
    boost::crc_32_type result;
    result.process_bytes(data, bytesCount);
    return result.checksum();
}

void OutgoingRemoteNode::populateQueueWithNewPackets(
    byte *messageData,
    const size_t messageBytesCount)
{
    const auto kMessageWithCRC32BytesCount = messageBytesCount + sizeof(uint32_t);

    PacketHeader::TotalPacketsCount kTotalPacketsCount =
        static_cast<PacketHeader::TotalPacketsCount>(
            kMessageWithCRC32BytesCount / Packet::kMaxSize);

    if (kMessageWithCRC32BytesCount % Packet::kMaxSize != 0)
        kTotalPacketsCount += 1;


    PacketHeader::ChannelIndex channelIndex = nextChannelIndex();
    uint32_t crcChecksum = crc32Checksum(
        messageData,
        messageBytesCount);


    size_t processedBytesCount = 0;
    if (kTotalPacketsCount > 1) {
        for (Packet::Index packetIndex=0; packetIndex<kTotalPacketsCount-1; ++packetIndex) {
            auto buffer = static_cast<byte*>(malloc(Packet::kMaxSize));

            memcpy(
                buffer,
                &Packet::kMaxSize,
                sizeof(Packet::kMaxSize));

            memcpy(
                buffer + PacketHeader::kChannelIndexOffset,
                &channelIndex,
                sizeof(channelIndex));

            memcpy(
                buffer + PacketHeader::kPacketsCountOffset,
                &kTotalPacketsCount,
                sizeof(kTotalPacketsCount));

            memcpy(
                buffer + PacketHeader::kPacketIndexOffset,
                &packetIndex,
                sizeof(packetIndex));

            memcpy(
                buffer + PacketHeader::kDataOffset,
                messageData + processedBytesCount,
                Packet::kMaxSize - PacketHeader::kSize);

            processedBytesCount += Packet::kMaxSize;

            const auto packetMaxSize = Packet::kMaxSize;
            mPacketsQueue.push(
                make_pair(
                    buffer,
                    packetMaxSize));
        }
    }

    // Writing last packet
    const PacketHeader::PacketIndex kLastPacketIndex = kTotalPacketsCount - 1;
    const Packet::Size kLastPacketSize =
          kMessageWithCRC32BytesCount
        - processedBytesCount
        + PacketHeader::kSize;
    auto buffer = static_cast<byte*>(malloc(kLastPacketSize));

    memcpy(
        buffer + PacketHeader::kPacketSizeOffset,
        &kLastPacketSize,
        sizeof(Packet::kMaxSize));

    memcpy(
        buffer + PacketHeader::kChannelIndexOffset,
        &channelIndex,
        sizeof(channelIndex));

    memcpy(
        buffer + PacketHeader::kPacketsCountOffset,
        &kTotalPacketsCount,
        sizeof(kTotalPacketsCount));

    memcpy(
        buffer + PacketHeader::kPacketIndexOffset,
        &kLastPacketIndex,
        sizeof(kLastPacketIndex));

    memcpy(
        buffer + PacketHeader::kDataOffset,
        messageData + processedBytesCount,
        kLastPacketSize - sizeof(uint32_t) - PacketHeader::kSize);

    processedBytesCount += kLastPacketSize - sizeof(uint32_t) - PacketHeader::kSize;

    // Copying CRC32 checksum
    memcpy(
        buffer + PacketHeader::kDataOffset + processedBytesCount,
        &crcChecksum,
        sizeof(crcChecksum));

    processedBytesCount += sizeof(crcChecksum);

    mPacketsQueue.push(
        make_pair(
            buffer,
            kLastPacketSize));
}

void OutgoingRemoteNode::beginPacketsSending()
{
    const auto kOnErrorDelaySeconds = 5;

    try {
        auto endpoint = mUUID2AddressService.endpoint(mRemoteNodeUUID);

        const auto packetDataAndSize = mPacketsQueue.front();
        mSocket.async_send_to(
            boost::asio::buffer(
                packetDataAndSize.first,
                packetDataAndSize.second),
            endpoint,
            [this, endpoint, kOnErrorDelaySeconds] (const boost::system::error_code &error, const size_t bytesTransferred) {
                const auto packetDataAndSize = mPacketsQueue.front();

                if (bytesTransferred != packetDataAndSize.second) {
                    if (error) {
                        errors()
                            << "OutgoingRemoteNode::beginPacketsSending: "
                            << "Next packet can't be sent to the node (" << mRemoteNodeUUID << "). "
                            << "Error code: " << error.value();

                        mPacketsSendingTimeoutTimer.expires_from_now(
                            chrono::seconds(kOnErrorDelaySeconds));

                        mPacketsSendingTimeoutTimer.async_wait(
                            [this] (const boost::system::error_code &){
                                this->beginPacketsSending(); });

                        return;
                    }
                }

#ifdef NETWORK_DEBUG_LOG
                const uint16_t channelIndex = *(new(packetDataAndSize.first + PacketHeader::kChannelIndexOffset) uint16_t);
                const uint16_t packetIndex = *(new(packetDataAndSize.first + PacketHeader::kPacketIndexOffset) uint16_t) + 1;
                const uint16_t totalPacketsCount = *(new(packetDataAndSize.first + PacketHeader::kPacketsCountOffset) uint16_t);

                this->debug()
                    << setw(4) << bytesTransferred <<  "B TX [ => ] "
                    << endpoint.address() << ":" << endpoint.port() << "; "
                    << "Channel: " << setw(6) << channelIndex << "; "
                    << "Packet: " << setw(6) << packetIndex << "/" << totalPacketsCount;
#endif

                // Removing packet from the memory
                free(packetDataAndSize.first);
                mPacketsQueue.pop();

                if (mPacketsQueue.size() > 0) {
                    beginPacketsSending();
                }
            });


    } catch (exception &e) {
        errors()
            << "OutgoingRemoteNode::beginPacketsSending: "
            << "Next packet can't be sent to the node (" << mRemoteNodeUUID << "). "
            << "Exception details are: " << e.what() << " "
            << "Sending data to this node would be suspended for " << kOnErrorDelaySeconds << " seconds. "
            << "Current packet sending would be retried.";

        mPacketsSendingTimeoutTimer.expires_from_now(
            chrono::seconds(kOnErrorDelaySeconds));

        mPacketsSendingTimeoutTimer.async_wait(
            [this] (const boost::system::error_code &){
                this->beginPacketsSending(); });

        return;
    }
}

PacketHeader::ChannelIndex OutgoingRemoteNode::nextChannelIndex()
    noexcept
{
    // Integer overflow is normal here.
    return mNextAvailableChannelIndex++;
}

LoggerStream OutgoingRemoteNode::errors() const
{
    return mLog.error(
        string("OutgoingRemoteNode [") + mRemoteNodeUUID.stringUUID() + string("]"));
}

LoggerStream OutgoingRemoteNode::debug() const
{
#ifdef NETWORK_DEBUG_LOG
    return mLog.debug(
        string("OutgoingRemoteNode [")
        + mRemoteNodeUUID.stringUUID()
        + string("]"));
#endif

#ifndef NETWORK_DEBUG_LOG
    return LoggerStream::dummy();
#endif
}
