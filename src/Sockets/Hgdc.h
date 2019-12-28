/* Copyright 2013-2019 Homegear GmbH
 *
 * libhomegear-base is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * libhomegear-base is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libhomegear-base.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef LIBHOMEGEAR_BASE_HGDC_H
#define LIBHOMEGEAR_BASE_HGDC_H

#include "../Systems/Packet.h"
#include "TcpSocket.h"
#include "../Encoding/BinaryRpc.h"
#include "../Encoding/RpcEncoder.h"
#include "../Encoding/RpcDecoder.h"
#include "../Output/Output.h"
#include "../IQueue.h"

#include <condition_variable>

namespace BaseLib
{

class Hgdc : public IQueue
{
private:
    class QueueEntry : public BaseLib::IQueueEntry
    {
    public:
        std::string method;
        BaseLib::PArray parameters;
    };

    SharedObjects* _bl = nullptr;
    uint16_t _port = 0;

    Output _out;
    std::unique_ptr<TcpSocket> _tcpSocket;
    std::unique_ptr<Rpc::BinaryRpc> _binaryRpc;
    std::unique_ptr<Rpc::RpcEncoder> _rpcEncoder;
    std::unique_ptr<Rpc::RpcDecoder> _rpcDecoder;

    std::atomic_bool _stopped{true};
    std::atomic_bool _stopCallbackThread{true};
    std::thread _listenThread;

    int32_t _currentEventHandlerId = 0;
    std::mutex _packetReceivedEventHandlersMutex;
    std::unordered_map<int64_t, std::list<std::pair<int32_t, std::function<void(int64_t, const std::string&, const std::vector<uint8_t>&)>>>> _packetReceivedEventHandlers;
    std::mutex _moduleUpdateEventHandlersMutex;
    std::unordered_map<int32_t, std::function<void(const BaseLib::PVariable&)>> _moduleUpdateEventHandlers;
    std::mutex _reconnectedEventHandlersMutex;
    std::unordered_map<int32_t, std::function<void()>> _reconnectedEventHandlers;

    std::mutex _invokeMutex;
    std::mutex _requestMutex;
    std::atomic_bool _waitForResponse{false};
    std::condition_variable _requestConditionVariable;
    BaseLib::PVariable _rpcResponse;

    void listen();
    PVariable invoke(const std::string& methodName, const PArray& parameters);
    void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry) override;
public:
    explicit Hgdc(SharedObjects* bl, uint16_t port);
    ~Hgdc();

    void start();
    void stop();

    int32_t registerPacketReceivedEventHandler(int64_t familyId, std::function<void(int64_t, const std::string&, const std::vector<uint8_t>&)> value);
    void unregisterPacketReceivedEventHandler(int32_t eventHandlerId);
    int32_t registerModuleUpdateEventHandler(std::function<void(const BaseLib::PVariable&)> value);
    void unregisterModuleUpdateEventHandler(int32_t eventHandlerId);
    int32_t registerReconnectedEventHandler(std::function<void()> value);
    void unregisterReconnectedEventHandler(int32_t eventHandlerId);

    PVariable getModules(int64_t familyId = -1);
    bool sendPacket(const std::string& serialNumber, const std::vector<uint8_t>& packet);
    bool sendPacket(const std::string& serialNumber, const std::vector<char>& packet);
};

}

#endif //LIBHOMEGEAR_BASE_HGDC_H
