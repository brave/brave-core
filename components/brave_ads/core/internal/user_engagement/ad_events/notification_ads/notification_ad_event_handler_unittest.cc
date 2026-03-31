/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_handler.h"

#include <string>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/notification_ad/notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdEventHandlerTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    event_handler_.SetDelegate(&delegate_mock_);
  }

  void FireEventAndVerifyExpectations(
      const std::string& expected_placement_id,
      mojom::NotificationAdEventType expected_mojom_ad_event_type,
      bool expected_success) {
    base::test::TestFuture<bool, std::string, mojom::NotificationAdEventType>
        test_future;
    event_handler_.FireEvent(
        expected_placement_id, expected_mojom_ad_event_type,
        test_future.GetCallback<bool, const std::string&,
                                mojom::NotificationAdEventType>());
    const auto [success, placement_id, mojom_ad_event_type] =
        test_future.Take();
    EXPECT_EQ(expected_success, success);
    EXPECT_EQ(expected_placement_id, placement_id);
    EXPECT_EQ(expected_mojom_ad_event_type, mojom_ad_event_type);
  }

  NotificationAdEventHandler event_handler_;
  ::testing::StrictMock<NotificationAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildAndSaveNotificationAd(/*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdServedEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      ad.placement_id, mojom::NotificationAdEventType::kServedImpression,
      /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildAndSaveNotificationAd(/*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdViewedEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      ad.placement_id, mojom::NotificationAdEventType::kViewedImpression,
      /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildAndSaveNotificationAd(/*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  ::testing::InSequence seq;
  EXPECT_CALL(delegate_mock_, OnWillFireNotificationAdClickedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdClickedEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(ad.placement_id,
                                 mojom::NotificationAdEventType::kClicked,
                                 /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(
    BraveAdsNotificationAdEventHandlerTest,
    ClickedEventNotifiesDelegateBeforeRecordingCompletesToEnsurePageLandIsRecorded) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildAndSaveNotificationAd(/*use_random_uuids=*/false);

  bool delegate_was_notified = false;
  ::testing::InSequence seq;
  EXPECT_CALL(delegate_mock_, OnWillFireNotificationAdClickedEvent(ad))
      .WillOnce([&] { delegate_was_notified = true; });
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdClickedEvent(ad));

  base::test::TestFuture<bool, std::string, mojom::NotificationAdEventType>
      test_future;

  // Act & Assert
  event_handler_.FireEvent(
      ad.placement_id, mojom::NotificationAdEventType::kClicked,
      test_future.GetCallback<bool, const std::string&,
                              mojom::NotificationAdEventType>());
  EXPECT_TRUE(delegate_was_notified);
  const auto [success, placement_id, mojom_ad_event_type] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(ad.placement_id, placement_id);
  EXPECT_EQ(mojom::NotificationAdEventType::kClicked, mojom_ad_event_type);
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireDismissedEvent) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildAndSaveNotificationAd(/*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdDismissedEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(ad.placement_id,
                                 mojom::NotificationAdEventType::kDismissed,
                                 /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireTimedOutEvent) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildAndSaveNotificationAd(/*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdTimedOutEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(ad.placement_id,
                                 mojom::NotificationAdEventType::kTimedOut,
                                 /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(BraveAdsNotificationAdEventHandlerTest,
       DoNotFireEventIfMissingPlacementId) {
  // Arrange
  const NotificationAdInfo ad =
      test::BuildAndSaveNotificationAd(/*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNotificationAdEvent(
                  test::kMissingPlacementId,
                  mojom::NotificationAdEventType::kViewedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      test::kMissingPlacementId,
      mojom::NotificationAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

}  // namespace brave_ads
