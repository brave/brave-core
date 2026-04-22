/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TEST_ADS_POLICY_BROWSERTEST_BASE_H_
#define BRAVE_BROWSER_BRAVE_ADS_TEST_ADS_POLICY_BROWSERTEST_BASE_H_

// Base fixture for browser tests that exercise BraveAdsDisabled group policy
// behavior. Subclasses configure the policy in
// SetUpInProcessBrowserTestFixture before the browser launches.

#include <memory>

#include "brave/browser/brave_ads/ads_service_waiter.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"

namespace brave_ads {

class AdsService;

class BraveAdsPolicyBrowserTestBase : public PlatformBrowserTest {
 public:
  BraveAdsPolicyBrowserTestBase();
  ~BraveAdsPolicyBrowserTestBase() override;

  void SetUpOnMainThread() override;

 protected:
  AdsService& GetAdsService();

  void SetUpBraveAdsPolicy(bool disable_ads);

  std::unique_ptr<test::AdsServiceWaiter> ads_service_waiter_;

 private:
  policy::MockConfigurationPolicyProvider configuration_policy_provider_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TEST_ADS_POLICY_BROWSERTEST_BASE_H_
