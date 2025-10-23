/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_COMMON_IAMCLIENT_PERMSERVICE_HPP_
#define AOS_COMMON_IAMCLIENT_PERMSERVICE_HPP_

#include <chrono>
#include <memory>
#include <string>

#include <core/common/iamclient/itf/permhandler.hpp>

#include "itf/tlscredentials.hpp"

namespace aos::common::iamclient {
/**
 * Permissions service.
 */
class PermissionsService : public aos::iamclient::PermHandlerItf {
public:
    /**
     * Initializes permissions service handler.
     *
     * @param IAMProtectedServerURL IAM protected server URL.
     * @param certStorage certificate storage.
     * @param mTLSCredentials TLS credentials.
     * @return Error.
     */
    Error Init(const std::string& IAMProtectedServerURL, const std::string& certStorage,
        TLSCredentialsItf& TLSCredentials, bool insecureConnection = false);

    /**
     * Adds new service instance and its permissions into cache.
     *
     * @param instanceIdent instance identification.
     * @param instancePermissions instance permissions.
     * @returns RetWithError<StaticString<cSecretLen>>.
     */
    RetWithError<StaticString<cSecretLen>> RegisterInstance(
        const InstanceIdent& instanceIdent, const Array<FunctionServicePermissions>& instancePermissions) override;

    /**
     * Unregisters instance deletes service instance with permissions from cache.
     *
     * @param instanceIdent instance identification.
     * @returns Error.
     */
    Error UnregisterInstance(const InstanceIdent& instanceIdent) override;

private:
    static constexpr auto cServiceTimeout = std::chrono::seconds(10);

    TLSCredentialsItf*                        mTLSCredentials {};
    std::string                               mIAMProtectedServerURL;
    std::string                               mCertStorage;
    std::shared_ptr<grpc::ChannelCredentials> mCredentials;
};

} // namespace aos::common::iamclient

#endif
