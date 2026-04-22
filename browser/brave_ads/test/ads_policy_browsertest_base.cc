/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/test/ads_policy_browsertest_base.h"

#include "base/check_deref.h"
#include "base/values.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "chrome/browser/ui/browser.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

BraveAdsPolicyBrowserTestBase::BraveAdsPolicyBrowserTestBase() = default;

BraveAdsPolicyBrowserTestBase::~BraveAdsPolicyBrowserTestBase() = default;

void BraveAdsPolicyBrowserTestBase::SetUpOnMainThread() {
  PlatformBrowserTest::SetUpOnMainThread();
  ads_service_waiter_ =
      std::make_unique<test::AdsServiceWaiter>(GetAdsService());
}

AdsService& BraveAdsPolicyBrowserTestBase::GetAdsService() {
  return CHECK_DEREF(AdsServiceFactory::GetForProfile(browser()->profile()));
}

void BraveAdsPolicyBrowserTestBase::SetUpBraveAdsPolicy(bool disable_ads) {
  EXPECT_CALL(configuration_policy_provider_,
              IsInitializationComplete(/*domain=*/::testing::_))
      .WillRepeatedly(::testing::Return(true));
  policy::BrowserPolicyConnector::SetPolicyProviderForTesting(
      &configuration_policy_provider_);
  policy::PolicyMap policies;
  policies.Set(policy::key::kBraveAdsDisabled, policy::POLICY_LEVEL_MANDATORY,
               policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_PLATFORM,
               base::Value(disable_ads),
               /*external_data_fetcher=*/nullptr);
  configuration_policy_provider_.UpdateChromePolicy(policies);
}

}  // namespace brave_ads
