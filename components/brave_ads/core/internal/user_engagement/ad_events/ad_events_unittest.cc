/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventsTest : public test::TestBase {};

TEST_F(BraveAdsAdEventsTest, RecordAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true));

  // Act
  RecordAdEvent(ad, mojom::ConfirmationType::kServedImpression,
                record_ad_event_callback.Get());

  // Assert
  base::RunLoop run_loop;
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, AdEventList{ad_event}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  const database::table::AdEvents database_table;
  database_table.GetUnexpired(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsAdEventsTest, PurgeOrphanedAdEvents) {
  // Arrange
  AdvanceClockTo(
      test::TimeFromUTCString("Wed, 31 Jan 2024 16:28"));  // Hello Florrie!!!

  base::MockCallback<AdEventCallback> record_ad_event_callback;
  EXPECT_CALL(record_ad_event_callback, Run(/*success=*/true)).Times(4);

  // Ad event 1: This served impression ad event should be purged because it
  // does not have an associated viewed impression ad event or matching ad type.
  const AdInfo ad_1 = test::BuildAd(mojom::AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad_1, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  RecordAdEvent(ad_event_1, record_ad_event_callback.Get());

  // Ad event 2: This served impression ad event should not be purged because it
  // has an associated viewed impression ad event for the matching ad type.
  const AdInfo ad_2 = test::BuildAd(mojom::AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2a =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  RecordAdEvent(ad_event_2a, record_ad_event_callback.Get());
  const AdEventInfo ad_event_2b =
      BuildAdEvent(ad_2, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  RecordAdEvent(ad_event_2b, record_ad_event_callback.Get());

  // Ad event 3: This served impression ad event should not be purged because it
  // has a mismatching ad type.
  const AdInfo ad_3 = test::BuildAd(mojom::AdType::kSearchResultAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad_3, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  RecordAdEvent(ad_event_3, record_ad_event_callback.Get());

  base::MockCallback<AdEventCallback> purge_orphaned_ad_events_callback;
  EXPECT_CALL(purge_orphaned_ad_events_callback, Run(/*success=*/true));

  // Act
  PurgeOrphanedAdEvents(mojom::AdType::kNotificationAd,
                        purge_orphaned_ad_events_callback.Get());

  // Assert
  base::RunLoop run_loop;
  base::MockCallback<database::table::GetAdEventsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            AdEventList{ad_event_2a, ad_event_2b, ad_event_3}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  const database::table::AdEvents database_table;
  database_table.GetUnexpired(callback.Get());
  run_loop.Run();
}

}  // namespace brave_ads
