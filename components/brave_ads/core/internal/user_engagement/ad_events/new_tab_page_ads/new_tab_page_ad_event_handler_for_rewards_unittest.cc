/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/test/new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/test/new_tab_page_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/test/ad_event_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdEventHandlerForRewardsTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    event_handler_.SetDelegate(&delegate_mock_);
  }

  void FireEventAndVerifyExpectations(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType mojom_ad_event_type,
      bool should_fire_event) {
    base::test::TestFuture<bool, std::string, mojom::NewTabPageAdEventType>
        test_future;
    event_handler_.FireEvent(
        placement_id, creative_instance_id, mojom_ad_event_type,
        test_future.GetCallback<bool, const std::string&,
                                mojom::NewTabPageAdEventType>());
    const auto [success, fired_placement_id, ad_event_type] =
        test_future.Take();
    EXPECT_EQ(should_fire_event, success);
    EXPECT_EQ(placement_id, fired_placement_id);
    EXPECT_EQ(mojom_ad_event_type, ad_event_type);
  }

  NewTabPageAdEventHandler event_handler_;
  ::testing::StrictMock<NewTabPageAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest, FireServedEvent) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdServedEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest, FireViewedEvent) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);
  test::RecordAdEvent(ad, mojom::ConfirmationType::kServedImpression);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdViewedEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::NewTabPageAdEventType::kViewedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  ad.placement_id, ad.creative_instance_id,
                  mojom::NewTabPageAdEventType::kViewedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest, FireClickedEvent) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  // Act & Assert
  base::RunLoop run_loop;
  ::testing::InSequence seq;
  EXPECT_CALL(delegate_mock_, OnWillFireNewTabPageAdClickedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdClickedEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::NewTabPageAdEventType::kClicked,
                                 /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(
    BraveAdsNewTabPageAdEventHandlerForRewardsTest,
    ClickedEventNotifiesDelegateBeforeRecordingCompletesToEnsurePageLandIsRecorded) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  bool delegate_was_notified = false;
  ::testing::InSequence seq;
  EXPECT_CALL(delegate_mock_, OnWillFireNewTabPageAdClickedEvent(ad))
      .WillOnce([&] { delegate_was_notified = true; });
  EXPECT_CALL(delegate_mock_, OnDidFireNewTabPageAdClickedEvent(ad));

  base::test::TestFuture<bool, std::string, mojom::NewTabPageAdEventType>
      test_future;

  // Act & Assert
  event_handler_.FireEvent(
      ad.placement_id, ad.creative_instance_id,
      mojom::NewTabPageAdEventType::kClicked,
      test_future.GetCallback<bool, const std::string&,
                              mojom::NewTabPageAdEventType>());
  const auto [success, placement_id, mojom_ad_event_type] = test_future.Take();
  EXPECT_TRUE(delegate_was_notified);
  EXPECT_TRUE(success);
  EXPECT_EQ(ad.placement_id, placement_id);
  EXPECT_EQ(mojom::NewTabPageAdEventType::kClicked, mojom_ad_event_type);
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kClicked});

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::NewTabPageAdEventType::kClicked))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::NewTabPageAdEventType::kClicked,
                                 /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireClickedEventIfAdPlacementWasNotServed) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnFailedToFireNewTabPageAdEvent(
                                  ad.placement_id, ad.creative_instance_id,
                                  mojom::NewTabPageAdEventType::kClicked))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(ad.placement_id, ad.creative_instance_id,
                                 mojom::NewTabPageAdEventType::kClicked,
                                 /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  test::kInvalidPlacementId, test::kCreativeInstanceId,
                  mojom::NewTabPageAdEventType::kServedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      test::kInvalidPlacementId, test::kCreativeInstanceId,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  test::kPlacementId, test::kInvalidCreativeInstanceId,
                  mojom::NewTabPageAdEventType::kServedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      test::kPlacementId, test::kInvalidCreativeInstanceId,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdEventHandlerForRewardsTest,
       DoNotFireEventForMissingCreativeInstanceId) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildAndSaveNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                     /*use_random_uuids=*/false);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireNewTabPageAdEvent(
                  ad.placement_id, test::kMissingCreativeInstanceId,
                  mojom::NewTabPageAdEventType::kServedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      ad.placement_id, test::kMissingCreativeInstanceId,
      mojom::NewTabPageAdEventType::kServedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

}  // namespace brave_ads
