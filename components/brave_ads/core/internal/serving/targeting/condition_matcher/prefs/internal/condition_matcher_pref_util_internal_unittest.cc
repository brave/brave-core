/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_pref_util_internal.h"

#include "base/containers/span.h"
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

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotConvertNoneValueTypeToString) {
  // Act & Assert
  EXPECT_FALSE(ToString(base::Value()));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, BooleanValueTypeToString) {
  // Act & Assert
  EXPECT_EQ("0", ToString(base::Value(false)));
  EXPECT_EQ("1", ToString(base::Value(true)));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, IntegerValueTypeToString) {
  // Act & Assert
  EXPECT_EQ("123", ToString(base::Value(123)));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, DoubleValueTypeToString) {
  // Act & Assert
  EXPECT_EQ("1.23", ToString(base::Value(1.23)));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, StringValueTypeToString) {
  // Act & Assert
  EXPECT_EQ("123", ToString(base::Value("123")));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotConvertDictValueTypeToString) {
  // Act & Assert
  EXPECT_FALSE(ToString(base::Value(base::Value::Dict().Set("foo", "bar"))));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotConvertListValueTypeToString) {
  // Act & Assert
  EXPECT_FALSE(ToString(base::Value(base::Value::List().Append("foo"))));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotConvertBinaryValueTypeToString) {
  // Arrange
  // Act & Assert
  EXPECT_FALSE(ToString(base::Value(
      base::span<const uint8_t>({0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x57,
                                 0x6F, 0x72, 0x6C, 0x64, 0x21}))));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetVirtualPrefValue) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::Value::Dict().Set("[virtual]:matrix", /*room*/ 303);
  });

  // Act & Assert
  EXPECT_EQ(base::Value(/*room*/ 303),
            MaybeGetPrefValue(&pref_provider_, "[virtual]:matrix"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, DoNotGetUnknownVirtualPrefValue) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::Value::Dict().Set("[virtual]:inverse.matrices", /*room*/ 101);
  });

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "[virtual]:matrix"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetBooleanProfilePrefValue) {
  // Arrange
  test::RegisterProfileBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ(base::Value(true), MaybeGetPrefValue(&pref_provider_, "boolean"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetIntegerProfilePrefValue) {
  // Arrange
  test::RegisterProfileIntegerPref("integer", 123);

  // Act & Assert
  EXPECT_EQ(base::Value(123), MaybeGetPrefValue(&pref_provider_, "integer"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetDoubleProfilePrefValue) {
  // Arrange
  test::RegisterProfileDoublePref("double", 1.23);

  // Act & Assert
  EXPECT_EQ(base::Value(1.23), MaybeGetPrefValue(&pref_provider_, "double"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetStringProfilePrefValue) {
  // Arrange
  test::RegisterProfileStringPref("string", "foo");

  // Act & Assert
  EXPECT_EQ(base::Value("foo"), MaybeGetPrefValue(&pref_provider_, "string"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetDictProfilePrefValue) {
  // Arrange
  test::RegisterProfileDictPref("dict", base::Value::Dict().Set("foo", "bar"));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "dict|foo"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetNestedDictProfilePrefValue) {
  // Arrange
  test::RegisterProfileDictPref(
      "dict",
      base::Value::Dict().Set("foo", base::Value::Dict().Set("bar", "qux")));

  // Act & Assert
  EXPECT_EQ(base::Value("qux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo|bar"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetDeeplyNestedDictProfilePrefValue) {
  // Arrange
  test::RegisterProfileDictPref(
      "dict", base::Value::Dict().Set(
                  "foo", base::Value::List().Append("bar").Append(
                             base::Value::Dict().Set("baz", "qux"))));

  // Act & Assert
  EXPECT_EQ(base::Value("qux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo|1|baz"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetNestedDictProfilePrefValueWithDotSeparatedPathComponents) {
  // Arrange
  test::RegisterProfileDictPref(
      "dict", base::Value::Dict().Set(
                  "foo.bar", base::Value::Dict().Set("baz.qux", "quux")));

  // Act & Assert
  EXPECT_EQ(base::Value("quux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo.bar|baz.qux"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetMalformedDictProfilePrefValue) {
  test::RegisterProfileDictPref("dict",
                                base::Value::Dict().Set("foo.bar", "baz"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "dict|foo.bar|baz"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "dict|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, ""));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetListProfilePrefValue) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append("foo").Append("bar"));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "list|1"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetNestedListProfilePrefValue) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append(
                  base::Value::List().Append("foo").Append("bar")));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "list|0|1"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetDeeplyNestedListProfilePrefValue) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append(base::Value::Dict().Set(
                  "foo", base::Value::List().Append("bar").Append("baz"))));

  // Act & Assert
  EXPECT_EQ(base::Value("baz"),
            MaybeGetPrefValue(&pref_provider_, "list|0|foo|1"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetNestedListProfilePrefValueWithDotSeparatedPathComponents) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append(base::Value::Dict().Set(
                  "foo.bar",
                  base::Value::List().Append("baz.qux").Append("quux.corge"))));

  // Act & Assert
  EXPECT_EQ(base::Value("quux.corge"),
            MaybeGetPrefValue(&pref_provider_, "list|0|foo.bar|1"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetListProfilePrefValueWithOutOfBoundsListIndicies) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append("foo").Append("bar"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|-1"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|2"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetMalformedListProfilePrefValue) {
  test::RegisterProfileListPref("list", base::Value::List().Append("foo"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|0|foo"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|bar"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, ""));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetBooleanLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ(base::Value(true), MaybeGetPrefValue(&pref_provider_, "boolean"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetIntegerLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateIntegerPref("integer", 123);

  // Act & Assert
  EXPECT_EQ(base::Value(123), MaybeGetPrefValue(&pref_provider_, "integer"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetDoubleLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateDoublePref("double", 1.23);

  // Act & Assert
  EXPECT_EQ(base::Value(1.23), MaybeGetPrefValue(&pref_provider_, "double"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetStringLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateStringPref("string", "foo");

  // Act & Assert
  EXPECT_EQ(base::Value("foo"), MaybeGetPrefValue(&pref_provider_, "string"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetDictLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateDictPref("dict",
                                   base::Value::Dict().Set("foo.bar", "baz"));

  // Act & Assert
  EXPECT_EQ(base::Value("baz"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo.bar"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetNestedDictLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateDictPref(
      "dict",
      base::Value::Dict().Set("foo", base::Value::Dict().Set("bar", "qux")));

  // Act & Assert
  EXPECT_EQ(base::Value("qux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo|bar"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetDeeplyNestedDictLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateDictPref(
      "dict", base::Value::Dict().Set(
                  "foo", base::Value::List().Append("bar").Append(
                             base::Value::Dict().Set("baz", "qux"))));

  // Act & Assert
  EXPECT_EQ(base::Value("qux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo|1|baz"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetNestedDictLocalStatePrefValueWithDotSeparatedPathComponents) {
  // Arrange
  test::RegisterLocalStateDictPref(
      "dict", base::Value::Dict().Set(
                  "foo.bar", base::Value::Dict().Set("baz.qux", "quux")));

  // Act & Assert
  EXPECT_EQ(base::Value("quux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo.bar|baz.qux"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetMalformedDictLocalStatePrefValue) {
  test::RegisterLocalStateDictPref("dict",
                                   base::Value::Dict().Set("foo.bar", "baz"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "dict|foo.bar|baz"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "dict|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, ""));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetListLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append("foo").Append("bar"));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "list|1"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, GetNestedListLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append(
                  base::Value::List().Append("foo").Append("bar")));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "list|0|1"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetDeeplyNestedListLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append(base::Value::Dict().Set(
                  "foo", base::Value::List().Append("bar").Append("baz"))));

  // Act & Assert
  EXPECT_EQ(base::Value("baz"),
            MaybeGetPrefValue(&pref_provider_, "list|0|foo|1"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       GetNestedListLocalStatePrefValueWithDotSeparatedPathComponents) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append(base::Value::Dict().Set(
                  "foo.bar",
                  base::Value::List().Append("baz.qux").Append("quux.corge"))));

  // Act & Assert
  EXPECT_EQ(base::Value("quux.corge"),
            MaybeGetPrefValue(&pref_provider_, "list|0|foo.bar|1"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetMalformedListLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateListPref("list", base::Value::List().Append("foo"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|0|foo"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|bar"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, ""));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest,
       DoNotGetListLocalStatePrefValueWithOutOfBoundsListIndicies) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append("foo").Append("bar"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|-1"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|2"));
}

TEST_F(BraveAdsConditionMatcherPrefUtilTest, DoNotGetUnknownPrefValue) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "foo.bar"));
}

}  // namespace brave_ads
