/*
 * Copyright (C) 2026 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstdio>

#include <core/common/tools/logger.hpp>
#include <core/common/tools/memory.hpp>

#include "bridgenetwork.hpp"

namespace aos::sm::networkmanager {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error BridgeNetwork::Init(InterfaceManagerItf& netIf)
{
    mNetIf = &netIf;

    return ErrorEnum::eNone;
}

Error BridgeNetwork::Attach(const String& instanceID, const BridgeParams& params, BridgeAttachResult& result)
{
    LOG_DBG() << "Attach bridge" << Log::Field("instanceID", instanceID) << Log::Field("bridge", params.mBridgeIfName);

    const auto hostName = HostVethName(instanceID);

    Error err;

    if (err = mNetIf->CreateVeth(hostName, params.mContainerIfName); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto cleanupVeth = DeferRelease(&hostName, [this, &err](const StaticString<cInterfaceLen>* host) {
        if (!err.IsNone()) {
            if (auto errDel = mNetIf->DeleteLink(*host); !errDel.IsNone()) {
                LOG_ERR() << "Failed to delete veth" << Log::Field(errDel);
            }
        }
    });

    if (err = mNetIf->SetMasterLink(hostName, params.mBridgeIfName); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = mNetIf->SetupLink(hostName); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = mNetIf->MoveLinkToNamespace(params.mContainerIfName, params.mNetNSPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    // The kernel administratively downs a link when it is moved to a new netns,
    // so bring the peer up inside the target namespace before adding the route
    // (a route via the gateway needs the connected subnet route, which only
    // exists while the interface is up).
    if (err = mNetIf->SetupLink(params.mContainerIfName, params.mNetNSPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = mNetIf->AddAddress(params.mContainerIfName, params.mIPWithMask, params.mNetNSPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = mNetIf->AddRoute(String("0.0.0.0/0"), params.mGateway, params.mNetNSPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (params.mHairpin) {
        if (err = mNetIf->SetHairpin(hostName, true); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
    }

    result.mHostIfName      = hostName;
    result.mContainerIfName = params.mContainerIfName;

    return ErrorEnum::eNone;
}

Error BridgeNetwork::Detach(const String& instanceID, const String& bridgeIfName)
{
    LOG_DBG() << "Detach bridge" << Log::Field("instanceID", instanceID) << Log::Field("bridge", bridgeIfName);

    const auto hostName = HostVethName(instanceID);

    if (auto errDel = mNetIf->DeleteLink(hostName); !errDel.IsNone()) {
        LOG_ERR() << "Failed to delete host veth" << Log::Field(errDel);

        return AOS_ERROR_WRAP(errDel);
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

StaticString<cInterfaceLen> BridgeNetwork::HostVethName(const String& instanceID)
{
    // FNV-1a 32-bit — deterministic so Detach can recompute without persisting the name.
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < instanceID.Size(); ++i) {
        hash ^= static_cast<uint8_t>(instanceID[i]);
        hash *= 16777619u;
    }

    // "veth" + 8 hex = 12 chars, within IFNAMSIZ-1 (15).
    StaticString<cInterfaceLen> name;

    name.Format("veth%08x", hash);

    return name;
}

} // namespace aos::sm::networkmanager
