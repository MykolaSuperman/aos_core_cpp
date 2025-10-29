/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>

#include <common/logger/logmodule.hpp>
#include <common/pbconvert/sm.hpp>
#include <common/utils/exception.hpp>
#include <common/utils/grpchelper.hpp>

#include "nodehandler.hpp"
#include "smcontroller.hpp"

namespace aos::cm::smcontroller {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error SMController::Init(const Config& config, iamclient::CertProviderItf& certProvider,
    crypto::CertLoaderItf& certLoader, crypto::x509::ProviderItf& cryptoProvider,
    UpdateImageProviderItf& updateImageProvider, alerts::ReceiverItf& alertsReceiver, SenderItf& logSender,
    communication::SenderItf& envVarsStatusSender, monitoring::ReceiverItf& monitoringReceiver,
    launcher::InstanceStatusReceiverItf& instanceStatusReceiver, nodeinfoprovider::SMInfoReceiverItf& smInfoReceiver,
    bool insecureConn)
{
    LOG_INF() << "Initialize SM Controller";

    mConfig                 = config;
    mCertProvider           = &certProvider;
    mCertLoader             = &certLoader;
    mCryptoProvider         = &cryptoProvider;
    mUpdateImageProvider    = &updateImageProvider;
    mAlertsReceiver         = &alertsReceiver;
    mLogSender              = &logSender;
    mEnvVarsStatusSender    = &envVarsStatusSender;
    mMonitoringReceiver     = &monitoringReceiver;
    mInstanceStatusReceiver = &instanceStatusReceiver;
    mSMInfoReceiver         = &smInfoReceiver;
    mInsecureConn           = insecureConn;

    if (auto err = CreateServerCredentials(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error SMController::Start()
{
    LOG_INF() << "Start SM Controller";

    if (auto err = StartServer(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto err = mCertProvider->SubscribeListener(String(mConfig.mCertStorage.c_str()), *this);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error SMController::Stop()
{
    LOG_INF() << "Stop SM Controller";

    if (auto err = StopServer(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mCertProvider->UnsubscribeListener(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * NodeConfigHandlerItf implementation
 **********************************************************************************************************************/

Error SMController::CheckNodeConfig(const String& nodeID, const NodeConfig& config)
{
    LOG_DBG() << "Checking node config for node: " << nodeID.CStr();

    NodeHandler* nodeHandler = FindNode(nodeID);
    if (!nodeHandler) {

        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "node not found"));
    }

    if (auto err = nodeHandler->CheckNodeConfig(config); !err.IsNone()) {
        LOG_ERR() << "Failed to check node config for node " << nodeID.CStr() << ": " << err;
        return err;
    }

    LOG_INF() << "Node config check passed for node: " << nodeID.CStr();
    return ErrorEnum::eNone;
}

Error SMController::UpdateNodeConfig(const String& nodeID, const NodeConfig& config)
{
    LOG_DBG() << "Updating config for node: " << Log::Field("nodeID", nodeID.CStr());

    NodeHandler* node = FindNode(nodeID);
    if (!node) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "node not found"));
    }

    if (auto err = node->UpdateNodeConfig(config); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error SMController::GetNodeConfigStatus(const String& nodeID, NodeConfigStatus& status)
{
    LOG_DBG() << "Getting config status for node: " << Log::Field("nodeID", nodeID.CStr());

    NodeHandler* node = FindNode(nodeID);
    if (!node) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "node not found"));
    }

    if (auto err = node->GetNodeConfigStatus(status); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * LogProviderItf implementation
 **********************************************************************************************************************/

Error SMController::RequestLog(const aos::RequestLog& log)
{
    LOG_DBG() << "Requesting log" << Log::Field("logID", log.mLogID);

    for (const auto& nodeID : log.mFilter.mNodes) {
        NodeHandler* node = FindNode(nodeID);
        if (!node) {
            return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "node not found"));
        }

        if (auto err = node->RequestLog(log); !err.IsNone()) {
            return err;
        }
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * NodeNetworkItf implementation
 **********************************************************************************************************************/

Error SMController::UpdateNetworks(const String& nodeID, const Array<UpdateNetworkParameters>& networkParameters)
{
    LOG_DBG() << "Updating networks for node: " << Log::Field("nodeID", nodeID.CStr());

    NodeHandler* node = FindNode(nodeID);
    if (!node) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "node not found"));
    }

    if (auto err = node->UpdateNetworks(networkParameters); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error SMController::OverrideEnvVars(const String& nodeID, const OverrideEnvVarsRequest& envVars)
{
    LOG_DBG() << "Overriding environment variables" << Log::Field("nodeID", nodeID.CStr());

    NodeHandler* node = FindNode(nodeID);
    if (!node) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "node not found"));
    }

    if (auto err = node->OverrideEnvVars(envVars); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * InstanceRunnerItf implementation
 **********************************************************************************************************************/

Error SMController::UpdateInstances(
    const String& nodeID, const Array<aos::InstanceInfo>& stopInstances, const Array<aos::InstanceInfo>& startInstances)
{
    LOG_DBG() << "Updating instances for node: " << Log::Field("nodeID", nodeID.CStr());

    NodeHandler* node = FindNode(nodeID);
    if (!node) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "node not found"));
    }

    if (auto err = node->UpdateInstances(stopInstances, startInstances); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * MonitoringProviderItf implementation
 **********************************************************************************************************************/

Error SMController::GetAverageMonitoring(const String& nodeID, aos::monitoring::NodeMonitoringData& monitoring)
{
    LOG_DBG() << "Getting average monitoring for node: " << Log::Field("nodeID", nodeID.CStr());

    NodeHandler* node = FindNode(nodeID);
    if (!node) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "node not found"));
    }

    if (auto err = node->GetAverageMonitoring(monitoring); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * ConnectionListenerItf implementation
 **********************************************************************************************************************/

void SMController::OnConnect()
{
    LOG_INF() << "Cloud connected";

    std::lock_guard<std::mutex> lock(mMutex);

    for (auto& nodeHandler : mNodeHandlers) {
        nodeHandler->OnConnect();
    }
}

void SMController::OnDisconnect()
{
    LOG_INF() << "Cloud disconnected";

    std::lock_guard<std::mutex> lock(mMutex);

    for (auto& nodeHandler : mNodeHandlers) {
        nodeHandler->OnDisconnect();
    }
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

grpc::Status SMController::RegisterSM(grpc::ServerContext*                                                    context,
    grpc::ServerReaderWriter<servicemanager::v5::SMIncomingMessages, servicemanager::v5::SMOutgoingMessages>* stream)
{
    LOG_INF() << "SM registration request received";

    std::lock_guard<std::mutex> lock(mMutex);

    auto nodeHandler = std::make_shared<NodeHandler>(context, stream, *mAlertsReceiver, *mLogSender,
        *mEnvVarsStatusSender, *mMonitoringReceiver, *mInstanceStatusReceiver, *mSMInfoReceiver, *this);

    if (auto err = nodeHandler->Start(); !err.IsNone()) {
        LOG_ERR() << "Failed to start node handler" << Log::Field(err);

        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to start node handler");
    }

    mNodeHandlers.push_back(nodeHandler);

    return grpc::Status::OK;
}

grpc::Status SMController::GetItemImageInfo(grpc::ServerContext* /*context*/,
    const servicemanager::v5::ItemImageInfoRequest* request, servicemanager::v5::ItemImageInfo* response)
{
    LOG_DBG() << "Get item image info request received";

    auto imageInfo = std::make_unique<UpdateImageInfo>();

    if (auto err = mUpdateImageProvider->GetUpdateImageInfo(request->image_id().c_str(), "", *imageInfo);
        !err.IsNone()) {
        return grpc::Status(grpc::StatusCode::INTERNAL, err.Message());
    }

    if (auto err = common::pbconvert::ConvertToProto(*imageInfo, *response); !err.IsNone()) {
        return grpc::Status(grpc::StatusCode::INTERNAL, err.Message());
    }

    return grpc::Status::OK;
}

grpc::Status SMController::GetLayerImageInfo(grpc::ServerContext* /*context*/,
    const servicemanager::v5::LayerImageInfoRequest* request, servicemanager::v5::ItemImageInfo* response)
{
    LOG_DBG() << "Get layer image info request received";

    auto imageInfo = std::make_unique<UpdateImageInfo>();

    if (auto err = mUpdateImageProvider->GetLayerImageInfo(request->digest().c_str(), *imageInfo); !err.IsNone()) {
        return grpc::Status(grpc::StatusCode::INTERNAL, err.Message());
    }

    if (auto err = common::pbconvert::ConvertToProto(*imageInfo, *response); !err.IsNone()) {
        return grpc::Status(grpc::StatusCode::INTERNAL, err.Message());
    }

    return grpc::Status::OK;
}

void SMController::OnCertChanged(const CertInfo& info)
{
    (void)info;

    LOG_DBG() << "Certificate changed";

    if (auto err = CreateServerCredentials(); !err.IsNone()) {
        LOG_ERR() << "Failed to create server credentials" << Log::Field(err);

        return;
    }

    if (auto err = Stop(); !err.IsNone()) {
        LOG_ERR() << "Failed to stop server" << Log::Field(err);

        return;
    }

    if (auto err = Start(); !err.IsNone()) {
        LOG_ERR() << "Failed to start server" << Log::Field(err);

        return;
    }
}

void SMController::OnNodeConnected(const String& nodeID)
{
    LOG_INF() << "SM client connected" << Log::Field("nodeID", nodeID);
}

void SMController::OnNodeDisconnected(const String& nodeID)
{
    LOG_INF() << "SM client disconnected" << Log::Field("nodeID", nodeID);

    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find_if(mNodeHandlers.begin(), mNodeHandlers.end(),
        [&nodeID](const std::shared_ptr<NodeHandler>& handler) { return handler->GetNodeID() == nodeID; });

    if (it != mNodeHandlers.end()) {
        mNodeHandlers.erase(it);
    }
}

NodeHandler* SMController::FindNode(const String& nodeID)
{
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = std::find_if(mNodeHandlers.begin(), mNodeHandlers.end(),
        [&nodeID](const std::shared_ptr<NodeHandler>& handler) { return handler->GetNodeID() == nodeID; });

    return (it != mNodeHandlers.end()) ? it->get() : nullptr;
}

Error SMController::CreateServerCredentials()
{
    if (!mInsecureConn) {
        auto certInfo = std::make_unique<CertInfo>();

        if (auto err = mCertProvider->GetCert(String(mConfig.mCertStorage.c_str()), {}, {}, *certInfo); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        mCredentials = aos::common::utils::GetMTLSServerCredentials(
            *certInfo, mConfig.mCACert.c_str(), *mCertLoader, *mCryptoProvider);
    } else {
        mCredentials = grpc::InsecureServerCredentials();
    }

    return ErrorEnum::eNone;
}

RetWithError<std::string> SMController::CorrectAddress(const std::string& addr) const
{
    if (addr.empty()) {
        return {addr, AOS_ERROR_WRAP(ErrorEnum::eInvalidArgument)};
    }

    if (addr[0] == ':') {
        return "0.0.0.0" + addr;
    }

    return addr;
}

Error SMController::StartServer()
{
    auto [correctedAddress, err] = CorrectAddress(mConfig.mCMServerURL);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    grpc::ServerBuilder builder;

    builder.AddListeningPort(correctedAddress, mCredentials);
    builder.RegisterService(this);

    mServer = builder.BuildAndStart();
    if (!mServer) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eFailed, "failed to start CM server"));
    }

    return ErrorEnum::eNone;
}

Error SMController::StopServer()
{
    std::lock_guard<std::mutex> lock(mMutex);

    for (auto& handler : mNodeHandlers) {
        if (handler) {
            handler->Stop();
        }
    }

    mNodeHandlers.clear();

    if (mServer) {
        mServer->Shutdown();
        mServer->Wait();
    }

    return ErrorEnum::eNone;
}

} // namespace aos::cm::smcontroller
