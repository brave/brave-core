/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_notifier_observer_mock.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds

namespace brave_ads {

class BraveAdsUserRewardsUtilTest : public AdsClientNotifierObserverMock,
                                    public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    AddAdsClientNotifierObserver(&observer_mock_);
  }

  void TearDown() override {
    RemoveAdsClientNotifierObserver(&observer_mock_);

    UnitTestBase::TearDown();
  }

  AdsClientNotifierObserverMock observer_mock_;
};

TEST_F(BraveAdsUserRewardsUtilTest, ShouldMigrateVerifiedRewardsUser) {
  // Arrange
  SetProfileBooleanPrefValue(prefs::kShouldMigrateVerifiedRewardsUser, true);

  // Act & Assert
  EXPECT_TRUE(ShouldMigrateVerifiedRewardsUser());
}

TEST_F(BraveAdsUserRewardsUtilTest,
       ShouldNotMigrateVerifiedRewardsUserIfBraveRewardsIsDisabled) {
  // Arrange
  test::DisableBraveRewards();

  SetProfileBooleanPrefValue(prefs::kShouldMigrateVerifiedRewardsUser, false);

  // Act & Assert
  EXPECT_FALSE(ShouldMigrateVerifiedRewardsUser());
}

TEST_F(BraveAdsUserRewardsUtilTest, UpdateIssuers) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnNotifyPrefDidChange(prefs::kIssuerPing));
  EXPECT_CALL(observer_mock_, OnNotifyPrefDidChange(prefs::kIssuers));

  const IssuersInfo issuers = test::BuildIssuers();

  // Act
  UpdateIssuers(issuers);

  // Assert
  EXPECT_TRUE(HasIssuers());
}

TEST_F(BraveAdsUserRewardsUtilTest, DoNotUpdateIfIssuersHasNotChanged) {
  // Arrange
  test::BuildAndSetIssuers();

  ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&observer_mock_));

  EXPECT_CALL(observer_mock_, OnNotifyPrefDidChange(prefs::kIssuerPing))
      .Times(0);
  EXPECT_CALL(observer_mock_, OnNotifyPrefDidChange(prefs::kIssuers)).Times(0);

  // Act
  UpdateIssuers(test::BuildIssuers());

  // Assert
  EXPECT_TRUE(HasIssuers());
}

}  // namespace brave_ads
