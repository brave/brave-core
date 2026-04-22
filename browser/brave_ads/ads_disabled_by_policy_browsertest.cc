/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/test/ads_policy_browsertest_base.h"
#include "content/public/test/browser_test.h"

namespace brave_ads {

class BraveAdsDisabledByPolicyTest : public BraveAdsPolicyBrowserTestBase {
  void SetUpInProcessBrowserTestFixture() override {
    SetUpBraveAdsPolicy(/*disable_ads=*/true);
  }
};

IN_PROC_BROWSER_TEST_F(BraveAdsDisabledByPolicyTest, ServiceDoesNotStart) {
  ads_service_waiter_->WaitForOnAdsServiceIneligibleToStart();
}

}  // namespace brave_ads
