/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_v2.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEligibleNewTabPageAdsV2Test : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    eligible_ads_ = std::make_unique<EligibleNewTabPageAdsV2>(
        *subdivision_targeting_, *anti_targeting_resource_);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  std::unique_ptr<EligibleNewTabPageAdsV2> eligible_ads_;
};

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, GetAds) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  const CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.segment = "parent";
  creative_ads.push_back(creative_ad_2);

  CreativeNewTabPageAdInfo creative_ad_3 =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  creative_ad_3.segment = "parent-child";
  creative_ads.push_back(creative_ad_3);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act & Assert
  base::RunLoop run_loop;
  base::MockCallback<EligibleAdsCallback<CreativeNewTabPageAdList>> callback;
  EXPECT_CALL(callback, Run(CreativeNewTabPageAdList{creative_ad_1}))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  eligible_ads_->GetForUserModel(
      UserModelInfo{IntentUserModelInfo{}, LatentInterestUserModelInfo{},
                    InterestUserModelInfo{SegmentList{"untargeted"}}},
      callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, GetAdsForNoMatchingSegments) {
  // Arrange
  CreativeNewTabPageAdList creative_ads;

  CreativeNewTabPageAdInfo creative_ad_1 =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  creative_ad_1.segment = "parent";
  creative_ads.push_back(creative_ad_1);

  CreativeNewTabPageAdInfo creative_ad_2 =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
  creative_ad_2.segment = "parent-child";
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNewTabPageAds(creative_ads);

  // Act & Assert
  base::RunLoop run_loop;
  base::MockCallback<EligibleAdsCallback<CreativeNewTabPageAdList>> callback;
  EXPECT_CALL(callback, Run(/*creative_ads=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  eligible_ads_->GetForUserModel(/*user_model=*/{}, callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsEligibleNewTabPageAdsV2Test, DoNotGetAdsIfNoEligibleAds) {
  // Act & Assert
  base::RunLoop run_loop;
  base::MockCallback<EligibleAdsCallback<CreativeNewTabPageAdList>> callback;
  EXPECT_CALL(callback, Run(/*creative_ads=*/::testing::IsEmpty()))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  eligible_ads_->GetForUserModel(
      UserModelInfo{
          IntentUserModelInfo{SegmentList{"parent-child", "parent"}},
          LatentInterestUserModelInfo{},
          InterestUserModelInfo{SegmentList{"parent-child", "parent"}}},
      callback.Get());
  run_loop.Run();
}

}  // namespace brave_ads
