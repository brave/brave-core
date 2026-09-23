/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creatives_builder.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/catalog_campaign_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_conversion_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_creative_set_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_os_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_segment_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creatives_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativesBuilderTest : public test::TestBase {};

TEST_F(BraveAdsCreativesBuilderTest, BuildCreativesFromEmptyCatalog) {
  // Arrange
  const CatalogInfo catalog;

  // Act
  const CreativesInfo creatives = BuildCreatives(catalog);

  // Assert
  EXPECT_THAT(creatives.notification_ads, ::testing::IsEmpty());
  EXPECT_THAT(creatives.conversions, ::testing::IsEmpty());
}

TEST_F(BraveAdsCreativesBuilderTest, BuildCreatives) {
  // Arrange
  CatalogCreativeNotificationAdInfo creative;
  creative.instance_id = test::kCreativeInstanceId;
  creative.payload.title = test::kTitle;
  creative.payload.body = test::kDescription;
  creative.payload.target_url = GURL(test::kTargetUrl);

  CatalogSegmentInfo segment;
  segment.name = test::kSegment;

  CatalogCreativeSetInfo creative_set;
  creative_set.id = test::kCreativeSetId;
  creative_set.per_day = 1;
  creative_set.per_week = 7;
  creative_set.per_month = 30;
  creative_set.total_max = 100;
  creative_set.value = test::kValue;
  creative_set.segments = {segment};
  creative_set.creative_notification_ads = {creative};

  CatalogCampaignInfo campaign;
  campaign.id = test::kCampaignId;
  campaign.advertiser_id = test::kAdvertiserId;
  campaign.priority = 1;
  campaign.pass_through_rate = 1.0;
  campaign.daily_cap = 10;
  campaign.creative_sets = {creative_set};

  CatalogInfo catalog;
  catalog.campaigns = {campaign};

  // Act
  const CreativesInfo creatives = BuildCreatives(catalog);

  // Assert
  ASSERT_EQ(1U, creatives.notification_ads.size());
  const CreativeNotificationAdInfo& ad = creatives.notification_ads.front();
  EXPECT_EQ(test::kCreativeInstanceId, ad.creative_instance_id);
  EXPECT_EQ(test::kCreativeSetId, ad.creative_set_id);
  EXPECT_EQ(test::kCampaignId, ad.campaign_id);
  EXPECT_EQ(test::kAdvertiserId, ad.advertiser_id);
  EXPECT_EQ(1, ad.priority);
  EXPECT_EQ(1.0, ad.pass_through_rate);
  EXPECT_EQ(10, ad.daily_cap);
  EXPECT_EQ(1, ad.per_day);
  EXPECT_EQ(7, ad.per_week);
  EXPECT_EQ(30, ad.per_month);
  EXPECT_EQ(100, ad.total_max);
  EXPECT_EQ(test::kValue, ad.value);
  EXPECT_EQ(test::kSegment, ad.segment);
  EXPECT_EQ(test::kTitle, ad.title);
  EXPECT_EQ(test::kDescription, ad.body);
  EXPECT_EQ(GURL(test::kTargetUrl), ad.target_url);
  EXPECT_THAT(creatives.conversions, ::testing::IsEmpty());
}

TEST_F(BraveAdsCreativesBuilderTest,
       BuildCreativesProducesOneAdPerCreativeSetSegment) {
  // Arrange
  CatalogCreativeNotificationAdInfo creative;
  creative.instance_id = test::kCreativeInstanceId;

  CatalogSegmentInfo foo_segment;
  foo_segment.name = "foo";
  CatalogSegmentInfo bar_segment;
  bar_segment.name = "BAR";

  CatalogCreativeSetInfo creative_set;
  creative_set.id = test::kCreativeSetId;
  creative_set.segments = {foo_segment, bar_segment};
  creative_set.creative_notification_ads = {creative};

  CatalogCampaignInfo campaign;
  campaign.id = test::kCampaignId;
  campaign.advertiser_id = test::kAdvertiserId;
  campaign.creative_sets = {creative_set};

  CatalogInfo catalog;
  catalog.campaigns = {campaign};

  // Act
  const CreativesInfo creatives = BuildCreatives(catalog);

  // Assert
  ASSERT_EQ(2U, creatives.notification_ads.size());
  EXPECT_EQ("foo", creatives.notification_ads.at(0).segment);
  EXPECT_EQ("bar", creatives.notification_ads.at(1).segment);
}

TEST_F(
    BraveAdsCreativesBuilderTest,
    DoesNotBuildNotificationAdsWhenCreativeSetDoesNotSupportCurrentOperatingSystem) {
  // Arrange
  CatalogCreativeNotificationAdInfo creative;
  creative.instance_id = test::kCreativeInstanceId;

  CatalogSegmentInfo segment;
  segment.name = test::kSegment;

  CatalogOsInfo unsupported_os;
  unsupported_os.name = "FakeOS";

  CatalogCreativeSetInfo creative_set;
  creative_set.id = test::kCreativeSetId;
  creative_set.oses = {unsupported_os};
  creative_set.segments = {segment};
  creative_set.creative_notification_ads = {creative};

  CatalogCampaignInfo campaign;
  campaign.id = test::kCampaignId;
  campaign.advertiser_id = test::kAdvertiserId;
  campaign.creative_sets = {creative_set};

  CatalogInfo catalog;
  catalog.campaigns = {campaign};

  // Act
  const CreativesInfo creatives = BuildCreatives(catalog);

  // Assert
  EXPECT_THAT(creatives.notification_ads, ::testing::IsEmpty());
}

TEST_F(BraveAdsCreativesBuilderTest,
       DoesNotBuildNotificationAdsWhenCreativeSetHasNoNotificationAdCreatives) {
  // Arrange
  CatalogSegmentInfo segment;
  segment.name = test::kSegment;

  CatalogConversionInfo conversion;
  conversion.creative_set_id = test::kCreativeSetId;
  conversion.url_pattern = "https://example.com/*";
  conversion.observation_window = base::Days(7);
  conversion.expire_at = test::Now() + base::Days(7);

  CatalogCreativeSetInfo creative_set;
  creative_set.id = test::kCreativeSetId;
  creative_set.segments = {segment};
  creative_set.conversions = {conversion};

  CatalogCampaignInfo campaign;
  campaign.id = test::kCampaignId;
  campaign.advertiser_id = test::kAdvertiserId;
  campaign.creative_sets = {creative_set};

  CatalogInfo catalog;
  catalog.campaigns = {campaign};

  // Act
  const CreativesInfo creatives = BuildCreatives(catalog);

  // Assert
  EXPECT_THAT(creatives.notification_ads, ::testing::IsEmpty());
  EXPECT_THAT(creatives.conversions, ::testing::IsEmpty());
}

TEST_F(BraveAdsCreativesBuilderTest, BuildCreativesIncludesValidConversion) {
  // Arrange
  CatalogCreativeNotificationAdInfo creative;
  creative.instance_id = test::kCreativeInstanceId;

  CatalogSegmentInfo segment;
  segment.name = test::kSegment;

  CatalogConversionInfo conversion;
  conversion.creative_set_id = test::kCreativeSetId;
  conversion.url_pattern = "https://example.com/*";
  conversion.observation_window = base::Days(7);
  conversion.expire_at = test::Now() + base::Days(7);

  CatalogCreativeSetInfo creative_set;
  creative_set.id = test::kCreativeSetId;
  creative_set.segments = {segment};
  creative_set.creative_notification_ads = {creative};
  creative_set.conversions = {conversion};

  CatalogCampaignInfo campaign;
  campaign.id = test::kCampaignId;
  campaign.advertiser_id = test::kAdvertiserId;
  campaign.creative_sets = {creative_set};

  CatalogInfo catalog;
  catalog.campaigns = {campaign};

  // Act
  const CreativesInfo creatives = BuildCreatives(catalog);

  // Assert
  ASSERT_EQ(1U, creatives.conversions.size());
  EXPECT_EQ(test::kCreativeSetId, creatives.conversions.front().id);
  EXPECT_EQ("https://example.com/*", creatives.conversions.front().url_pattern);
  EXPECT_EQ(base::Days(7), creatives.conversions.front().observation_window);
}

TEST_F(BraveAdsCreativesBuilderTest,
       DoesNotBuildCreativesWithInvalidConversion) {
  // Arrange
  CatalogCreativeNotificationAdInfo creative;
  creative.instance_id = test::kCreativeInstanceId;

  CatalogSegmentInfo segment;
  segment.name = test::kSegment;

  CatalogConversionInfo invalid_conversion;
  invalid_conversion.creative_set_id = test::kCreativeSetId;
  invalid_conversion.url_pattern = "";  // Empty pattern makes it invalid.
  invalid_conversion.observation_window = base::Days(7);
  invalid_conversion.expire_at = test::Now() + base::Days(7);

  CatalogCreativeSetInfo creative_set;
  creative_set.id = test::kCreativeSetId;
  creative_set.segments = {segment};
  creative_set.creative_notification_ads = {creative};
  creative_set.conversions = {invalid_conversion};

  CatalogCampaignInfo campaign;
  campaign.id = test::kCampaignId;
  campaign.advertiser_id = test::kAdvertiserId;
  campaign.creative_sets = {creative_set};

  CatalogInfo catalog;
  catalog.campaigns = {campaign};

  // Act
  const CreativesInfo creatives = BuildCreatives(catalog);

  // Assert
  EXPECT_THAT(creatives.conversions, ::testing::IsEmpty());
}

}  // namespace brave_ads
