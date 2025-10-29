/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_COMMON_PBCONVERT_SM_HPP_
#define AOS_COMMON_PBCONVERT_SM_HPP_

#include <servicemanager/v5/servicemanager.grpc.pb.h>

#include <core/cm/nodeinfoprovider/itf/sminforeceiver.hpp>
#include <core/cm/smcontroller/itf/updateimageprovider.hpp>
#include <core/common/monitoring/monitoring.hpp>
#include <core/common/types/alerts.hpp>
#include <core/common/types/envvars.hpp>
#include <core/common/types/instance.hpp>
#include <core/common/types/log.hpp>
#include <core/common/types/monitoring.hpp>
#include <core/common/types/network.hpp>
#include <core/common/types/unitconfig.hpp>

namespace aos::common::pbconvert {

/**
 * Converts ErrorInfo from grpc to aos.
 *
 * @param grpcError grpc error info.
 * @return Error.
 */
Error ConvertFromProto(const ::common::v2::ErrorInfo& grpcError);

/**
 * Converts NodeConfigStatus from grpc to aos.
 *
 * @param grpcStatus grpc node config status.
 * @param aosStatus aos node config status.
 */
Error ConvertFromProto(const servicemanager::v5::NodeConfigStatus& grpcStatus, NodeConfigStatus& aosStatus);

/**
 * Converts aos node config to grpc check node config message.
 *
 * @param config node config.
 * @param result check node config message.
 * @return Error
 */
Error ConvertToProto(const NodeConfig& config, servicemanager::v5::CheckNodeConfig& result);

/**
 * Converts aos node config to grpc set node config message.
 *
 * @param config node config.
 * @param result check node config message.
 * @return Error
 */
Error ConvertToProto(const NodeConfig& config, servicemanager::v5::SetNodeConfig& result);

/**
 * Converts grpc alert to aos alert item.
 *
 * @param grpcAlert grpc alert.
 * @param nodeID node ID.
 * @param alertItem aos alert item.
 * @return Error
 */
Error ConvertFromProto(const servicemanager::v5::Alert& grpcAlert, const String& nodeID, AlertVariant& alertItem);

/**
 * Converts aos request log to grpc system log request.
 *
 * @param log aos request log.
 * @param result grpc system log request.
 * @return Error
 */
Error ConvertToProto(const RequestLog& log, servicemanager::v5::SystemLogRequest& result);

/**
 * Converts aos request log to grpc instance log request.
 *
 * @param log aos request log.
 * @param result grpc instance log request.
 * @return Error
 */
Error ConvertToProto(const RequestLog& log, servicemanager::v5::InstanceLogRequest& result);

/**
 * Converts aos request log to grpc instance crash log request.
 *
 * @param log aos request log.
 * @param result grpc instance crash log request.
 * @return Error
 */
Error ConvertToProto(const RequestLog& log, servicemanager::v5::InstanceCrashLogRequest& result);

/**
 * Converts grpc log data to aos push log.
 *
 * @param grpcLogData grpc log data.
 * @param nodeID node ID.
 * @param aosPushLog aos push log.
 * @return Error
 */
Error ConvertFromProto(const servicemanager::v5::LogData& grpcLogData, const String& nodeID, PushLog& aosPushLog);

/**
 * Converts aos array of update network parameters to grpc update networks message.
 *
 * @param networkParams aos array of update network parameters.
 * @param result grpc update networks message.
 * @return Error
 */
Error ConvertToProto(const Array<UpdateNetworkParameters>& networkParams, servicemanager::v5::UpdateNetworks& result);

/**
 * Converts aos override env vars request to grpc override env vars message.
 *
 * @param envVars aos override env vars request.
 * @param result grpc override env vars message.
 * @return Error
 */
Error ConvertToProto(const OverrideEnvVarsRequest& envVars, servicemanager::v5::OverrideEnvVars& result);

/**
 * Converts grpc override env var status to aos override env vars statuses.
 *
 * @param grpcStatus grpc override env var status.
 * @param result aos override env vars statuses.
 * @return Error
 */
Error ConvertFromProto(const servicemanager::v5::OverrideEnvVarStatus& grpcStatus, OverrideEnvVarsStatuses& result);

/**
 * Converts aos instance info arrays to grpc update instances message.
 *
 * @param stopInstances aos instances to stop.
 * @param startInstances aos instances to start.
 * @param result grpc update instances message.
 * @return Error
 */
Error ConvertToProto(const Array<aos::InstanceInfo>& stopInstances, const Array<aos::InstanceInfo>& startInstances,
    servicemanager::v5::UpdateInstances& result);

/**
 * Converts aos image info to grpc item image info message.
 *
 * @param src aos image info.
 * @param dst grpc item image info message.
 * @return Error
 */
Error ConvertToProto(const aos::ImageInfo& src, servicemanager::v5::ItemImageInfo& dst);

/**
 * Converts aos update image info to grpc item image info message.
 *
 * @param src aos update image info.
 * @param dst grpc item image info message.
 * @return Error
 */
Error ConvertToProto(const cm::smcontroller::UpdateImageInfo& src, servicemanager::v5::ItemImageInfo& dst);

/**
 * Converts grpc average monitoring to aos node monitoring data.
 *
 * @param src grpc average monitoring.
 * @param nodeID node ID.
 * @param dst aos node monitoring data.
 * @return Error
 */
Error ConvertFromProto(
    const servicemanager::v5::AverageMonitoring& src, const String& nodeID, aos::monitoring::NodeMonitoringData& dst);

/**
 * Converts grpc instance status to aos instance status.
 *
 * @param src grpc instance status.
 * @param nodeID node ID to set in the result.
 * @param dst aos instance status.
 * @return Error
 */
Error ConvertFromProto(const servicemanager::v5::InstanceStatus& src, const String& nodeID, aos::InstanceStatus& dst);

/**
 * Converts grpc instant monitoring to aos node monitoring data.
 *
 * @param src grpc instant monitoring.
 * @param nodeID node ID to set in the result.
 * @param dst aos node monitoring data.
 * @return Error
 */
Error ConvertFromProto(
    const servicemanager::v5::InstantMonitoring& src, const String& nodeID, aos::monitoring::NodeMonitoringData& dst);

/**
 * Converts grpc SM info to aos SM info.
 *
 * @param src grpc SM info.
 * @param dst aos SM info.
 * @return Error
 */
Error ConvertFromProto(const servicemanager::v5::SMInfo& src, aos::cm::nodeinfoprovider::SMInfo& dst);

} // namespace aos::common::pbconvert

#endif
