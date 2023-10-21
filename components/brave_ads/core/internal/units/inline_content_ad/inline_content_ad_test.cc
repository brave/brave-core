/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr char kDimensions[] = "200x100";
}  // namespace

class BraveAdsInlineContentAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp(/*is_integration_test=*/true);

    NotifyTabDidChange(
        /*tab_id=*/1, /*redirect_chain=*/{GURL("brave://newtab")},
        /*is_visible=*/true);
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body=*/"/catalog_with_inline_content_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::InlineContentAdEventType& event_type,
      const bool should_fire_event) {
    base::MockCallback<TriggerAdEventCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event));
    GetAds().TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                         event_type, callback.Get());
  }

  void TriggerInlineContentAdEvents(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const std::vector<mojom::InlineContentAdEventType>& event_types,
      const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                  event_type, should_fire_event);
    }
  }
};

TEST_F(BraveAdsInlineContentAdIntegrationTest, ServeAd) {
  // Arrange
  test::ForcePermissionRules();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_,
              RecordP2AEvents(BuildP2AAdOpportunityEvents(
                  AdType::kInlineContentAd, /*segments=*/{})));

  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run(kDimensions, ::testing::Ne(absl::nullopt)));
  GetAds().MaybeServeInlineContentAd(kDimensions, callback.Get());
}

TEST_F(BraveAdsInlineContentAdIntegrationTest, DoNotServe) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);

  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run(kDimensions, /*ad=*/::testing::Eq(absl::nullopt)));
  GetAds().MaybeServeInlineContentAd(kDimensions, callback.Get());
}

TEST_F(BraveAdsInlineContentAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  test::ForcePermissionRules();

  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const std::string& dimensions,
                    const absl::optional<InlineContentAdInfo>& ad) {
        ASSERT_EQ(kDimensions, dimensions);
        ASSERT_TRUE(ad);
        ASSERT_TRUE(ad->IsValid());

        // Act & Assert
        TriggerInlineContentAdEvent(ad->placement_id, ad->creative_instance_id,
                                    mojom::InlineContentAdEventType::kViewed,
                                    /*should_fire_event=*/true);
      });

  GetAds().MaybeServeInlineContentAd(kDimensions, callback.Get());
}

TEST_F(BraveAdsInlineContentAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  test::ForcePermissionRules();

  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([=](const std::string& dimensions,
                    const absl::optional<InlineContentAdInfo>& ad) {
        ASSERT_EQ(kDimensions, dimensions);
        ASSERT_TRUE(ad);
        ASSERT_TRUE(ad->IsValid());

        TriggerInlineContentAdEvent(ad->placement_id, ad->creative_instance_id,
                                    mojom::InlineContentAdEventType::kViewed,
                                    /*should_fire_event=*/true);

        // Act & Assert
        TriggerInlineContentAdEvent(ad->placement_id, ad->creative_instance_id,
                                    mojom::InlineContentAdEventType::kClicked,
                                    /*should_fire_event=*/true);
      });

  GetAds().MaybeServeInlineContentAd(kDimensions, callback.Get());
}

}  // namespace brave_ads
