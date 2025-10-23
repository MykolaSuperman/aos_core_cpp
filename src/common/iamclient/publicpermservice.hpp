/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_COMMON_IAMCLIENT_PUBLICPERMSERVICE_HPP_
#define AOS_COMMON_IAMCLIENT_PUBLICPERMSERVICE_HPP_

#include <core/common/iamclient/itf/permprovider.hpp>

#include "itf/tlscredentials.hpp"

namespace aos::common::iamclient {
/**
 * Public permissions service.
 */
class PublicPermissionsService : public aos::iamclient::PermProviderItf {
public:
    /**
     * Initializes public permissions service.
     * @param iamPublicServerURL IAM public server URL.
     * @param certStorage certificate storage.
     * @param tlsCredentials TLS credentials.
     * @param insecureConnection whether to use insecure connection.
     * @return Error.
     */
    Error Init(std::string& iamPublicServerURL, TLSCredentialsItf& tlsCredentials, bool insecureConnection = false);

    /**
     * Returns instance ident and permissions by secret and functional server ID.
     *
     * @param secret secret.
     * @param funcServerID functional server ID.
     * @param[out] instanceIdent result instance ident.
     * @param[out] servicePermissions result service permission.
     * @returns Error.
     */
    Error GetPermissions(const String& secret, const String& funcServerID, InstanceIdent& instanceIdent,
        Array<FunctionPermissions>& servicePermissions) override;

private:
    static constexpr auto cServiceTimeout = std::chrono::seconds(10);

    TLSCredentialsItf*                        mTLSCredentials {};
    std::string                               mIAMPublicServerURL;
    std::shared_ptr<grpc::ChannelCredentials> mCredentials;
};
} // namespace aos::common::iamclient

#endif
