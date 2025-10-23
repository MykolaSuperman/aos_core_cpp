/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <grpcpp/grpcpp.h>

#include <common/logger/logmodule.hpp>
#include <common/pbconvert/common.hpp>
#include <common/utils/exception.hpp>
#include <iamanager/v6/iamanager.grpc.pb.h>

#include "nodesservice.hpp"

namespace aos::common::iamclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error NodesService::Init(const std::string& IAMProtectedServerURL, const std::string& certStorage,
    TLSCredentialsItf& TLSCredentials, bool insecureConnection)
{
    LOG_DBG() << "Init nodes service" << Log::Field("IAMProtectedServerURL", IAMProtectedServerURL.c_str())
              << Log::Field("certStorage", certStorage.c_str()) << Log::Field("insecureConnection", insecureConnection);

    mTLSCredentials        = &TLSCredentials;
    mIAMProtectedServerURL = IAMProtectedServerURL;

    auto [credentials, err] = mTLSCredentials->GetMTLSClientCredentials(certStorage.c_str(), insecureConnection);
    if (!err.IsNone()) {
        return err;
    }

    mCredentials = credentials;

    return ErrorEnum::eNone;
}

Error NodesService::PauseNode(const String& nodeID)
{
    LOG_INF() << "Pause node" << Log::Field("nodeID", nodeID.CStr());

    try {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMNodesService::NewStub(
            grpc::CreateCustomChannel(mIAMProtectedServerURL, mCredentials, grpc::ChannelArguments()));

        iamanager::v6::PauseNodeRequest  request;
        iamanager::v6::PauseNodeResponse response;

        request.set_node_id(nodeID.CStr());

        if (auto status = stub->PauseNode(ctx.get(), request, &response); !status.ok()) {
            return Error(ErrorEnum::eRuntime, status.error_message().c_str());
        }

        if (response.has_error()) {
            return Error(response.error().exit_code(), response.error().message().c_str());
        }

        return ErrorEnum::eNone;
    } catch (const std::exception& e) {
        return AOS_ERROR_WRAP(utils::ToAosError(e, ErrorEnum::eRuntime));
    }
}

Error NodesService::ResumeNode(const String& nodeID)
{
    LOG_INF() << "Resume node" << Log::Field("nodeID", nodeID.CStr());

    try {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMNodesService::NewStub(
            grpc::CreateCustomChannel(mIAMProtectedServerURL, mCredentials, grpc::ChannelArguments()));

        iamanager::v6::ResumeNodeRequest  request;
        iamanager::v6::ResumeNodeResponse response;

        request.set_node_id(nodeID.CStr());

        if (auto status = stub->ResumeNode(ctx.get(), request, &response); !status.ok()) {
            return Error(ErrorEnum::eRuntime, status.error_message().c_str());
        }

        if (response.has_error()) {
            return Error(response.error().exit_code(), response.error().message().c_str());
        }

        return ErrorEnum::eNone;
    } catch (const std::exception& e) {
        return AOS_ERROR_WRAP(utils::ToAosError(e, ErrorEnum::eRuntime));
    }
}

} // namespace aos::common::iamclient
