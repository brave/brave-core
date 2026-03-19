/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/condition_matcher_pref_util.h"

#include "base/containers/span.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/test/local_state_pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConditionMatcherPrefUtilTest : public test::TestBase {};

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetBooleanProfilePrefValueAsString) {
  // Arrange
  test::RegisterProfileBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ("1", MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(),
                                           "boolean"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetIntegerProfilePrefValueAsString) {
  // Arrange
  test::RegisterProfileIntegerPref("integer", 123);

  // Act & Assert
  EXPECT_EQ("123", MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(),
                                             "integer"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetDoubleProfilePrefValueAsString) {
  // Arrange
  test::RegisterProfileDoublePref("double", 1.23);

  // Act & Assert
  EXPECT_EQ("1.23", MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(),
                                              "double"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetStringProfilePrefValueAsString) {
  // Arrange
  test::RegisterProfileStringPref("string", "foo");

  // Act & Assert
  EXPECT_EQ("foo", MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(),
                                             "string"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetBooleanLocalStatePrefValueAsString) {
  // Arrange
  test::RegisterLocalStateBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ("1", MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(),
                                           "boolean"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetVirtualPrefValueAsString) {
  // Arrange
  base::DictValue virtual_prefs;
  virtual_prefs.Set("[virtual]:foo", 42);

  // Act & Assert
  EXPECT_EQ("42", MaybeGetPrefValueAsString(virtual_prefs, "[virtual]:foo"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetDictProfilePrefValueAsString) {
  // Arrange
  test::RegisterProfileDictPref("dict", base::DictValue().Set("key", "bar"));

  // Act & Assert
  EXPECT_FALSE(
      MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(), "dict"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetBinaryVirtualPrefValueAsString) {
  // Arrange
  base::DictValue virtual_prefs;
  virtual_prefs.Set(
      "[virtual]:foo",
      base::Value(base::span<const uint8_t>({0x48, 0x65, 0x6C, 0x6C, 0x6F})));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValueAsString(virtual_prefs, "[virtual]:foo"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetListProfilePrefValueAsString) {
  // Arrange
  test::RegisterProfileListPref("list", base::ListValue().Append("foo"));

  // Act & Assert
  EXPECT_FALSE(
      MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(), "list"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, DoNotGetUnknownPrefValueAsString) {
  // Act & Assert
  EXPECT_FALSE(
      MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(), "foo.bar"));
  EXPECT_FALSE(MaybeGetPrefValueAsString(GetAdsClient().GetVirtualPrefs(), ""));
}

}  // namespace brave_ads
