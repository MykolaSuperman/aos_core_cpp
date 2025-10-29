/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nodehandler.hpp"

#include <algorithm>

#include <common/logger/logmodule.hpp>

#include <common/pbconvert/sm.hpp>
#include <common/utils/exception.hpp>

namespace aos::cm::smcontroller {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

NodeHandler::NodeHandler(grpc::ServerContext*                                                                 context,
    grpc::ServerReaderWriter<servicemanager::v5::SMIncomingMessages, servicemanager::v5::SMOutgoingMessages>* stream,
    alerts::ReceiverItf& alertsReceiver, SenderItf& logSender, communication::SenderItf& envVarsStatusSender,
    monitoring::ReceiverItf& monitoringReceiver, launcher::InstanceStatusReceiverItf& instanceStatusReceiver,
    nodeinfoprovider::SMInfoReceiverItf& smInfoReceiver, NodeConnectionStatusListenerItf& connStatusListener)
    : mContext(context)
    , mStream(stream)
    , mAlertsReceiver(&alertsReceiver)
    , mLogSender(&logSender)
    , mEnvVarsStatusSender(&envVarsStatusSender)
    , mMonitoringReceiver(&monitoringReceiver)
    , mInstanceStatusReceiver(&instanceStatusReceiver)
    , mSMInfoReceiver(&smInfoReceiver)
    , mConnStatusListener(&connStatusListener)
{
}

Error NodeHandler::Start()
{
    LOG_INF() << "Start NodeHandler";

    mStopProcessing.store(false);
    mProcessThread = std::thread([this]() { ProcessMessages(); });

    return ErrorEnum::eNone;
}

Error NodeHandler::Stop()
{
    LOG_INF() << "Stop NodeHandler";

    mStopProcessing.store(true);

    if (mContext) {
        mContext->TryCancel();
    }

    if (mProcessThread.joinable()) {
        mProcessThread.join();
    }

    return ErrorEnum::eNone;
}

String NodeHandler::GetNodeID() const
{
    return mNodeID;
}

Error NodeHandler::GetNodeConfigStatus(NodeConfigStatus& status)
{
    LOG_DBG() << "Getting node configuration status" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    servicemanager::v5::SMOutgoingMessages outMsg;

    // Create a get_node_config_status request in the incoming message
    inMsg.mutable_get_node_config_status();
    auto* nodeConfigStatus = outMsg.mutable_node_config_status();

    if (auto err = SendMessageSync(inMsg, outMsg); !err.IsNone()) {
        return err;
    }

    if (auto err = common::pbconvert::ConvertFromProto(*nodeConfigStatus, status); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error NodeHandler::CheckNodeConfig(const NodeConfig& config)
{
    LOG_DBG() << "Checking node config for node" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    servicemanager::v5::SMOutgoingMessages outMsg;

    auto* checkNodeConfig = inMsg.mutable_check_node_config();
    if (auto err = common::pbconvert::ConvertToProto(config, *checkNodeConfig); !err.IsNone()) {
        return err;
    }

    auto* nodeConfigStatus = outMsg.mutable_node_config_status();

    if (auto err = SendMessageSync(inMsg, outMsg); !err.IsNone()) {
        return err;
    }

    return common::pbconvert::ConvertFromProto(nodeConfigStatus->error());
}

Error NodeHandler::UpdateNodeConfig(const NodeConfig& config)
{
    LOG_DBG() << "Updating node config for node" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    servicemanager::v5::SMOutgoingMessages outMsg;

    auto* setNodeConfig = inMsg.mutable_set_node_config();
    if (auto err = common::pbconvert::ConvertToProto(config, *setNodeConfig); !err.IsNone()) {
        return err;
    }

    auto* nodeConfigStatus = outMsg.mutable_node_config_status();

    if (auto err = SendMessageSync(inMsg, outMsg); !err.IsNone()) {
        return err;
    }

    return common::pbconvert::ConvertFromProto(nodeConfigStatus->error());
}

Error NodeHandler::RequestLog(const aos::RequestLog& log)
{
    LOG_DBG() << "Requesting log" << Log::Field("logID", log.mLogID) << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    servicemanager::v5::SMOutgoingMessages outMsg;

    switch (log.mLogType.GetValue()) {
    case LogTypeEnum::eSystemLog: {
        auto* systemLogRequest = inMsg.mutable_system_log_request();
        if (auto err = common::pbconvert::ConvertToProto(log, *systemLogRequest); !err.IsNone()) {
            return err;
        }
        break;
    }

    case LogTypeEnum::eInstanceLog: {
        auto* instanceLogRequest = inMsg.mutable_instance_log_request();
        if (auto err = common::pbconvert::ConvertToProto(log, *instanceLogRequest); !err.IsNone()) {
            return err;
        }
        break;
    }
    case LogTypeEnum::eCrashLog: {
        auto* instanceCrashLogRequest = inMsg.mutable_instance_crash_log_request();
        if (auto err = common::pbconvert::ConvertToProto(log, *instanceCrashLogRequest); !err.IsNone()) {
            return err;
        }
        break;
    }
    default:
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotSupported, "unknown log type"));
    }

    if (auto err = SendMessage(inMsg); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error NodeHandler::UpdateNetworks(const Array<UpdateNetworkParameters>& networkParameters)
{
    LOG_DBG() << "Updating networks for node" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    auto*                                  updateNetworks = inMsg.mutable_update_networks();

    if (auto err = common::pbconvert::ConvertToProto(networkParameters, *updateNetworks); !err.IsNone()) {
        return err;
    }

    if (auto err = SendMessage(inMsg); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error NodeHandler::OverrideEnvVars(const OverrideEnvVarsRequest& envVars)
{
    LOG_DBG() << "Overriding environment variables for node" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    auto*                                  overrideEnvVarsRequest = inMsg.mutable_override_env_vars();

    if (auto err = common::pbconvert::ConvertToProto(envVars, *overrideEnvVarsRequest); !err.IsNone()) {
        return err;
    }

    servicemanager::v5::SMOutgoingMessages outMsg;

    if (auto err = SendMessageSync(inMsg, outMsg); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error NodeHandler::ProcessMessages()
{
    try {
        servicemanager::v5::SMOutgoingMessages outgoingMsg;

        while (!mStopProcessing.load() && mStream->Read(&outgoingMsg)) {
            if (outgoingMsg.has_sm_info()) {
                ProcessSMInfo(outgoingMsg.sm_info());
            } else if (outgoingMsg.has_node_config_status()) {
                ProcessNodeConfigStatus(outgoingMsg.node_config_status());
            } else if (outgoingMsg.has_update_instances_status()) {
                ProcessUpdateInstancesStatus(outgoingMsg.update_instances_status());
            } else if (outgoingMsg.has_node_instances_status()) {
                ProcessNodeInstancesStatus(outgoingMsg.node_instances_status());
            } else if (outgoingMsg.has_override_env_var_status()) {
                ProcessOverrideEnvVarStatus(outgoingMsg.override_env_var_status());
            } else if (outgoingMsg.has_log()) {
                ProcessLogData(outgoingMsg.log());
            } else if (outgoingMsg.has_instant_monitoring()) {
                ProcessInstantMonitoring(outgoingMsg.instant_monitoring());
            } else if (outgoingMsg.has_average_monitoring()) {
                ProcessAverageMonitoring(outgoingMsg.average_monitoring());
            } else if (outgoingMsg.has_alert()) {
                ProcessAlert(outgoingMsg.alert());
            } else {
                LOG_WRN() << "Unknown message type received";
            }
        }
    } catch (const std::exception& e) {
        LOG_ERR() << "Handle incoming messages failed" << Log::Field(AOS_ERROR_WRAP(common::utils::ToAosError(e)));
    }

    mConnStatusListener->OnNodeDisconnected(GetNodeID());

    return ErrorEnum::eNone;
}

Error NodeHandler::SendMessage(const servicemanager::v5::SMIncomingMessages& message)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mStream->Write(message)) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eFailed, "failed to send message"));
    }

    return ErrorEnum::eNone;
}

Error NodeHandler::SendMessageSync(
    const servicemanager::v5::SMIncomingMessages& inMessage, servicemanager::v5::SMOutgoingMessages& outMessage)
{
    std::unique_lock<std::mutex> lock(mMutex);

    Message msg;

    msg.mOutputMessage = &outMessage;
    mMessages.push_back(&msg);

    // Writes/Reads to the stream can be synchronized independently. According to:
    // https://groups.google.com/g/grpc-io/c/G7FzRNQBWhU?pli=1
    if (!mStream->Write(inMessage)) {
        mMessages.erase(std::find(mMessages.begin(), mMessages.end(), &msg));

        return AOS_ERROR_WRAP(Error(ErrorEnum::eFailed, "failed to send message"));
    }

    // Receiving the message.
    {
        std::unique_lock<std::mutex> msgLock(msg.mMutex);
        lock.unlock();

        msg.mCondVar.wait_for(msgLock, cResponseTime, [&msg] { return msg.mResponseReceived; });

        lock.lock();
    }

    mMessages.erase(std::find(mMessages.begin(), mMessages.end(), &msg));

    if (!msg.mResponseReceived) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eTimeout, "response timeout"));
    }

    return ErrorEnum::eNone;
}

void NodeHandler::ProcessSMInfo(const servicemanager::v5::SMInfo& smInfo)
{
    LOG_DBG() << "Processing SM info" << Log::Field("nodeID", GetNodeID());

    auto aosSMInfo = std::make_unique<aos::cm::nodeinfoprovider::SMInfo>();

    if (auto err = common::pbconvert::ConvertFromProto(smInfo, *aosSMInfo); !err.IsNone()) {
        LOG_ERR() << "Failed to convert SM info from protobuf" << Log::Field(err);

        return;
    }

    if (mNodeID.IsEmpty()) {
        mNodeID = aosSMInfo->mNodeID;

        mConnStatusListener->OnNodeConnected(GetNodeID());
    }

    if (auto err = mSMInfoReceiver->OnSMInfoReceived(*aosSMInfo); !err.IsNone()) {
        LOG_ERR() << "Failed to send SM info to receiver" << Log::Field(err);

        return;
    }
}

void NodeHandler::ProcessNodeConfigStatus(const servicemanager::v5::NodeConfigStatus& status)
{
    LOG_DBG() << "Processing node config status" << Log::Field("nodeID", GetNodeID());

    try {
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto* msg : mMessages) {
            if (msg->mOutputMessage && msg->mOutputMessage->has_node_config_status()) {
                std::lock_guard<std::mutex> msgLock(msg->mMutex);

                msg->mResponseReceived = true;
                msg->mCondVar.notify_one();
                msg->mOutputMessage->mutable_node_config_status()->CopyFrom(status);

                break;
            }
        }
    } catch (const std::exception& e) {
        LOG_ERR() << "Failed to process node config status" << Log::Field("error", e.what());
    }
}

void NodeHandler::ProcessUpdateInstancesStatus(const servicemanager::v5::UpdateInstancesStatus& status)
{
    LOG_DBG() << "Processing update instances status" << Log::Field("nodeID", GetNodeID());

    for (const auto& grpcInstanceStatus : status.instances()) {
        auto instanceStatus = std::make_unique<InstanceStatus>();

        if (auto err = common::pbconvert::ConvertFromProto(grpcInstanceStatus, GetNodeID(), *instanceStatus);
            !err.IsNone()) {
            LOG_ERR() << "Failed to convert instance status from protobuf" << Log::Field(err);

            continue;
        }

        if (auto err = mInstanceStatusReceiver->OnInstanceStatusReceived(*instanceStatus); !err.IsNone()) {
            LOG_ERR() << "Failed to send instance status to receiver" << Log::Field(err);

            continue;
        }
    }
}

void NodeHandler::ProcessNodeInstancesStatus(const servicemanager::v5::NodeInstancesStatus& status)
{
    LOG_DBG() << "Processing node instances status" << Log::Field("nodeID", GetNodeID());

    auto statuses = std::make_unique<StaticArray<InstanceStatus, cMaxNumInstances>>();

    for (const auto& grpcInstanceStatus : status.instances()) {
        if (auto err = statuses->EmplaceBack(); !err.IsNone()) {
            LOG_ERR() << "Failed to allocate instance status item" << Log::Field(err);

            return;
        }

        if (auto err = common::pbconvert::ConvertFromProto(grpcInstanceStatus, GetNodeID(), statuses->Back());
            !err.IsNone()) {
            LOG_ERR() << "Failed to convert instance status from protobuf" << Log::Field(err);

            continue;
        }
    }

    if (auto err = mInstanceStatusReceiver->OnNodeInstancesStatusesReceived(GetNodeID(), *statuses); !err.IsNone()) {
        LOG_ERR() << "Failed to send node instances statuses to receiver" << Log::Field(err);

        return;
    }
}

void NodeHandler::ProcessOverrideEnvVarStatus(const servicemanager::v5::OverrideEnvVarStatus& status)
{
    LOG_DBG() << "Processing override env var status" << Log::Field("nodeID", GetNodeID());

    auto envVarsStatuses = std::make_unique<OverrideEnvVarsStatuses>();

    if (auto err = common::pbconvert::ConvertFromProto(status, *envVarsStatuses); !err.IsNone()) {
        LOG_ERR() << "Failed to convert override env var status from protobuf" << Log::Field(err);
        return;
    }

    if (auto err = mEnvVarsStatusSender->SendOverrideEnvsStatuses(*envVarsStatuses); !err.IsNone()) {
        LOG_ERR() << "Failed to send override env vars statuses" << Log::Field(err);
        return;
    }
}

void NodeHandler::ProcessLogData(const servicemanager::v5::LogData& logData)
{
    LOG_DBG() << "Processing log data" << Log::Field("nodeID", GetNodeID())
              << Log::Field("logID", logData.log_id().c_str()) << Log::Field("part", logData.part())
              << Log::Field("partCount", logData.part_count());

    auto pushLog = std::make_unique<PushLog>();

    if (auto err = common::pbconvert::ConvertFromProto(logData, GetNodeID(), *pushLog); !err.IsNone()) {
        LOG_ERR() << "Failed to convert log data from protobuf" << Log::Field(err);
        return;
    }

    if (auto err = mLogSender->SendLog(*pushLog); !err.IsNone()) {
        LOG_ERR() << "Failed to send log" << Log::Field(err);
        return;
    }
}

void NodeHandler::ProcessInstantMonitoring(const servicemanager::v5::InstantMonitoring& monitoring)
{
    LOG_DBG() << "Processing instant monitoring" << Log::Field("nodeID", GetNodeID());

    auto nodeMonitoringData = std::make_unique<aos::monitoring::NodeMonitoringData>();

    if (auto err = common::pbconvert::ConvertFromProto(monitoring, GetNodeID(), *nodeMonitoringData); !err.IsNone()) {
        LOG_ERR() << "Failed to convert instant monitoring from protobuf" << Log::Field(err);

        return;
    }

    if (auto err = mMonitoringReceiver->OnMonitoringReceived(*nodeMonitoringData); !err.IsNone()) {
        LOG_ERR() << "Failed to send instant monitoring to receiver" << Log::Field(err);

        return;
    }
}

void NodeHandler::ProcessAverageMonitoring(const servicemanager::v5::AverageMonitoring& monitoring)
{
    LOG_DBG() << "Processing average monitoring" << Log::Field("nodeID", GetNodeID());

    try {
        std::lock_guard<std::mutex> lock(mMutex);

        for (auto* msg : mMessages) {
            if (msg->mOutputMessage && msg->mOutputMessage->has_average_monitoring()) {
                std::lock_guard<std::mutex> msgLock(msg->mMutex);

                msg->mResponseReceived = true;
                msg->mCondVar.notify_one();
                msg->mOutputMessage->mutable_average_monitoring()->CopyFrom(monitoring);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERR() << "Failed to process node config status" << Log::Field("error", e.what());
    }
}

void NodeHandler::ProcessAlert(const servicemanager::v5::Alert& alert)
{
    LOG_DBG() << "Processing alert" << Log::Field("nodeID", GetNodeID());

    if (mAlertsReceiver) {
        auto aosAlert = std::make_unique<AlertVariant>();

        if (auto err = common::pbconvert::ConvertFromProto(alert, GetNodeID(), *aosAlert); !err.IsNone()) {
            LOG_ERR() << "Failed to convert alert from protobuf" << Log::Field(err);

            return;
        }

        mAlertsReceiver->OnAlertReceived(*aosAlert);
    }
}

Error NodeHandler::UpdateInstances(
    const Array<aos::InstanceInfo>& stopInstances, const Array<aos::InstanceInfo>& startInstances)
{
    LOG_DBG() << "Updating instances for node" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    auto*                                  updateInstances = inMsg.mutable_update_instances();

    if (auto err = common::pbconvert::ConvertToProto(stopInstances, startInstances, *updateInstances); !err.IsNone()) {
        return err;
    }

    if (auto err = SendMessage(inMsg); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error NodeHandler::GetAverageMonitoring(aos::monitoring::NodeMonitoringData& monitoring)
{
    LOG_DBG() << "Getting average monitoring data for node" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    servicemanager::v5::SMOutgoingMessages outMsg;

    inMsg.mutable_get_average_monitoring();
    auto* averageMonitoring = outMsg.mutable_average_monitoring();

    if (auto err = SendMessageSync(inMsg, outMsg); !err.IsNone()) {
        return err;
    }

    if (auto err = common::pbconvert::ConvertFromProto(*averageMonitoring, GetNodeID(), monitoring); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

void NodeHandler::OnConnect()
{
    LOG_DBG() << "Node connected" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    auto*                                  connectionStatus = inMsg.mutable_connection_status();
    connectionStatus->set_cloud_status(servicemanager::v5::CONNECTED);

    if (auto err = SendMessage(inMsg); !err.IsNone()) {
        LOG_ERR() << "Failed to send connection status" << Log::Field(err);
    }
}

void NodeHandler::OnDisconnect()
{
    LOG_DBG() << "Node disconnected" << Log::Field("nodeID", GetNodeID());

    servicemanager::v5::SMIncomingMessages inMsg;
    auto*                                  connectionStatus = inMsg.mutable_connection_status();
    connectionStatus->set_cloud_status(servicemanager::v5::DISCONNECTED);

    if (auto err = SendMessage(inMsg); !err.IsNone()) {
        LOG_ERR() << "Failed to send connection status" << Log::Field(err);
    }
}

} // namespace aos::cm::smcontroller
