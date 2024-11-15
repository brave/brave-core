/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/notification_ad/notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventsDatabaseTableTest : public test::TestBase {
 protected:
  database::table::AdEvents database_table_;
};

TEST_F(BraveAdsAdEventsDatabaseTableTest, RecordEvent) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));

  // Act
  database_table_.RecordEvent(ad_event, record_ad_event_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, GetUnexpired) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(2);

  // Ad event 1: Recorded on 19th March 2024. This ad event should not be
  // included because it will occur outside the expiry window.
  const NotificationAdInfo ad_1 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1, record_ad_event_callback.Get());

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(base::Days(90));

  // Ad event 2: Recorded on 17th June 2024. This ad event should be included
  // because it occurred within the expiry window.
  const NotificationAdInfo ad_2 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kClicked,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_2, record_ad_event_callback.Get());

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event_2}));
  database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest,
       GetUnexpiredIfCreativeSetExistsInCreativeSetConversions) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  // Ad event: Recorded on 19th March 2024. This ad event should be included
  // because it has an associated creative set conversion.
  const NotificationAdInfo ad =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));
  database_table_.RecordEvent(ad_event, record_ad_event_callback.Get());

  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          ad.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(base::Days(90));

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, GetUnexpiredOnTheCuspOfExpiry) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  // Ad event: Recorded on 19th March 2024. This ad event should be included
  // because it will occur on the cusp of the expiry window.
  const NotificationAdInfo ad =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));
  database_table_.RecordEvent(ad_event, record_ad_event_callback.Get());

  // Move the clock forward to just before the ad events expire.
  AdvanceClockBy(base::Days(90) - base::Milliseconds(1));

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, GetUnexpiredForAdType) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 16:28"));

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(3);

  // Ad event 1: Recorded on 19th March 2024. This ad event should not be
  // included because it will occur outside the expiry window.
  const NewTabPageAdInfo ad_1 =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1, record_ad_event_callback.Get());

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(base::Days(90));

  // Ad event 2: Recorded on 17th June 2024. This ad event should not be
  // included because it has a mismatching ad type.
  const NotificationAdInfo ad_2 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_2, record_ad_event_callback.Get());

  // Ad event 3: Recorded on 17th June 2024. This ad event should be included
  // because it occurred within the expiry window.
  const NewTabPageAdInfo ad_3 =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad_3, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_3, record_ad_event_callback.Get());

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event_3}));
  database_table_.GetUnexpired(mojom::AdType::kNewTabPageAd, callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest,
       GetUnexpiredForAdTypeIfCreativeSetExistsInCreativeSetConversions) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(2);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be included
  // because it has an associated creative set conversion.
  const NotificationAdInfo ad_1 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1, record_ad_event_callback.Get());

  // Ad event 2: Recorded on 19th March 2024. This ad event should not be
  // included because it has a mismatching ad type.
  const NewTabPageAdInfo ad_2 =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_2, record_ad_event_callback.Get());

  // Associate a creative set conversion to both ad events.
  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          ad_1.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(base::Days(90));

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event_1}));
  database_table_.GetUnexpired(mojom::AdType::kNotificationAd, callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, PurgeExpired) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(3);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be purged
  // because it will occur outside the expiry window.
  const NotificationAdInfo ad_1 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1, result_callback.Get());

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(base::Days(90));

  // Ad event 2: Recorded on 17th June 2024. This ad event should be included
  // because it occurred within the expiry window.
  const NotificationAdInfo ad_2 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_2, result_callback.Get());

  // Act
  database_table_.PurgeExpired(result_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event_2}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, PurgeExpiredForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(3);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be purged
  // because it will occur outside the expiry window.
  const NotificationAdInfo ad_1 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1, result_callback.Get());

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(base::Days(30));

  // Ad event 2: Recorded on 18th April 2024. This ad event should be included
  // because it occurred within the expiry window.
  const NotificationAdInfo ad_2 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kClicked,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_2, result_callback.Get());

  // Act
  database_table_.PurgeExpired(result_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event_2}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest,
       DoNotPurgeExpiredIfCreativeSetExistsInCreativeSetConversions) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(2);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be purged
  // because it will occur outside the expiry window.
  const NotificationAdInfo ad =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  database_table_.RecordEvent(ad_event, result_callback.Get());

  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          ad.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(base::Days(90));

  // Act
  database_table_.PurgeExpired(result_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, PurgeOrphanedForType) {
  // Arrange
  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(5);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);

  // Ad event 1: This served ad impression event should not be purged because it
  // has an associated viewed impression ad event for the matching ad type.
  const NotificationAdInfo ad_1 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_1_served =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1_served, result_callback.Get());

  const AdEventInfo ad_event_1_viewed =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1_viewed, result_callback.Get());

  // Ad event 2: This served impression ad event should not be purged because it
  // has a mismatching ad type.
  const NewTabPageAdInfo ad_2 =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_2_served =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_2_served, result_callback.Get());

  // Ad event 3: This served impression ad should be purged because it has a
  // matching ad type.
  const NotificationAdInfo ad_3 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_3_served =
      BuildAdEvent(ad_3, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_3_served, result_callback.Get());

  // Act
  database_table_.PurgeOrphaned(mojom::AdType::kNotificationAd,
                                result_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          ::testing::UnorderedElementsAreArray(AdEventList{
              ad_event_1_served, ad_event_1_viewed, ad_event_2_served})));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, PurgeOrphaned) {
  // Arrange
  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(5);

  // Ad event 1: This served impression ad event should not be purged because it
  // has a mismatching placement id.
  const NotificationAdInfo ad_1 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_1_served =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1_served, result_callback.Get());

  const AdEventInfo ad_event_1_viewed =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1_viewed, result_callback.Get());

  // Ad event 2: This served impression ad event should be purged because it has
  // a matching placement id.
  const NewTabPageAdInfo ad_2 =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_2_served =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_2_served, result_callback.Get());

  // Ad event 3: This served impression ad event should not be purged because it
  // has a mismatching placement id.
  const NotificationAdInfo ad_3 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_3_served =
      BuildAdEvent(ad_3, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_3_served, result_callback.Get());

  // Act
  database_table_.PurgeOrphaned(
      /*placement_ids=*/{ad_event_2_served.placement_id},
      result_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          ::testing::UnorderedElementsAreArray(AdEventList{
              ad_event_1_served, ad_event_1_viewed, ad_event_3_served})));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, PurgeAllOrphaned) {
  // Arrange
  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(5);

  // Ad event 1: This served impression ad event should not be purged because it
  // has an associated viewed impression ad event.
  const NotificationAdInfo ad_1 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_1_served =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1_served, result_callback.Get());

  const AdEventInfo ad_event_1_viewed =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_1_viewed, result_callback.Get());

  // Ad event 2: This served impression ad event should be purged because it
  // does not have an associated viewed impression ad event.
  const NewTabPageAdInfo ad_2 =
      test::BuildNewTabPageAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_2_served =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_2_served, result_callback.Get());

  // Ad event 3: This served impression ad event should be purged because it
  // does not have an associated viewed impression ad event.
  const NotificationAdInfo ad_3 =
      test::BuildNotificationAd(/*should_generate_random_uuids=*/true);

  const AdEventInfo ad_event_3_served =
      BuildAdEvent(ad_3, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  database_table_.RecordEvent(ad_event_3_served, result_callback.Get());

  // Act
  database_table_.PurgeAllOrphaned(result_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            ::testing::UnorderedElementsAreArray(AdEventList{
                                ad_event_1_served, ad_event_1_viewed})));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("ad_events", database_table_.GetTableName());
}

}  // namespace brave_ads
