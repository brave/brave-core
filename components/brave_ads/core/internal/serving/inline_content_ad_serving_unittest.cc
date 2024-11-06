/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving.h"

#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsInlineContentAdServingTest : public test::TestBase {
 protected:
  void MaybeServeAd(const std::string& dimensions,
                    MaybeServeInlineContentAdCallback callback) {
    SimulateOpeningNewTab(/*tab_id=*/1,
                          /*redirect_chain=*/{GURL("brave://newtab")},
                          net::HTTP_OK);

    SubdivisionTargeting subdivision_targeting;
    AntiTargetingResource anti_targeting_resource;
    InlineContentAdServing ad_serving(subdivision_targeting,
                                      anti_targeting_resource);
    ad_serving.SetDelegate(&delegate_mock_);

    ad_serving.MaybeServeAd(dimensions, std::move(callback));
  }

  ::testing::StrictMock<InlineContentAdServingDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsInlineContentAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kInlineContentAdServingFeature, {{"version", "0"}});

  test::ForcePermissionRules();

  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToServeInlineContentAd);

  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run(/*dimensions=*/"200x100",
                            /*ad=*/::testing::Eq(std::nullopt)));
  MaybeServeAd("200x100", callback.Get());
}

TEST_F(BraveAdsInlineContentAdServingTest, ServeAd) {
  // Arrange
  test::ForcePermissionRules();

  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  database::SaveCreativeInlineContentAds({creative_ad});
  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnOpportunityAroseToServeInlineContentAd);

  EXPECT_CALL(delegate_mock_, OnDidServeInlineContentAd);

  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run(/*dimensions=*/"200x100",
                            /*ad=*/::testing::Ne(std::nullopt)));
  MaybeServeAd("200x100", callback.Get());
}

TEST_F(BraveAdsInlineContentAdServingTest,
       DoNotServeAdForNonExistentDimensions) {
  // Arrange
  test::ForcePermissionRules();

  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnOpportunityAroseToServeInlineContentAd);

  EXPECT_CALL(delegate_mock_, OnFailedToServeInlineContentAd);

  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback,
              Run(/*dimensions=*/"?x?", /*ad=*/::testing::Eq(std::nullopt)));
  MaybeServeAd("?x?", callback.Get());
}

TEST_F(BraveAdsInlineContentAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(/*should_generate_random_uuids=*/true);
  database::SaveCreativeInlineContentAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToServeInlineContentAd);

  base::MockCallback<MaybeServeInlineContentAdCallback> callback;
  EXPECT_CALL(callback, Run(/*dimensions=*/"200x100",
                            /*ad=*/::testing::Eq(std::nullopt)));
  MaybeServeAd("200x100", callback.Get());
}

}  // namespace brave_ads
