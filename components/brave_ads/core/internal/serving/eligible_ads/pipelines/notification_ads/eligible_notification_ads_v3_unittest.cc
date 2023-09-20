/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v3.h"

#include <memory>
#include <vector>

#include "base/functional/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEligibleNotificationAdsV3Test : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<AntiTargetingResource>();
    eligible_ads_ = std::make_unique<EligibleNotificationAdsV3>(
        *subdivision_targeting_, *anti_targeting_resource_);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AntiTargetingResource> anti_targeting_resource_;
  std::unique_ptr<EligibleNotificationAdsV3> eligible_ads_;
};

TEST_F(BraveAdsEligibleNotificationAdsV3Test, GetAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.embedding = {0.1, 0.2, 0.3};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.embedding = {-0.3, 0.0, -0.2};
  creative_ads.push_back(creative_ad_2);

  const TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::BuildTextEmbeddingForTesting());

  database::SaveCreativeNotificationAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {},
          /*text_embedding_html_events*/ {text_embedding_html_event}),
      base::BindOnce(
          [](const CreativeNotificationAdInfo& creative_ad_1,
             const CreativeNotificationAdList& creative_ads) {
            // Assert
            EXPECT_FALSE(creative_ads.empty());

            EXPECT_EQ(creative_ads.at(0).embedding, creative_ad_1.embedding);
          },
          creative_ad_1));
}

TEST_F(BraveAdsEligibleNotificationAdsV3Test, GetAdsForNoStoredTextEmbeddings) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.embedding = {0.1, 0.2, 0.3, 0.4, 0.5};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.embedding = {-0.3, 0.0, -0.2, 0.6, 0.8};
  creative_ads.push_back(creative_ad_2);

  database::SaveCreativeNotificationAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*latent_interest_segments*/ {},
          /*text_embedding_html_events*/ {}),
      base::BindOnce([](const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleNotificationAdsV3Test,
       GetAdsForCreativeWithoutEmbeddingProperty) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["version"] = "3";
  enabled_features.emplace_back(kNotificationAdServingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  const CreativeNotificationAdList creative_ads =
      BuildCreativeNotificationAdsForTesting(/*count*/ 2);
  database::SaveCreativeNotificationAds(creative_ads);

  const TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::BuildTextEmbeddingForTesting());

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {},
          /*text_embedding_html_events*/ {text_embedding_html_event}),
      base::BindOnce([](const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(creative_ads.empty());
      }));
}

TEST_F(BraveAdsEligibleNotificationAdsV3Test, DoNotGetAdsIfNoEligibleAds) {
  // Arrange
  const TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::BuildTextEmbeddingForTesting());

  // Act
  eligible_ads_->GetForUserModel(
      BuildUserModelForTesting(
          /*intent_segments*/ {},
          /*latent_interest_segments*/ {},
          /*interest_segments*/ {},
          /*text_embedding_html_events*/ {text_embedding_html_event}),
      base::BindOnce([](const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_TRUE(creative_ads.empty());
      }));
}

}  // namespace brave_ads
