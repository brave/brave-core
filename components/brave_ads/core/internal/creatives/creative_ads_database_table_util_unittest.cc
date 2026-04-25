/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

constexpr char kConditionMatchersAsString[] =
    R"(Zm9vLmJhcnxiYXo=|W1Q9XTo3;cXV4LnF1dXg=|W1I8XToz)";

ConditionMatcherMap ConditionMatchers() {
  return {{/*pref_path*/ "foo.bar|baz", /*condition*/ "[T=]:7"},
          {/*pref_path*/ "qux.quux", /*condition*/ "[R<]:3"}};
}

}  // namespace

TEST(BraveAdsCreativeAdsDatabaseTableUtilTest, ConditionMatchersToString) {
  // Arrange
  const ConditionMatcherMap condition_matchers = ConditionMatchers();

  // Act & Assert
  EXPECT_EQ(kConditionMatchersAsString,
            ConditionMatchersToString(condition_matchers));
}

TEST(BraveAdsCreativeAdsDatabaseTableUtilTest, EmptyConditionMatchersToString) {
  // Act & Assert
  EXPECT_THAT(ConditionMatchersToString({}), ::testing::IsEmpty());
}

TEST(BraveAdsCreativeAdsDatabaseTableUtilTest, StringToConditionMatchers) {
  // Act & Assert
  EXPECT_EQ(ConditionMatchers(),
            StringToConditionMatchers(kConditionMatchersAsString));
}

TEST(BraveAdsCreativeAdsDatabaseTableUtilTest,
     IgnoreMalformedStringInConditionMatchers) {  // Act & Assert
  EXPECT_THAT(StringToConditionMatchers(R"(malformed)"), ::testing::IsEmpty());
}

}  // namespace brave_ads::database::table
