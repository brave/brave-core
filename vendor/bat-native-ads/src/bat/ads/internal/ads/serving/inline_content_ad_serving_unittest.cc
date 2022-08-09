/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/inline_content_ad_serving.h"

#include <memory>

#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_unittest_util.h"
#include "bat/ads/internal/ads/serving/inline_content_ad_serving_observer.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/ads/serving/serving_features_unittest_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segment_alias.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace inline_content_ads {

class BatAdsInlineContentAdServingTest : public ServingObserver,
                                         public UnitTestBase {
 protected:
  BatAdsInlineContentAdServingTest() = default;

  ~BatAdsInlineContentAdServingTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    features::ForceServingVersion(1);

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    serving_ = std::make_unique<Serving>(subdivision_targeting_.get(),
                                         anti_targeting_resource_.get());
    serving_->AddObserver(this);
  }

  void TearDown() override {
    serving_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnOpportunityAroseToServeInlineContentAd(
      const SegmentList& segments) override {
    had_opportunuity_ = true;
  }

  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeInlineContentAd() override { failed_to_serve_ad_ = true; }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<Serving> serving_;

  InlineContentAdInfo ad_;
  bool had_opportunuity_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};

TEST_F(BatAdsInlineContentAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  features::ForceServingVersion(0);

  // Act
  serving_->MaybeServeAd("200x100",
                         [=](const std::string& dimensions,
                             const absl::optional<InlineContentAdInfo>& ad) {
                           // Assert
                           EXPECT_FALSE(ad);
                           EXPECT_FALSE(had_opportunuity_);
                           EXPECT_FALSE(did_serve_ad_);
                           EXPECT_TRUE(failed_to_serve_ad_);
                         });
}

TEST_F(BatAdsInlineContentAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRules();

  CreativeInlineContentAdList creative_ads;
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  SaveCreativeAds(creative_ads);

  // Act
  serving_->MaybeServeAd("200x100",
                         [=](const std::string& dimensions,
                             const absl::optional<InlineContentAdInfo>& ad) {
                           // Assert
                           EXPECT_TRUE(ad);
                           EXPECT_TRUE(had_opportunuity_);
                           EXPECT_TRUE(did_serve_ad_);
                           EXPECT_FALSE(failed_to_serve_ad_);
                           EXPECT_EQ(ad, ad_);
                         });
}

TEST_F(BatAdsInlineContentAdServingTest, DoNotServeAdForNonExistentDimensions) {
  // Arrange
  ForcePermissionRules();

  CreativeInlineContentAdList creative_ads;
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  SaveCreativeAds(creative_ads);

  // Act
  serving_->MaybeServeAd("?x?",
                         [=](const std::string& dimensions,
                             const absl::optional<InlineContentAdInfo>& ad) {
                           // Assert
                           EXPECT_FALSE(ad);
                           EXPECT_FALSE(had_opportunuity_);
                           EXPECT_FALSE(did_serve_ad_);
                           EXPECT_TRUE(failed_to_serve_ad_);
                         });

  // Assert
}

TEST_F(BatAdsInlineContentAdServingTest,
       DoNotServeAdIfNotAllowedDueToPermissionRules) {
  // Arrange
  CreativeInlineContentAdList creative_ads;
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  SaveCreativeAds(creative_ads);

  // Act
  serving_->MaybeServeAd("200x100",
                         [=](const std::string& dimensions,
                             const absl::optional<InlineContentAdInfo>& ad) {
                           // Assert
                           EXPECT_FALSE(ad);
                           EXPECT_FALSE(had_opportunuity_);
                           EXPECT_FALSE(did_serve_ad_);
                           EXPECT_TRUE(failed_to_serve_ad_);
                         });

  // Assert
}

}  // namespace inline_content_ads
}  // namespace ads
