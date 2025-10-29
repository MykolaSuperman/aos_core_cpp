/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CM_SMCONTROLLER_SMCONTROLLER_HPP_
#define AOS_CM_SMCONTROLLER_SMCONTROLLER_HPP_

#include <memory>
#include <mutex>
#include <vector>

#include <servicemanager/v5/servicemanager.grpc.pb.h>

#include <core/cm/launcher/itf/instancerunner.hpp>
#include <core/cm/launcher/itf/monitoringprovider.hpp>
#include <core/cm/launcher/itf/nodeenvvarhandler.hpp>
#include <core/cm/networkmanager/itf/nodenetwork.hpp>
#include <core/cm/nodeinfoprovider/itf/sminforeceiver.hpp>
#include <core/cm/smcontroller/itf/logprovider.hpp>
#include <core/cm/smcontroller/itf/updateimageprovider.hpp>
#include <core/cm/unitconfig/itf/nodeconfighandler.hpp>
#include <core/common/cloudconnection/itf/cloudconnection.hpp>
#include <core/common/crypto/itf/certloader.hpp>
#include <core/common/iamclient/itf/certprovider.hpp>

#include "nodehandler.hpp"

namespace aos::cm::smcontroller {

/**
 * SM controller configuration.
 */
struct Config {
    std::string mCMServerURL;
    std::string mCertStorage;
    std::string mCACert;
};

/**
 * Service Manager Controller.
 */
class SMController : public unitconfig::NodeConfigHandlerItf,
                     public LogProviderItf,
                     public networkmanager::NodeNetworkItf,
                     public launcher::NodeEnvVarHandlerItf,
                     public launcher::InstanceRunnerItf,
                     public launcher::MonitoringProviderItf,
                     public ConnectionListenerItf,
                     private iamclient::CertListenerItf,
                     public NodeConnectionStatusListenerItf,
                     private servicemanager::v5::SMService::Service {
public:
    /**
     * Initializes the SM controller.
     *
     * @return Error code.
     */
    Error Init(const Config& config, iamclient::CertProviderItf& certProvider, crypto::CertLoaderItf& certLoader,
        crypto::x509::ProviderItf& cryptoProvider, UpdateImageProviderItf& updateImageProvider,
        alerts::ReceiverItf& alertsReceiver, SenderItf& logSender, communication::SenderItf& envVarsStatusSender,
        monitoring::ReceiverItf& monitoringReceiver, launcher::InstanceStatusReceiverItf& instanceStatusReceiver,
        nodeinfoprovider::SMInfoReceiverItf& smInfoReceiver, bool insecureConn = false);

    /**
     * Starts the SM controller.
     *
     * @return Error code.
     */
    Error Start();

    /**
     * Stops the SM controller.
     *
     * @return Error code.
     */
    Error Stop();

    //
    // NodeConfigHandlerItf interface methods
    //

    /**
     * Checks node config.
     *
     * @param nodeID Node ID.
     * @param config Node config.
     * @return Error.
     */
    Error CheckNodeConfig(const String& nodeID, const NodeConfig& config) override;

    /**
     * Updates node config.
     *
     * @param nodeID Node ID.
     * @param config Node config.
     * @return Error.
     */
    Error UpdateNodeConfig(const String& nodeID, const NodeConfig& config) override;

    /**
     * Returns node config status.
     *
     * @param nodeID Node ID.
     * @param status Node config status.
     * @return Error.
     */
    Error GetNodeConfigStatus(const String& nodeID, NodeConfigStatus& status) override;

    //
    // LogProviderItf interface methods
    //

    /**
     * Requests log.
     *
     * @param log log request.
     * @return Error.
     */
    Error RequestLog(const aos::RequestLog& log) override;

    //
    // NodeNetworkItf interface methods
    //

    /**
     * Updates network parameters for a node.
     *
     * @param nodeID Node ID.
     * @param networkParameters Network parameters.
     * @return Error.
     */
    Error UpdateNetworks(const String& nodeID, const Array<UpdateNetworkParameters>& networkParameters) override;

    //
    // NodeEnvVarHandlerItf interface methods
    //

    /**
     * Overrides environment variables.
     *
     * @param nodeID Node ID.
     * @param envVars Environment variables.
     * @return Error.
     */
    Error OverrideEnvVars(const String& nodeID, const OverrideEnvVarsRequest& envVars) override;

    //
    // InstanceRunnerItf interface methods
    //

    /**
     * Updates instances on specified node.
     *
     * @param nodeID Node ID.
     * @param stopInstances Instance list to stop.
     * @param startInstances Instance list to start.
     * @return Error.
     */
    Error UpdateInstances(const String& nodeID, const Array<aos::InstanceInfo>& stopInstances,
        const Array<aos::InstanceInfo>& startInstances) override;

    //
    // MonitoringProviderItf interface methods
    //

    /**
     * Returns monitoring data for a node.
     *
     * @param nodeID Node ID.
     * @param monitoring Monitoring data.
     * @return Error.
     */
    Error GetAverageMonitoring(const String& nodeID, aos::monitoring::NodeMonitoringData& monitoring) override;

    //
    // ConnectionListenerItf interface methods
    //

    /**
     * Handles cloud connected event.
     */
    void OnConnect() override;

    /**
     * Handles cloud disconnected event.
     */
    void OnDisconnect() override;

private:
    grpc::Status RegisterSM(grpc::ServerContext* context,
        grpc::ServerReaderWriter<servicemanager::v5::SMIncomingMessages, servicemanager::v5::SMOutgoingMessages>*
            stream) override;
    grpc::Status GetItemImageInfo(grpc::ServerContext* context, const servicemanager::v5::ItemImageInfoRequest* request,
        servicemanager::v5::ItemImageInfo* response) override;
    grpc::Status GetLayerImageInfo(grpc::ServerContext*  context,
        const servicemanager::v5::LayerImageInfoRequest* request, servicemanager::v5::ItemImageInfo* response) override;

    // iamclient::CertListenerItf interface
    void OnCertChanged(const CertInfo& info) override;

    // NodeConnectionStatusListenerItf interface
    void OnNodeConnected(const String& nodeID) override;
    void OnNodeDisconnected(const String& nodeID) override;

    NodeHandler* FindNode(const String& nodeID);

    Error                     CreateServerCredentials();
    RetWithError<std::string> CorrectAddress(const std::string& addr) const;
    Error                     StartServer();
    Error                     StopServer();

    Config                               mConfig {};
    crypto::CertLoaderItf*               mCertLoader {};
    crypto::x509::ProviderItf*           mCryptoProvider {};
    iamclient::CertProviderItf*          mCertProvider {};
    UpdateImageProviderItf*              mUpdateImageProvider {};
    alerts::ReceiverItf*                 mAlertsReceiver {};
    SenderItf*                           mLogSender {};
    communication::SenderItf*            mEnvVarsStatusSender {};
    monitoring::ReceiverItf*             mMonitoringReceiver {};
    launcher::InstanceStatusReceiverItf* mInstanceStatusReceiver {};
    nodeinfoprovider::SMInfoReceiverItf* mSMInfoReceiver {};
    bool                                 mInsecureConn {};

    std::unique_ptr<grpc::Server>            mServer;
    std::mutex                               mMutex;
    std::shared_ptr<grpc::ServerCredentials> mCredentials;

    std::vector<std::shared_ptr<NodeHandler>> mNodeHandlers;
};

} // namespace aos::cm::smcontroller

#endif
