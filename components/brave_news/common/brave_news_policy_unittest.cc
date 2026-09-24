// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

class BraveNewsPolicyTest : public testing::Test {
 public:
  BraveNewsPolicyTest() {
    prefs::RegisterProfilePrefs(pref_service_.registry());
  }

 protected:
  void BlockBraveNewsByPolicy(bool value) {
    pref_service_.SetManagedPref(prefs::kBraveNewsDisabledByPolicy,
                                 base::Value(value));
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(BraveNewsPolicyTest, PolicyDisablesBraveNews) {
  // Initially, policy should not be disabled
  EXPECT_FALSE(pref_service_.GetBoolean(prefs::kBraveNewsDisabledByPolicy));
  EXPECT_FALSE(
      pref_service_.IsManagedPreference(prefs::kBraveNewsDisabledByPolicy));

  // Set policy to disable Brave News
  BlockBraveNewsByPolicy(true);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(pref_service_.FindPreference(prefs::kBraveNewsDisabledByPolicy));
  EXPECT_TRUE(
      pref_service_.IsManagedPreference(prefs::kBraveNewsDisabledByPolicy));
  EXPECT_TRUE(pref_service_.GetBoolean(prefs::kBraveNewsDisabledByPolicy));

  // Test that IsEnabled returns false when policy disables it
  EXPECT_FALSE(IsEnabled(&pref_service_));
}

TEST_F(BraveNewsPolicyTest, PolicyEnablesBraveNews) {
  // Set policy to enable Brave News (policy value false = not disabled)
  BlockBraveNewsByPolicy(false);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(pref_service_.FindPreference(prefs::kBraveNewsDisabledByPolicy));
  EXPECT_TRUE(
      pref_service_.IsManagedPreference(prefs::kBraveNewsDisabledByPolicy));
  EXPECT_FALSE(pref_service_.GetBoolean(prefs::kBraveNewsDisabledByPolicy));

  // Set user preferences needed for IsEnabled to return true
  pref_service_.SetBoolean(prefs::kNewTabPageShowToday, true);
  pref_service_.SetBoolean(prefs::kBraveNewsOptedIn, true);

  // Test that IsEnabled returns true when policy allows it and user opts in
  EXPECT_TRUE(IsEnabled(&pref_service_));
}

TEST_F(BraveNewsPolicyTest, NoPolicyDefaultBehavior) {
  // Test that the policy preference exists but is not managed by default
  EXPECT_TRUE(pref_service_.FindPreference(prefs::kBraveNewsDisabledByPolicy));
  EXPECT_FALSE(
      pref_service_.IsManagedPreference(prefs::kBraveNewsDisabledByPolicy));
  EXPECT_FALSE(pref_service_.GetBoolean(prefs::kBraveNewsDisabledByPolicy));

  // Set user preferences needed for IsEnabled to return true
  pref_service_.SetBoolean(prefs::kNewTabPageShowToday, true);
  pref_service_.SetBoolean(prefs::kBraveNewsOptedIn, true);

  // Test that IsEnabled returns true when no policy is set and user opts in
  EXPECT_TRUE(IsEnabled(&pref_service_));
}

TEST_F(BraveNewsPolicyTest, PolicyOverridesUserPreferences) {
  // Set user preferences to enable Brave News
  pref_service_.SetBoolean(prefs::kNewTabPageShowToday, true);
  pref_service_.SetBoolean(prefs::kBraveNewsOptedIn, true);

  // Without policy, should be enabled
  EXPECT_TRUE(IsEnabled(&pref_service_));

  // Set policy to disable Brave News
  BlockBraveNewsByPolicy(true);

  // Test that policy overrides user preferences
  EXPECT_TRUE(pref_service_.GetBoolean(prefs::kBraveNewsDisabledByPolicy));
  EXPECT_FALSE(IsEnabled(&pref_service_));
}

}  // namespace brave_news
