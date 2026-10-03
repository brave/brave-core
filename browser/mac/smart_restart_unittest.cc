/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/mac/smart_restart.h"

#include <optional>

#include "chrome/browser/enterprise/browser_management/management_service_factory.h"
#include "chrome/browser/lifetime/restartability_monitor.h"
#include "chrome/browser/lifetime/smart_restart_policy.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/policy/core/common/management/scoped_management_service_override_for_testing.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace smart_restart {

class BraveSmartRestartPolicyTest : public testing::Test {
 public:
  BraveSmartRestartPolicyTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override { ASSERT_TRUE(profile_manager_.SetUp()); }
  void TearDown() override { SetSparkleIsUpdaterForTesting(std::nullopt); }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager profile_manager_;
  policy::ScopedManagementServiceOverrideForTesting platform_management_{
      policy::ManagementServiceFactory::GetForPlatform(),
      policy::EnterpriseManagementAuthority::NONE};
};

TEST_F(BraveSmartRestartPolicyTest, ZeroWindowProceedsWithoutSparkle) {
  SetSparkleIsUpdaterForTesting(false);
  EXPECT_TRUE(SmartRestartPolicy::CanZeroWindowRestartProceed());
}

TEST_F(BraveSmartRestartPolicyTest, ZeroWindowBlockedWithSparkle) {
  SetSparkleIsUpdaterForTesting(true);
  EXPECT_FALSE(SmartRestartPolicy::CanZeroWindowRestartProceed());
}

TEST_F(BraveSmartRestartPolicyTest, LockScreenProceedsWithoutSparkle) {
  SetSparkleIsUpdaterForTesting(false);
  EXPECT_EQ(ExtendedExecutionOutcome::kExecuted,
            SmartRestartPolicy::CanLockScreenRestartProceed(
                ExtendedRestartabilityState()));
}

TEST_F(BraveSmartRestartPolicyTest, LockScreenBlockedWithSparkle) {
  SetSparkleIsUpdaterForTesting(true);
  EXPECT_EQ(ExtendedExecutionOutcome::kBlockedByPolicy,
            SmartRestartPolicy::CanLockScreenRestartProceed(
                ExtendedRestartabilityState()));
}

}  // namespace smart_restart
