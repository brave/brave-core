/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/enterprise/platform_auth/mock_platform_auth_provider.h"
#include "chrome/browser/enterprise/platform_auth/platform_auth_navigation_throttle.h"
#include "chrome/browser/enterprise/platform_auth/platform_auth_provider_manager.h"
#include "chrome/browser/enterprise/platform_auth/scoped_set_provider_for_testing.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "content/public/test/test_renderer_host.h"
#include "content/test/storage_partition_test_helpers.h"
#include "content/test/test_web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::_;

namespace containers {
namespace {

void EnableManager(enterprise_auth::PlatformAuthProviderManager& manager,
                   bool enabled) {
  base::test::TestFuture<void> future;
  manager.SetEnabled(enabled, future.GetCallback());
  EXPECT_TRUE(future.Wait());
  EXPECT_EQ(manager.IsEnabled(), enabled);
}

}  // namespace

class PlatformAuthNavigationThrottleContainersTest : public testing::Test {
 public:
  PlatformAuthNavigationThrottleContainersTest()
      : mock_provider_(owned_provider_.get()) {
    scoped_feature_list_.InitAndEnableFeature(features::kContainers);
    ON_CALL(*mock_provider_, Die()).WillByDefault([this]() {
      mock_provider_ = nullptr;
    });
    EXPECT_CALL(*mock_provider_, Die());
  }

 protected:
  void SetUp() override {
    EXPECT_CALL(*mock_provider_, SupportsOriginFiltering())
        .WillOnce(::testing::Return(true));
  }

  std::unique_ptr<enterprise_auth::PlatformAuthProvider> TakeProvider() {
    return std::move(owned_provider_);
  }

  enterprise_auth::MockPlatformAuthProvider* mock_provider() {
    return mock_provider_;
  }

  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  content::RenderViewHostTestEnabler rvh_test_enabler_;
  TestingProfile profile_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<
      ::testing::StrictMock<enterprise_auth::MockPlatformAuthProvider>>
      owned_provider_{std::make_unique<
          ::testing::StrictMock<enterprise_auth::MockPlatformAuthProvider>>()};
  raw_ptr<::testing::StrictMock<enterprise_auth::MockPlatformAuthProvider>>
      mock_provider_ = nullptr;
};

TEST_F(PlatformAuthNavigationThrottleContainersTest,
       MaybeCreateAndAdd_SkipsContainerTabs) {
  EXPECT_CALL(*mock_provider(), FetchOrigins(_))
      .WillOnce([](enterprise_auth::PlatformAuthProvider::FetchOriginsCallback
                       callback) { std::move(callback).Run(nullptr); });
  enterprise_auth::ScopedSetProviderForTesting set_provider(TakeProvider());
  EnableManager(enterprise_auth::PlatformAuthProviderManager::GetInstance(),
                true);

  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false, kContainersStoragePartitionDomain,
      /*partition_name=*/"test-container");
  scoped_refptr<content::SiteInstance> site =
      content::SiteInstance::CreateForFixedStoragePartition(
          &profile_, GURL("https://www.example.test/"), config);
  std::unique_ptr<content::TestWebContents> web_contents =
      content::TestWebContents::Create(&profile_, site);

  content::MockNavigationHandle test_handle(
      GURL("https://www.example.test/"), web_contents->GetPrimaryMainFrame());
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);

  enterprise_auth::PlatformAuthNavigationThrottle::MaybeCreateAndAdd(registry);
  EXPECT_EQ(registry.throttles().size(), 0u);
}

TEST_F(PlatformAuthNavigationThrottleContainersTest,
       MaybeCreateAndAdd_CreatesThrottleForNonContainerTabs) {
  EXPECT_CALL(*mock_provider(), FetchOrigins(_))
      .WillOnce([](enterprise_auth::PlatformAuthProvider::FetchOriginsCallback
                       callback) { std::move(callback).Run(nullptr); });
  enterprise_auth::ScopedSetProviderForTesting set_provider(TakeProvider());
  EnableManager(enterprise_auth::PlatformAuthProviderManager::GetInstance(),
                true);

  scoped_refptr<content::SiteInstance> site =
      content::SiteInstance::Create(&profile_);
  std::unique_ptr<content::TestWebContents> web_contents =
      content::TestWebContents::Create(&profile_, site);

  content::MockNavigationHandle test_handle(
      GURL("https://www.example.test/"), web_contents->GetPrimaryMainFrame());
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);

  enterprise_auth::PlatformAuthNavigationThrottle::MaybeCreateAndAdd(registry);
  EXPECT_EQ(registry.throttles().size(), 1u);
}

}  // namespace containers
