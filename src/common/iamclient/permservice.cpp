/*
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <grpcpp/grpcpp.h>
#include <iamanager/v6/iamanager.grpc.pb.h>

#include <common/logger/logmodule.hpp>
#include <common/pbconvert/common.hpp>
#include <common/utils/exception.hpp>

#include "permservice.hpp"

namespace aos::common::iamclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error PermissionsService::Init(const std::string& IAMProtectedServerURL, const std::string& certStorage,
    TLSCredentialsItf& TLSCredentials, bool insecureConnection)
{
    LOG_DBG() << "Init permissions service" << Log::Field("IAMProtectedServerURL", IAMProtectedServerURL.c_str())
              << Log::Field("certStorage", certStorage.c_str()) << Log::Field("insecureConnection", insecureConnection);

    mCertStorage           = certStorage;
    mTLSCredentials        = &TLSCredentials;
    mIAMProtectedServerURL = IAMProtectedServerURL;

    auto [credentials, err] = mTLSCredentials->GetMTLSClientCredentials(mCertStorage.c_str(), insecureConnection);
    if (!err.IsNone()) {
        return err;
    }

    mCredentials = credentials;

    return ErrorEnum::eNone;
}

RetWithError<StaticString<cSecretLen>> PermissionsService::RegisterInstance(
    const InstanceIdent& instanceIdent, const Array<FunctionServicePermissions>& instancePermissions)
{
    LOG_INF() << "Register instance" << Log::Field("itemID", instanceIdent.mItemID)
              << Log::Field("subjectID", instanceIdent.mSubjectID) << Log::Field("instance", instanceIdent.mInstance);

    try {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMPermissionsService::NewStub(
            grpc::CreateCustomChannel(mIAMProtectedServerURL, mCredentials, grpc::ChannelArguments()));

        auto request = pbconvert::ConvertToProto(instanceIdent, instancePermissions);

        iamanager::v6::RegisterInstanceResponse response;

        if (auto status = stub->RegisterInstance(ctx.get(), request, &response); !status.ok()) {
            return {StaticString<cSecretLen>(), ErrorEnum::eRuntime};
        }

        return {response.secret().c_str(), ErrorEnum::eNone};
    } catch (const std::exception& e) {
        return {{}, AOS_ERROR_WRAP(utils::ToAosError(e, ErrorEnum::eRuntime))};
    }
}

Error PermissionsService::UnregisterInstance(const InstanceIdent& instanceIdent)
{
    LOG_INF() << "Unregister instance" << Log::Field("itemID", instanceIdent.mItemID)
              << Log::Field("subjectID", instanceIdent.mSubjectID) << Log::Field("instance", instanceIdent.mInstance);

    try {

        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->set_deadline(std::chrono::system_clock::now() + cServiceTimeout);

        auto stub = iamanager::v6::IAMPermissionsService::NewStub(
            grpc::CreateCustomChannel(mIAMProtectedServerURL, mCredentials, grpc::ChannelArguments()));

        iamanager::v6::UnregisterInstanceRequest request;
        request.mutable_instance()->CopyFrom(pbconvert::ConvertToProto(instanceIdent));

        google::protobuf::Empty response;

        if (auto status = stub->UnregisterInstance(ctx.get(), request, &response); !status.ok()) {
            return ErrorEnum::eRuntime;
        }

        return ErrorEnum::eNone;
    } catch (const std::exception& e) {
        return AOS_ERROR_WRAP(utils::ToAosError(e, ErrorEnum::eRuntime));
    }
}

} // namespace aos::common::iamclient
