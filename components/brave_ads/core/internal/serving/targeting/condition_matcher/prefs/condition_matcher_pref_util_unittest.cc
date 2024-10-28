/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/condition_matcher_pref_util.h"

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_pref_provider.h"
#include "brave/components/brave_ads/core/internal/common/test/local_state_pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConditionMatcherPrefUtilTest : public test::TestBase {
 protected:
  const AdsClientPrefProvider pref_provider_;
};

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetProfilePrefValueAsString) {
  // Arrange
  test::RegisterProfileBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ("1", MaybeGetPrefValueAsString(&pref_provider_, "boolean"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetLocalStatePrefValueAsString) {
  // Arrange
  test::RegisterLocalStateBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ("1", MaybeGetPrefValueAsString(&pref_provider_, "boolean"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, DoNotGetUnknownPrefValueAsString) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValueAsString(&pref_provider_, "foo.bar"));
  EXPECT_FALSE(MaybeGetPrefValueAsString(&pref_provider_, ""));
}

}  // namespace brave_ads
