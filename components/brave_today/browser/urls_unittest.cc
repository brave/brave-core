// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/urls.h"
#include "base/feature_list.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

class BraveNewsUrlsTest : public testing::Test {
 public:
  BraveNewsUrlsTest() = default;
  ~BraveNewsUrlsTest() override = default;

  void SetLocale(const std::string& locale) {
    brave_l10n::LocaleHelper::SetForTesting(&locale_helper_mock);
    ON_CALL(locale_helper_mock, GetLocale())
        .WillByDefault(testing::Return(locale));
  }

  testing::NiceMock<brave_l10n::LocaleHelperMock> locale_helper_mock;
};

TEST_F(BraveNewsUrlsTest, BraveNewsV2IsDisabled) {
  EXPECT_FALSE(
      base::FeatureList::IsEnabled(brave_today::features::kBraveNewsV2Feature));
}

TEST_F(BraveNewsUrlsTest, BraveNewsUsesV1ByDefault) {
  SetLocale("en-US");
  std::string region = brave_today::GetRegionUrlPart();
  EXPECT_EQ(brave_today::GetV1RegionUrlPart(), region);
  EXPECT_EQ("", region);

  SetLocale("ja-JP");
  region = brave_today::GetRegionUrlPart();
  EXPECT_EQ(brave_today::GetV1RegionUrlPart(), region);
  EXPECT_EQ("ja", region);

  // Unknown/unsupported locale.
  SetLocale("na-NA");
  region = brave_today::GetRegionUrlPart();
  EXPECT_EQ(brave_today::GetV1RegionUrlPart(), region);
  EXPECT_EQ("", region);
}

TEST_F(BraveNewsUrlsTest, BraveNewsV2FlagUsesGlobalFeeds) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(brave_today::features::kBraveNewsV2Feature);

  SetLocale("en-US");
  EXPECT_EQ("global.", brave_today::GetRegionUrlPart());

  SetLocale("ja-JP");
  EXPECT_EQ("global.", brave_today::GetRegionUrlPart());

  SetLocale("na-NA");
  EXPECT_EQ("global.", brave_today::GetRegionUrlPart());
}

TEST_F(BraveNewsUrlsTest, BraveNewsV2FlagDoesNotAffectV1Region) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(brave_today::features::kBraveNewsV2Feature);

  SetLocale("en-US");
  EXPECT_EQ("", brave_today::GetV1RegionUrlPart());

  SetLocale("ja-JP");
  EXPECT_EQ("ja", brave_today::GetV1RegionUrlPart());

  SetLocale("na-NA");
  EXPECT_EQ("", brave_today::GetV1RegionUrlPart());
}
