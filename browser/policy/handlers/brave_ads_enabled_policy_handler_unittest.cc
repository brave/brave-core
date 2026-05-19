/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/handlers/brave_ads_enabled_policy_handler.h"

#include <optional>
#include <string_view>

#include "base/values.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/policy/core/browser/configuration_policy_handler_parameters.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

constexpr std::string_view kBraveAdsTypePrefs[] = {
    brave_ads::prefs::kOptedInToNotificationAds,
    brave_ads::prefs::kOptedInToSearchResultAds,
    ntp_background_images::prefs::
        kNewTabPageShowSponsoredImagesBackgroundImage};

}  // namespace

class BraveAdsEnabledPolicyHandlerTest : public testing::Test {
 protected:
  void SetAndApplyPolicy(std::optional<bool> policy_enabled) {
    PolicyMap policies;
    if (policy_enabled.has_value()) {
      policies.Set(key::kBraveAdsEnabled, POLICY_LEVEL_MANDATORY,
                   POLICY_SCOPE_USER, POLICY_SOURCE_PLATFORM,
                   base::Value(*policy_enabled),
                   /*external_data_fetcher=*/nullptr);
    }
    PolicyHandlerParameters params;
    handler_.ApplyPolicySettingsWithParameters(policies, params, &prefs_);
  }

  PrefValueMap& prefs() { return prefs_; }

 private:
  PrefValueMap prefs_;
  BraveAdsEnabledPolicyHandler handler_;
};

TEST_F(BraveAdsEnabledPolicyHandlerTest,
       PolicySettingFalseDisablesAllBraveAdsTypes) {
  SetAndApplyPolicy(/*policy_enabled=*/false);

  for (const std::string_view pref : kBraveAdsTypePrefs) {
    bool pref_value;
    ASSERT_TRUE(prefs().GetBoolean(pref, &pref_value));
    EXPECT_FALSE(pref_value);
  }
}

TEST_F(BraveAdsEnabledPolicyHandlerTest,
       PolicySettingTrueDoesNotAffectBraveAdsTypes) {
  SetAndApplyPolicy(/*policy_enabled=*/true);

  for (const std::string_view pref : kBraveAdsTypePrefs) {
    bool pref_value;
    EXPECT_FALSE(prefs().GetBoolean(pref, &pref_value));
  }
}

TEST_F(BraveAdsEnabledPolicyHandlerTest,
       PolicyUnsetDoesNotAffectBraveAdsTypes) {
  SetAndApplyPolicy(/*policy_enabled=*/std::nullopt);

  for (const std::string_view pref : kBraveAdsTypePrefs) {
    bool pref_value;
    EXPECT_FALSE(prefs().GetBoolean(pref, &pref_value));
  }
}

}  // namespace policy
