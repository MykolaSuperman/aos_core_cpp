/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <common/iamclient/publicpermservice.hpp>
#include <common/utils/exception.hpp>
#include <core/common/tests/utils/log.hpp>

#include "mocks/tlscredentialsmock.hpp"
#include "stubs/iampublicpermissionsservicestub.hpp"

using namespace testing;
using namespace aos::common::iamclient;

/***********************************************************************************************************************
 * Test Suite
 **********************************************************************************************************************/

class PublicPermissionsServiceTest : public Test {
protected:
    void SetUp() override
    {
        aos::tests::utils::InitLog();

        mStub = std::make_unique<IAMPublicPermissionsServiceStub>();

        EXPECT_CALL(mTLSCredentialsMock, GetTLSClientCredentials(_))
            .WillRepeatedly(Return(aos::RetWithError<std::shared_ptr<grpc::ChannelCredentials>> {
                grpc::InsecureChannelCredentials(), aos::ErrorEnum::eNone}));

        mService = std::make_unique<PublicPermissionsService>();

        std::string url = "localhost:8012";
        auto        err = mService->Init(url, mTLSCredentialsMock, true);
        ASSERT_EQ(err, aos::ErrorEnum::eNone);
    }

    void TearDown() override
    {
        mService.reset();
        mStub.reset();
    }

    std::unique_ptr<IAMPublicPermissionsServiceStub> mStub;
    std::unique_ptr<PublicPermissionsService>        mService;
    TLSCredentialsMock                               mTLSCredentialsMock;
};

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

TEST_F(PublicPermissionsServiceTest, GetPermissions)
{
    mStub->SetInstanceIdent("app1", "user1", 123);
    mStub->SetPermissions({"func1", "func2", "func3"});

    aos::InstanceIdent                             instanceIdent;
    aos::StaticArray<aos::FunctionPermissions, 10> servicePermissions;

    auto err = mService->GetPermissions("secret123", "funcServer1", instanceIdent, servicePermissions);

    EXPECT_EQ(err, aos::ErrorEnum::eNone);
    EXPECT_STREQ(instanceIdent.mItemID.CStr(), "app1");
    EXPECT_STREQ(instanceIdent.mSubjectID.CStr(), "user1");
    EXPECT_EQ(instanceIdent.mInstance, 123);

    EXPECT_EQ(servicePermissions.Size(), 3);

    EXPECT_TRUE(std::any_of(servicePermissions.begin(), servicePermissions.end(),
        [](const auto& perm) { return strcmp(perm.mFunction.CStr(), "func1") == 0; }));
    EXPECT_TRUE(std::any_of(servicePermissions.begin(), servicePermissions.end(),
        [](const auto& perm) { return strcmp(perm.mFunction.CStr(), "func2") == 0; }));
    EXPECT_TRUE(std::any_of(servicePermissions.begin(), servicePermissions.end(),
        [](const auto& perm) { return strcmp(perm.mFunction.CStr(), "func3") == 0; }));

    EXPECT_STREQ(mStub->GetLastSecret().c_str(), "secret123");
    EXPECT_STREQ(mStub->GetLastFuncServerID().c_str(), "funcServer1");
}

TEST_F(PublicPermissionsServiceTest, GetPermissionsEmpty)
{
    mStub->SetInstanceIdent("app2", "user2", 456);
    mStub->SetPermissions({});

    aos::InstanceIdent                             instanceIdent;
    aos::StaticArray<aos::FunctionPermissions, 10> servicePermissions;

    auto err = mService->GetPermissions("secret456", "funcServer2", instanceIdent, servicePermissions);

    EXPECT_EQ(err, aos::ErrorEnum::eNone);
    EXPECT_STREQ(instanceIdent.mItemID.CStr(), "app2");
    EXPECT_STREQ(instanceIdent.mSubjectID.CStr(), "user2");
    EXPECT_EQ(instanceIdent.mInstance, 456);
    EXPECT_EQ(servicePermissions.Size(), 0);
    EXPECT_STREQ(mStub->GetLastSecret().c_str(), "secret456");
    EXPECT_STREQ(mStub->GetLastFuncServerID().c_str(), "funcServer2");
}
