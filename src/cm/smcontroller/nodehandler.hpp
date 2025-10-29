/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CM_SMCONTROLLER_NODEHANDLER_HPP_
#define AOS_CM_SMCONTROLLER_NODEHANDLER_HPP_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <core/cm/alerts/itf/receiver.hpp>
#include <core/cm/launcher/itf/instancestatusreceiver.hpp>
#include <core/cm/launcher/itf/sender.hpp>
#include <core/cm/monitoring/itf/receiver.hpp>
#include <core/cm/nodeinfoprovider/itf/sminforeceiver.hpp>
#include <core/cm/smcontroller/itf/sender.hpp>
#include <core/common/types/envvars.hpp>
#include <core/common/types/instance.hpp>
#include <core/common/types/log.hpp>
#include <core/common/types/network.hpp>
#include <core/common/types/unitconfig.hpp>

#include <servicemanager/v5/servicemanager.grpc.pb.h>

namespace aos::cm::smcontroller {

/**
 * Message structure for synchronous communication.
 */
struct Message {
    servicemanager::v5::SMOutgoingMessages* mOutputMessage;
    std::condition_variable                 mCondVar;
    std::mutex                              mMutex;
    bool                                    mResponseReceived = false;
};

/**
 * Node connection status listener interface.
 */
class NodeConnectionStatusListenerItf {
public:
    virtual ~NodeConnectionStatusListenerItf() = default;

    virtual void OnNodeConnected(const String& nodeID)    = 0;
    virtual void OnNodeDisconnected(const String& nodeID) = 0;
};

/**
 * Handles communication with a specific Service Manager on a node.
 */
class NodeHandler {
public:
    /**
     * Constructor.
     *
     * @param context grpc server context.
     * @param stream grpc stream.
     * @param alertsReceiver alerts receiver.
     * @param logSender log sender.
     * @param envVarsStatusSender environment variables status sender.
     * @param monitoringReceiver monitoring receiver.
     * @param instanceStatusReceiver instance status receiver.
     * @param smInfoReceiver SM info receiver.
     */
    NodeHandler(grpc::ServerContext* context,
        grpc::ServerReaderWriter<servicemanager::v5::SMIncomingMessages, servicemanager::v5::SMOutgoingMessages>*
                             stream,
        alerts::ReceiverItf& alertsReceiver, SenderItf& logSender, communication::SenderItf& envVarsStatusSender,
        monitoring::ReceiverItf& monitoringReceiver, launcher::InstanceStatusReceiverItf& instanceStatusReceiver,
        nodeinfoprovider::SMInfoReceiverItf& smInfoReceiver, NodeConnectionStatusListenerItf& connStatusListener);

    /**
     * Starts handling the node communication.
     *
     * @return Error code.
     */
    Error Start();

    /**
     * Stops handling the node communication.
     *
     * @return Error code.
     */
    Error Stop();

    /**
     * Get the Node ID.
     *
     * @return String.
     */
    String GetNodeID() const;

    /**
     * Gets node config status.
     *
     * @param status node config status.
     * @return Error code.
     */
    Error GetNodeConfigStatus(NodeConfigStatus& status);

    /**
     * Checks node config.
     *
     * @param config node config.
     * @return Error code.
     */
    Error CheckNodeConfig(const NodeConfig& config);

    /**
     * Updates node config.
     *
     * @param config node config.
     * @return Error code.
     */
    Error UpdateNodeConfig(const NodeConfig& config);

    /**
     * Requests log.
     *
     * @param log log request.
     * @return Error code.
     */
    Error RequestLog(const aos::RequestLog& log);

    /**
     * Updates network parameters.
     *
     * @param networkParameters network parameters.
     * @return Error code.
     */
    Error UpdateNetworks(const Array<UpdateNetworkParameters>& networkParameters);

    /**
     * Overrides environment variables.
     *
     * @param envVars environment variables request.
     * @return Error code.
     */
    Error OverrideEnvVars(const OverrideEnvVarsRequest& envVars);

    /**
     * Updates instances.
     *
     * @param stopInstances instances to stop.
     * @param startInstances instances to start.
     * @return Error code.
     */
    Error UpdateInstances(
        const Array<aos::InstanceInfo>& stopInstances, const Array<aos::InstanceInfo>& startInstances);

    /**
     * Gets average monitoring data.
     *
     * @param monitoring average monitoring data.
     * @return Error code.
     */
    Error GetAverageMonitoring(aos::monitoring::NodeMonitoringData& monitoring);

    /**
     * Handles cloud connected event.
     */
    void OnConnect();

    /**
     * Handles cloud disconnected event.
     */
    void OnDisconnect();

private:
    static constexpr auto cResponseTime = std::chrono::seconds(5);

    Error SendMessage(const servicemanager::v5::SMIncomingMessages& message);
    Error SendMessageSync(
        const servicemanager::v5::SMIncomingMessages& inMessage, servicemanager::v5::SMOutgoingMessages& outMessage);

    // Message processing methods
    Error ProcessMessages();

    void ProcessSMInfo(const servicemanager::v5::SMInfo& smInfo);
    void ProcessNodeConfigStatus(const servicemanager::v5::NodeConfigStatus& status);
    void ProcessUpdateInstancesStatus(const servicemanager::v5::UpdateInstancesStatus& status);
    void ProcessNodeInstancesStatus(const servicemanager::v5::NodeInstancesStatus& status);
    void ProcessOverrideEnvVarStatus(const servicemanager::v5::OverrideEnvVarStatus& status);
    void ProcessLogData(const servicemanager::v5::LogData& logData);
    void ProcessInstantMonitoring(const servicemanager::v5::InstantMonitoring& monitoring);
    void ProcessAverageMonitoring(const servicemanager::v5::AverageMonitoring& monitoring);
    void ProcessAlert(const servicemanager::v5::Alert& alert);

    grpc::ServerContext* mContext {};
    grpc::ServerReaderWriter<servicemanager::v5::SMIncomingMessages, servicemanager::v5::SMOutgoingMessages>*
                                         mStream {};
    alerts::ReceiverItf*                 mAlertsReceiver {};
    SenderItf*                           mLogSender {};
    communication::SenderItf*            mEnvVarsStatusSender {};
    monitoring::ReceiverItf*             mMonitoringReceiver {};
    launcher::InstanceStatusReceiverItf* mInstanceStatusReceiver {};
    nodeinfoprovider::SMInfoReceiverItf* mSMInfoReceiver {};
    NodeConnectionStatusListenerItf*     mConnStatusListener {};

    std::vector<Message*> mMessages;
    std::mutex            mMutex;
    bool                  mCredentialListUpdated {};
    grpc::ServerContext*  mCtx {};

    std::thread       mProcessThread;
    std::atomic<bool> mStopProcessing {};

    StaticString<cIDLen> mNodeID;
};

} // namespace aos::cm::smcontroller

#endif
