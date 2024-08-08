/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_delta_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventsTest : public test::TestBase {};

TEST_F(BraveAdsAdEventsTest, RecordAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));

  // Act
  RecordAdEvent(ad, ConfirmationType::kServedImpression,
                record_ad_event_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  const database::table::AdEvents database_table;
  database_table.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsTest, PurgeExpiredAdEvents) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString(
      "Tue, 19 Mar 2024 05:35"));  // Happy 1st Birthday Rory!

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(4);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be purged
  // because there are no associated creative set conversions.
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad_1, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_1, record_ad_event_callback.Get());

  // Ad event 2: Recorded on 19th March 2024. This ad event should be purged
  // because there are no associated creative set conversions.
  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad_2, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_2, record_ad_event_callback.Get());

  // Ad event 3: Recorded on 19th March 2024. This ad event should not be purged
  // because it has an associated creative set conversion.
  const AdInfo ad_3 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_3 = BuildAdEvent(
      ad_3, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_3, record_ad_event_callback.Get());

  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          ad_3.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(Months(3));

  // Ad event 4: Recorded on 19th June 2024. This ad event should not be purged
  // because it occurred within the expiry window.
  const AdInfo ad_4 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_4 = BuildAdEvent(
      ad_4, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_4, record_ad_event_callback.Get());

  base::MockCallback<AdEventCallback> purge_expired_ad_events_callback;
  EXPECT_CALL(purge_expired_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeExpiredAdEvents(purge_expired_ad_events_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, AdEventList{ad_event_3, ad_event_4}));
  const database::table::AdEvents database_table;
  database_table.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsTest, PurgeExpiredAdEventsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  AdvanceClockTo(test::TimeFromUTCString(
      "Tue, 19 Mar 2024 05:35"));  // Happy 1st Birthday Rory!

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(4);

  // Ad event 1: Recorded on 19th March 2024. This ad event should be purged
  // because there are no associated creative set conversions.
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad_1, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_1, record_ad_event_callback.Get());

  // Ad event 2: Recorded on 19th March 2024. This ad event should be purged
  // because there are no associated creative set conversions.
  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad_2, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_2, record_ad_event_callback.Get());

  // Ad event 3: Recorded on 19th March 2024. This ad event should not be purged
  // because it has an associated creative set conversion.
  const AdInfo ad_3 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_3 = BuildAdEvent(
      ad_3, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_3, record_ad_event_callback.Get());

  CreativeSetConversionList creative_set_conversions;
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(
          ad_3.creative_set_id,
          /*url_pattern=*/"https://www.brave.com/signup/*",
          /*observation_window=*/base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion);
  database::SaveCreativeSetConversions(creative_set_conversions);

  // Move the clock forward to when the ad events expire.
  AdvanceClockBy(base::Days(30));

  // Ad event 4: Recorded on 18th April 2024. This ad event should not be purged
  // because it occurred within the expiry window.
  const AdInfo ad_4 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_4 = BuildAdEvent(
      ad_4, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_4, record_ad_event_callback.Get());

  base::MockCallback<AdEventCallback> purge_expired_ad_events_callback;
  EXPECT_CALL(purge_expired_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeExpiredAdEvents(purge_expired_ad_events_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, AdEventList{ad_event_3, ad_event_4}));
  const database::table::AdEvents database_table;
  database_table.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsTest, DoNotPurgeExpiredAdEventsOnTheCuspOfExpiry) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  // Ad event: Recorded on 19th March 2024. This ad event should not be purged
  // because it will occur on the cusp of the expiry window.
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));
  RecordAdEvent(ad_event, record_ad_event_callback.Get());

  // Move the clock forward to just before the ad events expire.
  AdvanceClockBy(Months(3) - base::Milliseconds(1));

  base::MockCallback<AdEventCallback> purge_expired_ad_events_callback;
  EXPECT_CALL(purge_expired_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeExpiredAdEvents(purge_expired_ad_events_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}));
  const database::table::AdEvents database_table;
  database_table.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsAdEventsTest, PurgeOrphanedAdEvents) {
  // Arrange
  AdvanceClockTo(
      test::TimeFromUTCString("Wed, 31 Jan 2024 16:28"));  // Hello Florrie!!!

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(4);

  // Ad event 1: This served impression ad event should be purged because it
  // does not have an associated viewed impression ad event or matching ad type.
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad_1, ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  RecordAdEvent(ad_event_1, record_ad_event_callback.Get());

  // Ad event 2: This served impression ad event should not be purged because it
  // has an associated viewed impression ad event for the matching ad type.
  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2a = BuildAdEvent(
      ad_2, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_2a, record_ad_event_callback.Get());
  const AdEventInfo ad_event_2b = BuildAdEvent(
      ad_2, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  RecordAdEvent(ad_event_2b, record_ad_event_callback.Get());

  // Ad event 3: This served impression ad event should not be purged because it
  // has a mismatching ad type.
  const AdInfo ad_3 = test::BuildAd(AdType::kSearchResultAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_3 = BuildAdEvent(
      ad_3, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
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
  database_table.GetUnexpired(callback.Get());
}

}  // namespace brave_ads
