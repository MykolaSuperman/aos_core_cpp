/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_COMMON_IAMCLIENT_NODESSERVICE_HPP_
#define AOS_COMMON_IAMCLIENT_NODESSERVICE_HPP_

#include <memory>

#include <core/common/iamclient/itf/nodehandler.hpp>

#include "itf/tlscredentials.hpp"

namespace aos::common::iamclient {
/**
 * Nodes service.
 */
class NodesService : public aos::iamclient::NodeHandlerItf {
public:
    /**
     * Initializes nodes service.
     *
     * @param IAMProtectedServerURL IAM protected server URL.
     * @param certStorage certificate storage.
     * @param TLSCredentials TLS credentials.
     * @param insecureConnection use insecure connection.
     * @return Error.
     */
    Error Init(const std::string& IAMProtectedServerURL, const std::string& certStorage,
        TLSCredentialsItf& TLSCredentials, bool insecureConnection = false);

    /**
     * Pauses node.
     *
     * @param nodeID node ID.
     * @returns Error.
     */
    Error PauseNode(const String& nodeID) override;

    /**
     * Resumes node.
     *
     * @param nodeID node ID.
     * @returns Error.
     */
    Error ResumeNode(const String& nodeID) override;

private:
    static constexpr auto cServiceTimeout = std::chrono::seconds(10);

    std::string                               mIAMProtectedServerURL;
    std::shared_ptr<grpc::ChannelCredentials> mCredentials;
    TLSCredentialsItf*                        mTLSCredentials {};
};

} // namespace aos::common::iamclient
#endif
