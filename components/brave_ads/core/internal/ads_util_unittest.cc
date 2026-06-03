/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads_util.h"

#include "base/test/scoped_command_line.h"
#include "brave/components/brave_ads/core/internal/common/locale/test/fake_locale.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/rewards_flags.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdsUtilTest : public ::testing::Test {
  void SetUp() override {
    prefs_.registry()->RegisterBooleanPref(
        brave_rewards::prefs::kUseRewardsStagingServer, false);

    brave_rewards::RewardsFlags::SetForceParsingForTesting(true);
  }

 protected:
  test::FakeLocale fake_locale_;

  TestingPrefServiceSimple prefs_;

  base::test::ScopedCommandLine scoped_command_line_;
};

TEST_F(BraveAdsAdsUtilTest, UsesDefaultEnvironment) {
#if defined(OFFICIAL_BUILD)
  // In official builds, the default is production.
  EXPECT_FALSE(IsStagingEnvironment(prefs_));
#else
  // In non-official builds, the default is staging.
  EXPECT_TRUE(IsStagingEnvironment(prefs_));
#endif  // OFFICIAL_BUILD
}

TEST_F(BraveAdsAdsUtilTest, UsesStagingEnvironmentWhenEnabledFromCommandLine) {
  scoped_command_line_.GetProcessCommandLine()->AppendSwitchASCII(
      "rewards", "staging=true");
  EXPECT_TRUE(IsStagingEnvironment(prefs_));
}

TEST_F(BraveAdsAdsUtilTest, UsesProductionWhenDisabledFromCommandLine) {
  scoped_command_line_.GetProcessCommandLine()->AppendSwitchASCII(
      "rewards", "staging=false");
  EXPECT_FALSE(IsStagingEnvironment(prefs_));
}

#if BUILDFLAG(IS_ANDROID)
TEST_F(BraveAdsAdsUtilTest, UsesStagingWhenEnabledOnAndroid) {
  scoped_command_line_.GetProcessCommandLine()->AppendSwitchASCII(
      "rewards", "staging=false");
  prefs_.SetBoolean(brave_rewards::prefs::kUseRewardsStagingServer, true);
  EXPECT_TRUE(IsStagingEnvironment(prefs_));
}
#endif

TEST_F(BraveAdsAdsUtilTest, IsSupportedRegion) {
  // Arrange
  fake_locale_.SetCountryCode("US");

  // Act & Assert
  EXPECT_TRUE(IsSupportedRegion());
}

TEST_F(BraveAdsAdsUtilTest, IsUnsupportedRegion) {
  // Arrange
  fake_locale_.SetCountryCode(/*cuba*/ "CU");

  // Act & Assert
  EXPECT_FALSE(IsSupportedRegion());
}

}  // namespace brave_ads
