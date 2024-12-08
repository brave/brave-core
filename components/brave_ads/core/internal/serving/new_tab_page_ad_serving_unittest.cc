/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving.h"

#include <utility>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdServingTest : public test::TestBase {
 protected:
  void MaybeServeAd(MaybeServeNewTabPageAdCallback callback) {
    SubdivisionTargeting subdivision_targeting;
    AntiTargetingResource anti_targeting_resource;
    ad_serving_ = std::make_unique<NewTabPageAdServing>(
        subdivision_targeting, anti_targeting_resource);
    ad_serving_->SetDelegate(&delegate_mock_);

    ad_serving_->MaybeServeAd(std::move(callback));
  }

  ::testing::StrictMock<NewTabPageAdServingDelegateMock> delegate_mock_;
  std::unique_ptr<NewTabPageAdServing> ad_serving_;
};

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNewTabPageAdServingFeature, {{"version", "0"}});

  test::ForcePermissionRules();

  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  database::SaveCreativeNewTabPageAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToServeNewTabPageAd);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*ad=*/::testing::Eq(std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  MaybeServeAd(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdServingTest, ServeAd) {
  // Arrange
  test::ForcePermissionRules();

  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  database::SaveCreativeNewTabPageAds({creative_ad});
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnOpportunityAroseToServeNewTabPageAd);

  EXPECT_CALL(delegate_mock_, OnDidServeNewTabPageAd);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*ad=*/::testing::Ne(std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  MaybeServeAd(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdIfMissingWallpapers) {
  // Arrange
  test::ForcePermissionRules();

  CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  creative_ad.wallpapers.clear();
  database::SaveCreativeNewTabPageAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnOpportunityAroseToServeNewTabPageAd);

  EXPECT_CALL(delegate_mock_, OnFailedToServeNewTabPageAd);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*ad=*/::testing::Eq(std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  MaybeServeAd(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdServingTest, DoNotServeAdIfNoEligibleAdsFound) {
  // Arrange
  test::ForcePermissionRules();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnOpportunityAroseToServeNewTabPageAd);

  EXPECT_CALL(delegate_mock_, OnFailedToServeNewTabPageAd);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*ad=*/::testing::Eq(std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  MaybeServeAd(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsNewTabPageAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  database::SaveCreativeNewTabPageAds({creative_ad});

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToServeNewTabPageAd);

  base::MockCallback<MaybeServeNewTabPageAdCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*ad=*/::testing::Eq(std::nullopt)))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  MaybeServeAd(callback.Get());
  run_loop.Run();
}

}  // namespace brave_ads
