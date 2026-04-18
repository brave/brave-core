/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/mock_callback.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/database/database_maintenance.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database {

class BraveAdsDatabaseMaintenanceIssue54710Test : public test::TestBase {
 protected:
  void SetUp() override {
    TestBase::SetUp();
    maintenance_ = std::make_unique<Maintenance>();
    ads_client_notifier_.NotifyDidInitializeAds();
  }

  AdEventList GetAllAdEvents() {
    base::test::TestFuture<bool, AdEventList> test_future;
    database_table_.GetAll(test_future.GetCallback<bool, const AdEventList&>());
    const auto [success, ad_events] = test_future.Take();
    EXPECT_TRUE(success);
    return ad_events;
  }

  std::unique_ptr<Maintenance> maintenance_;

  database::table::AdEvents database_table_;
};

TEST_F(BraveAdsDatabaseMaintenanceIssue54710Test,
       PurgesNewTabPageAdEventsOnPrefChangeWhenNotOptedInToNewTabPageAds) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad =
      test::BuildAd(mojom::AdType::kNewTabPageAd, /*use_random_uuids=*/true);
  base::MockCallback<AdEventCallback> record_callback;
  EXPECT_CALL(record_callback, Run(/*success=*/true));
  RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                record_callback.Get());

  // Act
  GetAdsClient().SetProfilePref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage,
      base::Value(false));

  // Assert
  EXPECT_THAT(GetAllAdEvents(), ::testing::IsEmpty());
}

TEST_F(BraveAdsDatabaseMaintenanceIssue54710Test,
       DoesNotPurgeNewTabPageAdEventsOnPrefChangeWhenJoinedBraveRewards) {
  // Arrange
  const AdInfo ad =
      test::BuildAd(mojom::AdType::kNewTabPageAd, /*use_random_uuids=*/true);
  base::MockCallback<AdEventCallback> record_callback;
  EXPECT_CALL(record_callback, Run(/*success=*/true));
  RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                record_callback.Get());

  // Act
  GetAdsClient().SetProfilePref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage,
      base::Value(false));

  // Assert
  EXPECT_THAT(GetAllAdEvents(), ::testing::SizeIs(1U));
}

TEST_F(BraveAdsDatabaseMaintenanceIssue54710Test,
       DoesNotPurgeNewTabPageAdEventsOnPrefChangeWhenOptedInToNewTabPageAds) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad =
      test::BuildAd(mojom::AdType::kNewTabPageAd, /*use_random_uuids=*/true);
  base::MockCallback<AdEventCallback> record_callback;
  EXPECT_CALL(record_callback, Run(/*success=*/true));
  RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                record_callback.Get());

  // Act
  GetAdsClient().SetProfilePref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage,
      base::Value(true));

  // Assert
  EXPECT_THAT(GetAllAdEvents(), ::testing::SizeIs(1U));
}

}  // namespace brave_ads::database
