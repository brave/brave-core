/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_observer_mock.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds

namespace brave_ads {

class BraveAdsUserRewardsUtilTest : public AdsClientNotifierObserverMock,
                                    public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    GetAdsClient().AddObserver(&ads_client_notifier_observer_mock_);
  }

  void TearDown() override {
    GetAdsClient().RemoveObserver(&ads_client_notifier_observer_mock_);

    test::TestBase::TearDown();
  }

  AdsClientNotifierObserverMock ads_client_notifier_observer_mock_;
};

TEST_F(BraveAdsUserRewardsUtilTest, UpdateIssuers) {
  // Arrange
  EXPECT_CALL(ads_client_notifier_observer_mock_,
              OnNotifyPrefDidChange(prefs::kIssuerPing));
  EXPECT_CALL(ads_client_notifier_observer_mock_,
              OnNotifyPrefDidChange(prefs::kIssuers));

  const IssuersInfo issuers = test::BuildIssuers();

  // Act
  UpdateIssuers(issuers);

  // Assert
  EXPECT_TRUE(HasIssuers());
}

TEST_F(BraveAdsUserRewardsUtilTest, DoNotUpdateIfIssuersHasNotChanged) {
  // Arrange
  test::BuildAndSetIssuers();

  EXPECT_CALL(ads_client_notifier_observer_mock_,
              OnNotifyPrefDidChange(prefs::kIssuerPing))
      .Times(0);
  EXPECT_CALL(ads_client_notifier_observer_mock_,
              OnNotifyPrefDidChange(prefs::kIssuers))
      .Times(0);

  const IssuersInfo issuers = test::BuildIssuers();

  // Act
  UpdateIssuers(issuers);

  // Assert
  EXPECT_TRUE(HasIssuers());
}

}  // namespace brave_ads
