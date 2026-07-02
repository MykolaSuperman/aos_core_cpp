/*
 * Copyright (C) 2026 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_SM_NETWORKMANAGER_FIREWALL_HPP_
#define AOS_SM_NETWORKMANAGER_FIREWALL_HPP_

#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <utility>

#include <core/common/tools/noncopyable.hpp>
#include <core/sm/networkmanager/itf/firewall.hpp>

#include <sm/nftables/itf/firewallbackend.hpp>

namespace aos::sm::networkmanager {

/**
 * Native firewall implementation backed by an FWBackendItf.
 */
class Firewall : public FirewallItf, private NonCopyable {
public:
    /**
     * Initializes the firewall.
     *
     * @param backend firewall backend.
     * @return Error.
     */
    Error Init(nftables::FWBackendItf& backend);

    /**
     * Starts the firewall: ensures the table and base chains are in place.
     *
     * @return Error.
     */
    Error Start() override;

    /**
     * Stops the firewall: removes the rules and chains it added.
     *
     * @return Error.
     */
    Error Stop() override;

    /**
     * Adds a per-instance chain with input/output rules.
     *
     * @param instanceID instance id.
     * @param params per-instance firewall parameters.
     * @return Error.
     */
    Error AddInstance(const String& instanceID, const InstanceFirewallParams& params) override;

    /**
     * Removes the per-instance chain.
     *
     * @param instanceID instance id.
     * @return Error.
     */
    Error RemoveInstance(const String& instanceID) override;

    /**
     * Atomically replaces the per-instance chain content.
     *
     * @param instanceID instance id.
     * @param params new per-instance firewall parameters.
     * @return Error.
     */
    Error UpdateInstance(const String& instanceID, const InstanceFirewallParams& params) override;

    /**
     * Adds an IPMasq rule.
     *
     * @param subnet source subnet (CIDR).
     * @param outIf output interface.
     * @return Error.
     */
    Error AddMasquerade(const String& subnet, const String& outIf) override;

    /**
     * Removes the IPMasq rule.
     *
     * @param subnet source subnet (CIDR).
     * @param outIf output interface.
     * @return Error.
     */
    Error RemoveMasquerade(const String& subnet, const String& outIf) override;

    /**
     * Begins batch mode: RemoveInstance accumulates its nft deletes into a
     * single shared transaction instead of committing per instance.
     *
     * @return Error.
     */
    Error BeginBatch() override;

    /**
     * Commits the accumulated batch transaction and leaves batch mode.
     *
     * @return Error.
     */
    Error FlushBatch() override;

private:
    static constexpr auto cTableName           = "aos";
    static constexpr auto cForwardChain        = "forward";
    static constexpr auto cPostroutingChain    = "postrouting";
    static constexpr auto cForwardPriority     = 0;
    static constexpr auto cNATPriority         = 100;
    static constexpr auto cInstanceChainPrefix = "instance_";

    static std::string ChainName(const String& instanceID);

    Error CreateSkeleton();
    Error ReconcileArtifacts(const std::vector<nftables::FWListedRule>& forwardRules);

    const std::string                             mTable {cTableName};
    nftables::FWBackendItf*                       mBackend {};
    std::set<std::pair<std::string, std::string>> mMasqueradeRules;

    // Per-instance forward-chain jump handles (chain -> {inbound, outbound}),
    // captured at AddInstance/UpdateInstance so RemoveInstance can delete them
    // by handle without re-listing the whole forward chain. Guarded by mMutex
    // as instances are added/removed concurrently from the launcher's workers.
    std::mutex                                                                                 mMutex;
    std::unordered_map<std::string, std::pair<nftables::FWRuleHandle, nftables::FWRuleHandle>> mInstanceJumps;

    // Batch teardown: when active, RemoveInstance appends its deletes here
    // (serialized by mMutex) instead of committing, and FlushBatch commits the
    // lot in one nft transaction. Guarded by mMutex.
    bool                                mBatchMode {false};
    std::unique_ptr<nftables::FWTxnItf> mBatchTxn;
};

} // namespace aos::sm::networkmanager

#endif
