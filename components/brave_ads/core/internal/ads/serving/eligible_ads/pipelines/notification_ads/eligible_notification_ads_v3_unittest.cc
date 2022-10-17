/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v3.h"

#include <memory>

#include "bat/ads/internal/ads/serving/targeting/user_model_builder_unittest_util.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_unittest_util.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::notification_ads {

class BatAdsEligibleNotificationAdsV3Test : public UnitTestBase {
 protected:
  BatAdsEligibleNotificationAdsV3Test() = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    eligible_ads_ = std::make_unique<EligibleAdsV3>(
        subdivision_targeting_.get(), anti_targeting_resource_.get());
  }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<EligibleAdsV3> eligible_ads_;
};

TEST_F(BatAdsEligibleNotificationAdsV3Test, GetAds) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd();
  creative_ad_1.embedding = {0.1, 0.2, 0.3, 0.4, 0.5};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 = BuildCreativeNotificationAd();
  creative_ad_2.embedding = {-0.3, 0.0, -0.2, 0.6, 0.8};
  creative_ads.push_back(creative_ad_2);

  const TextEmbeddingHtmlEventInfo text_embedding_event =
      BuildTextEmbeddingHtmlEvent(BuildTextEmbedding());

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({}, {}, {}, {text_embedding_event}),
      [](const bool had_opportunity,
         const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(!creative_ads.empty());
      });
}

TEST_F(BatAdsEligibleNotificationAdsV3Test, GetAdsForNoEmbeddings) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd();
  creative_ad_1.embedding = {0.1, 0.2, 0.3, 0.4, 0.5};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 = BuildCreativeNotificationAd();
  creative_ad_2.embedding = {-0.3, 0.0, -0.2, 0.6, 0.8};
  creative_ads.push_back(creative_ad_2);

  SaveCreativeAds(creative_ads);

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({}, {}, {}, {}),
      [](const bool had_opportunity,
         const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(!creative_ads.empty());
      });
}

TEST_F(BatAdsEligibleNotificationAdsV3Test, DoNotGetAdsIfNoEligibleAds) {
  // Arrange
  const TextEmbeddingHtmlEventInfo text_embedding_event =
      BuildTextEmbeddingHtmlEvent(BuildTextEmbedding());

  // Act
  eligible_ads_->GetForUserModel(
      targeting::BuildUserModel({}, {}, {}, {text_embedding_event}),
      [](const bool had_opportunity,
         const CreativeNotificationAdList& creative_ads) {
        // Assert
        EXPECT_FALSE(had_opportunity);
        EXPECT_TRUE(creative_ads.empty());
      });
}

}  // namespace ads::notification_ads
