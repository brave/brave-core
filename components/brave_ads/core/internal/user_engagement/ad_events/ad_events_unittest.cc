/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"

#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventsTest : public UnitTestBase {};

TEST_F(BraveAdsAdEventsTest, RecordAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event = BuildAdEvent(ad, ConfirmationType::kServed,
                                            /*created_at=*/Now());

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));

  // Act
  RecordAdEvent(ad, ConfirmationType::kServed, record_ad_event_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  const database::table::AdEvents database_table;
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsTest, PurgeExpiredAdEvents) {
  // Arrange
  AdvanceClockTo(
      TimeFromString("Tue, 19 Mar 2024 05:35:00",
                     /*is_local=*/false));  // Happy 1st Birthday Rory!

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(5);

  // Ad 1: Served on 19th March 2024. This ad event should be purged because the
  // creative set is inactive.
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad_1, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_1, record_ad_event_callback.Get());

  // Ad 2: Served on 19th March 2024. This ad event should not be purged because
  // and associated creative set is active.
  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/false);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad_2, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_2, record_ad_event_callback.Get());

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/false);
  database::SaveCreativeNotificationAds({creative_ad});

  // Ad 3: Served on 19th March 2024. This ad event should be purged because the
  // creative set is inactive.
  const AdInfo ad_3 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad_3, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_3, record_ad_event_callback.Get());

  // Ad 4: Served on 19th March 2024. This ad event should not be purged because
  // an associated creative set conversion is active.
  const AdInfo ad_4 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event_4 =
      BuildAdEvent(ad_4, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_4, record_ad_event_callback.Get());

  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          ad_4.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  const base::TimeDelta three_months =
      base::Days(/*march*/ 31 + /*april*/ 30 + /*may*/ 31);
  AdvanceClockBy(three_months);

  // Ad 5: Served on 19th June 2024. This ad event should not be purged because
  // the creative set has not expired.
  const AdInfo ad_5 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event_5 =
      BuildAdEvent(ad_5, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_5, record_ad_event_callback.Get());

  base::MockCallback<AdEventCallback> purge_expired_ad_events_callback;
  EXPECT_CALL(purge_expired_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeExpiredAdEvents(purge_expired_ad_events_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            ::testing::UnorderedElementsAreArray(AdEventList{
                                {ad_event_2, ad_event_4, ad_event_5}})));
  const database::table::AdEvents database_table;
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsTest, DoNotPurgeAdEventsOnTheCuspOfExpiry) {
  // Arrange
  AdvanceClockTo(
      TimeFromString("Tue, 19 Mar 2024 05:35:00",
                     /*is_local=*/false));  // Happy 1st Birthday Rory!

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(1);

  // Ad: Served on 19th March 2024. This ad event should not be purged because
  // it is on the cusp of expiry.
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event, record_ad_event_callback.Get());

  // Move the clock forward to just before the ad event expires.
  const base::TimeDelta three_months =
      base::Days(/*march*/ 31 + /*april*/ 30 + /*may*/ 31);
  AdvanceClockBy(three_months - base::Milliseconds(1));

  base::MockCallback<AdEventCallback> purge_expired_ad_events_callback;
  EXPECT_CALL(purge_expired_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeExpiredAdEvents(purge_expired_ad_events_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  const database::table::AdEvents database_table;
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsTest, PurgeOrphanedAdEvents) {
  // Arrange
  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(4);

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const AdEventInfo orphaned_ad_event_1 =
      BuildAdEvent(ad_1, ConfirmationType::kServed,
                   /*created_at=*/Now());
  RecordAdEvent(orphaned_ad_event_1, record_ad_event_callback.Get());

  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event_2a =
      BuildAdEvent(ad_2, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_2a, record_ad_event_callback.Get());
  const AdEventInfo ad_event_2b =
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now());
  RecordAdEvent(ad_event_2b, record_ad_event_callback.Get());

  const AdInfo ad_3 = test::BuildAd(AdType::kSearchResultAd,
                                    /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad_3, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_3, record_ad_event_callback.Get());

  base::MockCallback<AdEventCallback> purge_orphaned_ad_events_callback;
  EXPECT_CALL(purge_orphaned_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeOrphanedAdEvents(mojom::AdType::kNotificationAd,
                        purge_orphaned_ad_events_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            AdEventList{ad_event_2a, ad_event_2b, ad_event_3}));
  const database::table::AdEvents database_table;
  database_table.GetAll(callback.Get());
}

}  // namespace brave_ads
