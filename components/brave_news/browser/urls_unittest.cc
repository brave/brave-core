// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/urls.h"
#include "base/feature_list.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

class BraveNewsUrlsTest : public testing::Test {};

TEST_F(BraveNewsUrlsTest, BraveNewsV2FeatureFlag) {
  EXPECT_TRUE(
      base::FeatureList::IsEnabled(brave_news::features::kBraveNewsV2Feature));
}

TEST_F(BraveNewsUrlsTest, BraveNewsV1UsesLocalUrl) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(brave_news::features::kBraveNewsV2Feature);

  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_US"};
    std::string region = brave_news::GetRegionUrlPart();
    EXPECT_EQ(brave_news::GetV1RegionUrlPart(), region);
    EXPECT_EQ("", region);
  }

  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"ja_JP"};
    std::string region = brave_news::GetRegionUrlPart();
    EXPECT_EQ(brave_news::GetV1RegionUrlPart(), region);
    EXPECT_EQ("ja", region);
  }

  // Unknown/unsupported locale.
  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"na_NA"};
    std::string region = brave_news::GetRegionUrlPart();
    EXPECT_EQ(brave_news::GetV1RegionUrlPart(), region);
    EXPECT_EQ("", region);
  }
}

TEST_F(BraveNewsUrlsTest, BraveNewsUsesGlobalFeedsWithV2) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(brave_news::features::kBraveNewsV2Feature);

  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_US"};
    EXPECT_EQ("global.", brave_news::GetRegionUrlPart());
  }

  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"ja_JP"};
    EXPECT_EQ("global.", brave_news::GetRegionUrlPart());
  }

  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"na_NA"};
    EXPECT_EQ("global.", brave_news::GetRegionUrlPart());
  }
}

TEST_F(BraveNewsUrlsTest, BraveNewsV2FlagDoesNotAffectV1Region) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(brave_news::features::kBraveNewsV2Feature);

  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_US"};
    EXPECT_EQ("", brave_news::GetV1RegionUrlPart());
  }

  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"ja-JP"};
    EXPECT_EQ("ja", brave_news::GetV1RegionUrlPart());
  }

  {
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"na-NA"};
    EXPECT_EQ("", brave_news::GetV1RegionUrlPart());
  }
}
