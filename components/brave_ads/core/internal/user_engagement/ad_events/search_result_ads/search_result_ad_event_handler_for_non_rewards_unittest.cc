/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <cstddef>
#include <string>

#include "base/check.h"
#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void VerifyDepositForCreativeInstanceIdExpectation(
    const std::string& creative_instance_id) {
  base::MockCallback<database::table::GetDepositsCallback> callback;
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  EXPECT_CALL(callback, Run(/*success=*/::testing::_,
                            /*deposit=*/::testing::Eq(std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  const database::table::Deposits database_table;
  database_table.GetForCreativeInstanceId(creative_instance_id, callback.Get());
  run_loop.Run();
}

void VerifyCreativeSetConversionExpectation(size_t expected_count) {
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  EXPECT_CALL(
      callback,
      Run(/*success=*/::testing::_,
          /*creative_set_conversions=*/::testing::SizeIs(expected_count)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  const database::table::CreativeSetConversions database_table;
  database_table.GetUnexpired(callback.Get());
  run_loop.Run();
}

}  // namespace

class BraveAdsSearchResultAdEventHandlerForNonRewardsTest
    : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    scoped_feature_list_.InitAndEnableFeature(
        kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

    test::DisableBraveRewards();

    event_handler_.SetDelegate(&delegate_mock_);
  }

  void FireEventAndVerifyExpectations(
      const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      bool should_fire_event) {
    CHECK(mojom_creative_ad);

    base::MockCallback<FireSearchResultAdEventHandlerCallback> callback;
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    EXPECT_CALL(callback,
                Run(/*success=*/should_fire_event,
                    mojom_creative_ad->placement_id, mojom_ad_event_type))
        .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
    event_handler_.FireEvent(mojom_creative_ad.Clone(), mojom_ad_event_type,
                             callback.Get());
    run_loop.Run();

    size_t expected_count = 0;

    if (should_fire_event &&
        mojom_ad_event_type == mojom::SearchResultAdEventType::kClicked) {
      const SearchResultAdInfo ad =
          FromMojomBuildSearchResultAd(mojom_creative_ad);
      expected_count =
          mojom_creative_ad->creative_set_conversion && ad.IsValid() ? 1 : 0;
    }

    VerifyDepositForCreativeInstanceIdExpectation(
        mojom_creative_ad->creative_instance_id);

    VerifyCreativeSetConversionExpectation(expected_count);
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  SearchResultAdEventHandler event_handler_;
  ::testing::StrictMock<SearchResultAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireEventIfShouldNotAlwaysTriggerAdEvents) {
  // Arrange
  scoped_feature_list_.Reset();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kClicked))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(mojom_creative_ad,
                                 mojom::SearchResultAdEventType::kClicked,
                                 /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireServedEventWithoutConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireSearchResultAdEvent(
                  ad, mojom::SearchResultAdEventType::kServedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      mojom_creative_ad, mojom::SearchResultAdEventType::kServedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireServedEventWithConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireSearchResultAdEvent(
                  ad, mojom::SearchResultAdEventType::kServedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      mojom_creative_ad, mojom::SearchResultAdEventType::kServedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireViewedEventWithoutConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireSearchResultAdEvent(
                  ad, mojom::SearchResultAdEventType::kViewedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      mojom_creative_ad, mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireViewedEventWithConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireSearchResultAdEvent(
                  ad, mojom::SearchResultAdEventType::kViewedImpression))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(
      mojom_creative_ad, mojom::SearchResultAdEventType::kViewedImpression,
      /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireClickedEventWithoutConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(/*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kClicked))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(mojom_creative_ad,
                                 mojom::SearchResultAdEventType::kClicked,
                                 /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       FireClickedEventWithConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdClickedEvent(ad))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(mojom_creative_ad,
                                 mojom::SearchResultAdEventType::kClicked,
                                 /*should_fire_event=*/true);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);
  test::RecordAdEvent(ad, mojom::ConfirmationType::kClicked);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kClicked))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(mojom_creative_ad,
                                 mojom::SearchResultAdEventType::kClicked,
                                 /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireClickedEventWithInvalidPlacementId) {
  // Arrange
  mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  mojom_creative_ad->placement_id = test::kInvalidPlacementId;
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kClicked))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(mojom_creative_ad,
                                 mojom::SearchResultAdEventType::kClicked,
                                 /*should_fire_event=*/false);
  run_loop.Run();
}

TEST_F(BraveAdsSearchResultAdEventHandlerForNonRewardsTest,
       DoNotFireClickedEventWithInvalidCreativeInstanceId) {
  // Arrange
  mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  mojom_creative_ad->creative_instance_id = test::kInvalidCreativeInstanceId;
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kClicked))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  FireEventAndVerifyExpectations(mojom_creative_ad,
                                 mojom::SearchResultAdEventType::kClicked,
                                 /*should_fire_event=*/false);
  run_loop.Run();
}

}  // namespace brave_ads
