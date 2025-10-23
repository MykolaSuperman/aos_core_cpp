/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <grpcpp/grpcpp.h>
#include <iamanager/v6/iamanager.grpc.pb.h>

#include <common/logger/logmodule.hpp>
#include <common/utils/exception.hpp>

#include "provisioningservice.hpp"

namespace aos::common::iamclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ProvisioningService::Init(const std::string& IAMProtectedServerURL, const std::string& certStorage,
    TLSCredentialsItf& TLSCredentials, bool insecureConnection)
{
    LOG_DBG() << "Init provisioning service" << Log::Field("IAMProtectedServerURL", IAMProtectedServerURL.c_str())
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

Error ProvisioningService::GetCertTypes(const String& nodeID, Array<StaticString<cCertTypeLen>>& certTypes) const
{
    LOG_INF() << "Get cert types" << Log::Field("nodeID", nodeID.CStr());

    try {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMProvisioningService::NewStub(
            grpc::CreateCustomChannel(mIAMProtectedServerURL, mCredentials, grpc::ChannelArguments()));

        iamanager::v6::GetCertTypesRequest request;
        iamanager::v6::CertTypes           response;

        request.set_node_id(nodeID.CStr());

        if (auto status = stub->GetCertTypes(ctx.get(), request, &response); !status.ok()) {
            return Error(ErrorEnum::eRuntime, status.error_message().c_str());
        }

        for (const auto& type : response.types()) {
            if (auto err = certTypes.EmplaceBack(type.c_str()); !err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }

        return ErrorEnum::eNone;
    } catch (const std::exception& e) {
        return AOS_ERROR_WRAP(utils::ToAosError(e, ErrorEnum::eRuntime));
    }
}

Error ProvisioningService::StartProvisioning(const String& nodeID, const String& password)
{
    LOG_INF() << "Start provisioning" << Log::Field("nodeID", nodeID.CStr());

    try {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMProvisioningService::NewStub(
            grpc::CreateCustomChannel(mIAMProtectedServerURL, mCredentials, grpc::ChannelArguments()));

        iamanager::v6::StartProvisioningRequest  request;
        iamanager::v6::StartProvisioningResponse response;

        request.set_node_id(nodeID.CStr());
        request.set_password(password.CStr());

        if (auto status = stub->StartProvisioning(ctx.get(), request, &response); !status.ok()) {
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

Error ProvisioningService::FinishProvisioning(const String& nodeID, const String& password)
{
    LOG_INF() << "Finish provisioning" << Log::Field("nodeID", nodeID.CStr());

    try {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMProvisioningService::NewStub(
            grpc::CreateCustomChannel(mIAMProtectedServerURL, mCredentials, grpc::ChannelArguments()));

        iamanager::v6::FinishProvisioningRequest  request;
        iamanager::v6::FinishProvisioningResponse response;

        request.set_node_id(nodeID.CStr());
        request.set_password(password.CStr());

        if (auto status = stub->FinishProvisioning(ctx.get(), request, &response); !status.ok()) {
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

Error ProvisioningService::Deprovision(const String& nodeID, const String& password)
{
    LOG_INF() << "Deprovision" << Log::Field("nodeID", nodeID.CStr());

    try {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMProvisioningService::NewStub(
            grpc::CreateCustomChannel(mIAMProtectedServerURL, mCredentials, grpc::ChannelArguments()));

        iamanager::v6::DeprovisionRequest  request;
        iamanager::v6::DeprovisionResponse response;

        request.set_node_id(nodeID.CStr());
        request.set_password(password.CStr());

        if (auto status = stub->Deprovision(ctx.get(), request, &response); !status.ok()) {
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
