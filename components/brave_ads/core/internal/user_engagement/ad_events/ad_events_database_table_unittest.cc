/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/time/time_delta_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventsDatabaseTableTest : public UnitTestBase {
 protected:
  database::table::AdEvents database_table_;
};

TEST_F(BraveAdsAdEventsDatabaseTableTest, RecordEvent) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);

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
  AdvanceClockTo(TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(2);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Ad event 1: Recorded on 19th March 2024. This ad event should not be
  // included because it will occur outside the expiry window.
  const AdEventInfo ad_event_1 = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_1, record_ad_event_callback.Get());

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(Months(3));

  // Ad event 2: Recorded on 19th June 2024. This ad event should be included
  // because it occurred within the expiry window.
  const AdEventInfo ad_event_2 = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kClicked,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_2, record_ad_event_callback.Get());

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event_2}));
  database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest,
       GetUnexpiredIfCreativeSetExistsInCreativeSetConversions) {
  // Arrange
  AdvanceClockTo(TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  // Ad event: Recorded on 19th March 2024. This ad event should be included
  // because it has an associated creative set conversion.
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));
  database_table_.RecordEvent(ad_event, record_ad_event_callback.Get());

  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          creative_ad.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(Months(3));

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, GetUnexpiredOnTheCuspOfExpiry) {
  // Arrange
  AdvanceClockTo(TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  // Ad event: Recorded on 19th March 2024. This ad event should be included
  // because it will occur on the cusp of the expiry window.
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));
  database_table_.RecordEvent(ad_event, record_ad_event_callback.Get());

  // Move the clock forward to just before the ad events expire.
  AdvanceClockBy(Months(3) - base::Milliseconds(1));

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, GetUnexpiredForType) {
  // Arrange
  AdvanceClockTo(TimeFromUTCString("Tue, 19 Mar 2024 16:28"));

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(3);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Ad event 1: Recorded on 19th March 2024. This ad event should not be
  // included because it will occur outside the expiry window.
  const AdEventInfo ad_event_1 = test::BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_1, record_ad_event_callback.Get());

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(Months(3));

  // Ad event 2: Recorded on 19th March 2024. This ad event should not be
  // included because it has a mismatching ad type.
  const AdEventInfo ad_event_2 = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_2, record_ad_event_callback.Get());

  // Ad event 3: Recorded on 19th June 2024. This ad event should be included
  // because it occurred within the expiry window.
  const AdEventInfo ad_event_3 = test::BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_3, record_ad_event_callback.Get());

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event_3}));
  database_table_.GetUnexpiredForType(mojom::AdType::kNewTabPageAd,
                                      callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest,
       GetUnexpiredForTypeIfCreativeSetExistsInCreativeSetConversions) {
  // Arrange
  AdvanceClockTo(TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(2);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be included
  // because it has an associated creative set conversion.
  const AdEventInfo ad_event_1 = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_1, record_ad_event_callback.Get());

  // Ad event 2: Recorded on 19th March 2024. This ad event should not be
  // included because it has a mismatching ad type.
  const AdEventInfo ad_event_2 = test::BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_2, record_ad_event_callback.Get());

  // Associate a creative set conversion to both ad events.
  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          creative_ad.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(Months(3));

  // Act & Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event_1}));
  database_table_.GetUnexpiredForType(mojom::AdType::kNotificationAd,
                                      callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, PurgeExpired) {
  // Arrange
  AdvanceClockTo(TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(3);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be purged
  // because it will occur outside the expiry window.
  const AdEventInfo ad_event_1 = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_1, result_callback.Get());

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(Months(3));

  // Ad event 2: Recorded on 19th June 2024. This ad event should be included
  // because it occurred within the expiry window.
  const AdEventInfo ad_event_2 = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kClicked,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
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
  AdvanceClockTo(TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(2);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be purged
  // because it will occur outside the expiry window.
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);

  database_table_.RecordEvent(ad_event, result_callback.Get());

  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          creative_ad.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(Months(3));

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
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Ad event 1: This served ad impression event should not be purged because it
  // has an associated viewed impression ad event for the matching ad type.
  const AdEventInfo ad_event_1_served = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/false);
  database_table_.RecordEvent(ad_event_1_served, result_callback.Get());

  const AdEventInfo ad_event_1_viewed = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/false);
  database_table_.RecordEvent(ad_event_1_viewed, result_callback.Get());

  // Ad event 2: This served impression ad event should not be purged because it
  // has a mismatching ad type.
  const AdEventInfo ad_event_2_served = test::BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_2_served, result_callback.Get());

  // Ad event 3: This served impression ad should be purged because it has a
  // matching ad type.
  const AdEventInfo ad_event_3_served = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_3_served, result_callback.Get());

  // Act
  database_table_.PurgeOrphaned(mojom::AdType::kNotificationAd,
                                result_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          testing::UnorderedElementsAreArray(AdEventList{
              ad_event_1_served, ad_event_1_viewed, ad_event_2_served})));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, PurgeOrphaned) {
  // Arrange
  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(5);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Ad event 1: This served impression ad event should not be purged because it
  // has a mismatching placement id.
  const AdEventInfo ad_event_1_served = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/false);
  database_table_.RecordEvent(ad_event_1_served, result_callback.Get());

  const AdEventInfo ad_event_1_viewed = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/false);
  database_table_.RecordEvent(ad_event_1_viewed, result_callback.Get());

  // Ad event 2: This served impression ad event should be purged because it has
  // a matching placement id.
  const AdEventInfo ad_event_2_served = test::BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_2_served, result_callback.Get());

  // Ad event 3: This served impression ad event should not be purged because it
  // has a mismatching placement id.
  const AdEventInfo ad_event_3_served = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
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
          testing::UnorderedElementsAreArray(AdEventList{
              ad_event_1_served, ad_event_1_viewed, ad_event_3_served})));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, PurgeAllOrphaned) {
  // Arrange
  base::MockCallback<ResultCallback> result_callback;
  EXPECT_CALL(result_callback, Run(/*success=*/true)).Times(5);

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Ad event 1: This served impression ad event should not be purged because it
  // has an associated viewed impression ad event.
  const AdEventInfo ad_event_1_served = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/false);
  database_table_.RecordEvent(ad_event_1_served, result_callback.Get());

  const AdEventInfo ad_event_1_viewed = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/false);
  database_table_.RecordEvent(ad_event_1_viewed, result_callback.Get());

  // Ad event 2: This served impression ad event should be purged because it
  // does not have an associated viewed impression ad event.
  const AdEventInfo ad_event_2_served = test::BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_2_served, result_callback.Get());

  // Ad event 3: This served impression ad event should be purged because it
  // does not have an associated viewed impression ad event.
  const AdEventInfo ad_event_3_served = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServedImpression,
      /*created_at=*/Now(), /*should_use_random_uuids=*/true);
  database_table_.RecordEvent(ad_event_3_served, result_callback.Get());

  // Act
  database_table_.PurgeAllOrphaned(result_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            testing::UnorderedElementsAreArray(AdEventList{
                                ad_event_1_served, ad_event_1_viewed})));
  database_table_.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsDatabaseTableTest, GetTableName) {
  // Act & Assert
  EXPECT_EQ("ad_events", database_table_.GetTableName());
}

}  // namespace brave_ads
