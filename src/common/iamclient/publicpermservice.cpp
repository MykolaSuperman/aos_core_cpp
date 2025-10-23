/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <grpcpp/grpcpp.h>

#include <iamanager/v6/iamanager.grpc.pb.h>

#include <common/logger/logmodule.hpp>
#include <common/pbconvert/iam.hpp>
#include <common/utils/exception.hpp>

#include "publicpermservice.hpp"

namespace aos::common::iamclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error PublicPermissionsService::Init(
    std::string& iamPublicServerURL, TLSCredentialsItf& tlsCredentials, bool insecureConnection)
{
    LOG_DBG() << "Init public permissions service" << Log::Field("IAMPublicServerURL", iamPublicServerURL.c_str())
              << Log::Field("insecureConnection", insecureConnection);

    mTLSCredentials     = &tlsCredentials;
    mIAMPublicServerURL = iamPublicServerURL;

    auto [credentials, err] = mTLSCredentials->GetTLSClientCredentials(insecureConnection);
    if (!err.IsNone()) {
        return err;
    }

    mCredentials = credentials;

    return ErrorEnum::eNone;
}

Error PublicPermissionsService::GetPermissions(const String& secret, const String& funcServerID,
    InstanceIdent& instanceIdent, Array<FunctionPermissions>& servicePermissions)
{
    LOG_DBG() << "Get permissions" << Log::Field("funcServerID", funcServerID.CStr())
              << Log::Field("secret", secret.CStr()) << Log::Field("instanceIdent", instanceIdent);

    try {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMPublicPermissionsService::NewStub(
            grpc::CreateCustomChannel(mIAMPublicServerURL, mCredentials, grpc::ChannelArguments()));

        auto request = pbconvert::ConvertToProto(secret, funcServerID);

        iamanager::v6::PermissionsResponse response;

        if (auto status = stub->GetPermissions(ctx.get(), request, &response); !status.ok()) {
            return ErrorEnum::eRuntime;
        }

        return pbconvert::ConvertToAos(response, instanceIdent, servicePermissions);
    } catch (const std::exception& e) {
        return AOS_ERROR_WRAP(utils::ToAosError(e, ErrorEnum::eRuntime));
    }
}

} // namespace aos::common::iamclient
