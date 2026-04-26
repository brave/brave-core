/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Verifies that the BraveAdsDisabled administrator policy prevents or permits
// the Ads service from initializing.

#include "base/check_deref.h"
#include "base/values.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/ads_service_waiter.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_ads {

class BraveAdsPolicyBrowserTest : public PlatformBrowserTest,
                                  public ::testing::WithParamInterface<bool> {
 public:
  BraveAdsPolicyBrowserTest() = default;
  ~BraveAdsPolicyBrowserTest() override = default;

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(configuration_policy_provider_,
                IsInitializationComplete(::testing::_))
        .WillRepeatedly(::testing::Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(
        &configuration_policy_provider_);
    policy::PolicyMap policies;
    policies.Set(policy::key::kBraveAdsDisabled, policy::POLICY_LEVEL_MANDATORY,
                 policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_PLATFORM,
                 base::Value(IsAdsDisabledByPolicy()),
                 /*external_data_fetcher=*/nullptr);
    configuration_policy_provider_.UpdateChromePolicy(policies);
  }

  bool IsAdsDisabledByPolicy() { return GetParam(); }

  AdsService& GetAdsService() {
    return CHECK_DEREF(AdsServiceFactory::GetForProfile(browser()->profile()));
  }

 private:
  policy::MockConfigurationPolicyProvider configuration_policy_provider_;
};

IN_PROC_BROWSER_TEST_P(BraveAdsPolicyBrowserTest,
                       ServiceInitializationRespectsPolicy) {
  test::AdsServiceWaiter waiter(GetAdsService());
  if (IsAdsDisabledByPolicy()) {
    waiter.WaitForOnAdsServiceIneligibleToStart();
    EXPECT_FALSE(GetAdsService().IsInitialized());
  } else {
    waiter.WaitForOnDidInitializeAdsService();
    EXPECT_TRUE(GetAdsService().IsInitialized());
  }
}

INSTANTIATE_TEST_SUITE_P(
    BraveAdsPolicyBrowserTest,
    BraveAdsPolicyBrowserTest,
    ::testing::Bool(),
    [](const testing::TestParamInfo<BraveAdsPolicyBrowserTest::ParamType>&
           info) {
      return absl::StrFormat("BraveAds_%sByPolicy",
                             info.param ? "Disabled" : "NotDisabled");
    });

}  // namespace brave_ads
