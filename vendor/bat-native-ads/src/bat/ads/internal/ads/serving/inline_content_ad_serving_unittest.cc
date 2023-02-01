/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/inline_content_ad_serving.h"

#include <memory>

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ads/serving/inline_content_ad_serving_observer.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/ads/serving/serving_features_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segment_alias.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::inline_content_ads {

class BatAdsInlineContentAdServingObserver : public ServingObserver {
 public:
  void OnOpportunityAroseToServeInlineContentAd(
      const SegmentList& /*segments*/) override {
    had_opportunuity_ = true;
  }

  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnFailedToServeInlineContentAd() override { failed_to_serve_ad_ = true; }

  const InlineContentAdInfo& ad() const { return ad_; }

  bool had_opportunuity() const { return had_opportunuity_; }

  bool did_serve_ad() const { return did_serve_ad_; }

  bool failed_to_serve_ad() const { return failed_to_serve_ad_; }

 private:
  InlineContentAdInfo ad_;
  bool had_opportunuity_ = false;
  bool did_serve_ad_ = false;
  bool failed_to_serve_ad_ = false;
};
class BatAdsInlineContentAdServingTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    features::ForceServingVersion(1);

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    anti_targeting_resource_ = std::make_unique<resource::AntiTargeting>();
    serving_ = std::make_unique<Serving>(subdivision_targeting_.get(),
                                         anti_targeting_resource_.get());
    serving_->AddObserver(&serving_observer_);
  }

  void TearDown() override {
    serving_->RemoveObserver(&serving_observer_);

    UnitTestBase::TearDown();
  }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<resource::AntiTargeting> anti_targeting_resource_;
  std::unique_ptr<Serving> serving_;

  BatAdsInlineContentAdServingObserver serving_observer_;
};

TEST_F(BatAdsInlineContentAdServingTest, DoNotServeAdForUnsupportedVersion) {
  // Arrange
  features::ForceServingVersion(0);

  // Act
  serving_->MaybeServeAd(
      "200x100", base::BindOnce(
                     [](BatAdsInlineContentAdServingObserver* serving_observer,
                        const std::string& /*dimensions*/,
                        const absl::optional<InlineContentAdInfo>& ad) {
                       // Assert
                       EXPECT_FALSE(ad);
                       EXPECT_FALSE(serving_observer->had_opportunuity());
                       EXPECT_FALSE(serving_observer->did_serve_ad());
                       EXPECT_TRUE(serving_observer->failed_to_serve_ad());
                     },
                     base::Unretained(&serving_observer_)));
}

TEST_F(BatAdsInlineContentAdServingTest, ServeAd) {
  // Arrange
  ForcePermissionRulesForTesting();

  CreativeInlineContentAdList creative_ads;
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  SaveCreativeAds(creative_ads);

  // Act
  serving_->MaybeServeAd(
      "200x100", base::BindOnce(
                     [](BatAdsInlineContentAdServingObserver* serving_observer,
                        const std::string& /*dimensions*/,
                        const absl::optional<InlineContentAdInfo>& ad) {
                       // Assert
                       EXPECT_TRUE(ad);
                       EXPECT_TRUE(serving_observer->had_opportunuity());
                       EXPECT_TRUE(serving_observer->did_serve_ad());
                       EXPECT_FALSE(serving_observer->failed_to_serve_ad());
                       EXPECT_EQ(ad, serving_observer->ad());
                     },
                     base::Unretained(&serving_observer_)));
}

TEST_F(BatAdsInlineContentAdServingTest, DoNotServeAdForNonExistentDimensions) {
  // Arrange
  ForcePermissionRulesForTesting();

  CreativeInlineContentAdList creative_ads;
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd();
  creative_ads.push_back(creative_ad);
  SaveCreativeAds(creative_ads);

  // Act
  serving_->MaybeServeAd(
      "?x?", base::BindOnce(
                 [](BatAdsInlineContentAdServingObserver* serving_observer,
                    const std::string& /*dimensions*/,
                    const absl::optional<InlineContentAdInfo>& ad) {
                   // Assert
                   EXPECT_FALSE(ad);
                   EXPECT_FALSE(serving_observer->had_opportunuity());
                   EXPECT_FALSE(serving_observer->did_serve_ad());
                   EXPECT_TRUE(serving_observer->failed_to_serve_ad());
                 },
                 base::Unretained(&serving_observer_)));

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
  serving_->MaybeServeAd(
      "200x100", base::BindOnce(
                     [](BatAdsInlineContentAdServingObserver* serving_observer,
                        const std::string& /*dimensions*/,
                        const absl::optional<InlineContentAdInfo>& ad) {
                       // Assert
                       EXPECT_FALSE(ad);
                       EXPECT_FALSE(serving_observer->had_opportunuity());
                       EXPECT_FALSE(serving_observer->did_serve_ad());
                       EXPECT_TRUE(serving_observer->failed_to_serve_ad());
                     },
                     base::Unretained(&serving_observer_)));

  // Assert
}

}  // namespace ads::inline_content_ads
