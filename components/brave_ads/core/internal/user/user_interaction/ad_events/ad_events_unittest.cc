/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventsTest : public UnitTestBase {};

TEST_F(BraveAdsAdEventsTest, RecordAdEvent) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
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

  const base::TimeDelta three_months =
      base::Days(/*march*/ 31 + /*april*/ 30 + /*may*/ 31);
  AdvanceClockBy(three_months);

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(3);

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids=*/true);
  AdEventInfo ad_event_for_ad_1 =
      BuildAdEvent(ad_1, ConfirmationType::kServed, /*created_at=*/Now());
  ad_event_for_ad_1.placement_id = "WALDO.1";
  RecordAdEvent(ad_event_for_ad_1, record_ad_event_callback.Get());

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids=*/true);
  AdEventInfo ad_event_for_ad_2 =
      BuildAdEvent(ad_2, ConfirmationType::kServed, /*created_at=*/Now());
  ad_event_for_ad_2.placement_id = "WALDO.2";
  RecordAdEvent(ad_event_for_ad_2, record_ad_event_callback.Get());

  AdvanceClockBy(three_months);

  const AdInfo ad_3 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids=*/true);
  AdEventInfo ad_event_for_ad_3 =
      BuildAdEvent(ad_3, ConfirmationType::kServed, /*created_at=*/Now());
  ad_event_for_ad_3.placement_id = "WALDO.3";
  RecordAdEvent(ad_event_for_ad_3, record_ad_event_callback.Get());

  base::MockCallback<AdEventCallback> purge_expired_ad_events_callback;
  EXPECT_CALL(purge_expired_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeExpiredAdEvents(purge_expired_ad_events_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, AdEventList{{ad_event_for_ad_3}}));
  const database::table::AdEvents database_table;
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsAdEventsTest, PurgeOrphanedAdEvents) {
  // Arrange
  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(4);

  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids=*/true);
  const AdEventInfo orphaned_ad_event_for_ad_1 =
      BuildAdEvent(ad_1, ConfirmationType::kServed,
                   /*created_at=*/Now());
  RecordAdEvent(orphaned_ad_event_for_ad_1, record_ad_event_callback.Get());

  const AdInfo ad_2 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event_for_ad_2a =
      BuildAdEvent(ad_2, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_for_ad_2a, record_ad_event_callback.Get());
  const AdEventInfo ad_event_for_ad_2b =
      BuildAdEvent(ad_2, ConfirmationType::kViewed, /*created_at=*/Now());
  RecordAdEvent(ad_event_for_ad_2b, record_ad_event_callback.Get());

  const AdInfo ad_3 = BuildAdForTesting(AdType::kSearchResultAd,
                                        /*should_use_random_uuids=*/true);
  const AdEventInfo ad_event_for_ad_3 =
      BuildAdEvent(ad_3, ConfirmationType::kServed, /*created_at=*/Now());
  RecordAdEvent(ad_event_for_ad_3, record_ad_event_callback.Get());

  base::MockCallback<AdEventCallback> purge_orphaned_ad_events_callback;
  EXPECT_CALL(purge_orphaned_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeOrphanedAdEvents(mojom::AdType::kNotificationAd,
                        purge_orphaned_ad_events_callback.Get());

  // Assert
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            AdEventList{{ad_event_for_ad_2a, ad_event_for_ad_2b,
                                         ad_event_for_ad_3}}));
  const database::table::AdEvents database_table;
  database_table.GetAll(callback.Get());
}

}  // namespace brave_ads
