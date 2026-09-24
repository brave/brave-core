// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"

#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_waiter.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace brave_ads {

class BraveAdsServiceImplIOSTest : public PlatformTest {
 public:
  BraveAdsServiceImplIOSTest() {
    RegisterProfilePrefs(prefs_.registry());
    brave_rewards::RegisterProfilePrefs(prefs_.registry());
    ads_service_ = std::make_unique<AdsServiceImplIOS>(prefs_);
  }

 protected:
  // Blocks until every task already posted to the current sequence has run,
  // including tasks posted by production code from a pref change
  // notification (which cannot run synchronously; see
  // `AdsServiceImplIOS::OnAdsPrefChanged`).
  void FlushPendingTasks() {
    bool did_run = false;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindLambdaForTesting([&] { did_run = true; }));
    ASSERT_TRUE(base::test::RunUntil([&] { return did_run; }));
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<AdsServiceImplIOS> ads_service_;
};

TEST_F(BraveAdsServiceImplIOSTest,
       ClearsAdsDataWhenSponsoredAdsBecomeDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  // `ClearAdsPrefs` clears the whole `brave.brave_ads.*` prefix at once, so
  // checking this one pref is enough to tell whether the whole prefix was
  // cleared.
  prefs_.SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kDiagnosticId));
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotClearAdsDataWhenUnrelatedPrefChanges) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotClearAdsDataWhenSponsoredAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(
    BraveAdsServiceImplIOSTest,
    PreservesAdsDataWhenSponsoredAdsBecomeDisabledForBraveRewardsUser) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  // Ensures `MaybeClearAdsData`'s posted task has had a
  // chance to (not) clear data before asserting.
  FlushPendingTasks();

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(
    BraveAdsServiceImplIOSTest,
    PreservesAdsDataWhenBraveRewardsBecomesDisabledWhileSponsoredAdsRemainEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);
  // Ensures `MaybeClearAdsData`'s posted task has had a
  // chance to (not) clear data before asserting.
  FlushPendingTasks();

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearsAdsDataWhenBraveRewardsBecomesDisabledAndSponsoredAdsAreDisabled) {
  // Arrange
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  // `ClearAdsPrefs` clears the whole `brave.brave_ads.*` prefix at once, so
  // checking this one pref is enough to tell whether the whole prefix was
  // cleared.
  prefs_.SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kDiagnosticId));
}

}  // namespace brave_ads
