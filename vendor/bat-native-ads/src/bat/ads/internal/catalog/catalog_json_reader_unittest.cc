/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_json_reader.h"

#include "base/time/time.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"
#include "bat/ads/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_focal_point_info.h"
#include "bat/ads/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_info.h"
#include "bat/ads/internal/catalog/catalog_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kInvalidCatalog[] = "INVALID_JSON";
constexpr char kEmptyCatalog[] = "empty_catalog.json";
constexpr char kCatalogWithSingleCampaign[] =
    "catalog_with_single_campaign.json";
constexpr char kCatalogWithMultipleCampaigns[] =
    "catalog_with_multiple_campaigns.json";

}  // namespace

class BatAdsCatalogTest : public UnitTestBase {
 protected:
  BatAdsCatalogTest() = default;

  ~BatAdsCatalogTest() override = default;

  CatalogCampaignInfo BuildCatalogCampaign1() {
    // Segments
    CatalogSegmentList catalog_segments;

    CatalogSegmentInfo catalog_segment;
    catalog_segment.code = "yNl0N-ers2";
    catalog_segment.name = "technology & computing";
    catalog_segments.push_back(catalog_segment);

    // OSes
    CatalogOsList catalog_oses;

    CatalogOsInfo catalog_os_1;
    catalog_os_1.code = "-Ug5OXisJ";
    catalog_os_1.name = "linux";
    catalog_oses.push_back(catalog_os_1);

    CatalogOsInfo catalog_os_2;
    catalog_os_2.code = "_Bt5nxrNo";
    catalog_os_2.name = "macos";
    catalog_oses.push_back(catalog_os_2);

    CatalogOsInfo catalog_os_3;
    catalog_os_3.code = "i1g4cO6Pl";
    catalog_os_3.name = "windows";
    catalog_oses.push_back(catalog_os_3);

    // Creative Notification Ads
    CatalogCreativeNotificationAdList catalog_creative_notification_ads;

    CatalogCreativeNotificationAdInfo catalog_creative_notification_ad;
    catalog_creative_notification_ad.creative_instance_id =
        "87c775ca-919b-4a87-8547-94cf0c3161a2";
    CatalogTypeInfo catalog_notification_ad_type;
    catalog_notification_ad_type.code = "notification_all_v1";
    catalog_notification_ad_type.name = "notification";
    catalog_notification_ad_type.platform = "all";
    catalog_notification_ad_type.version = 1;
    catalog_creative_notification_ad.type = catalog_notification_ad_type;
    catalog_creative_notification_ad.payload.body =
        "Test Notification Ad Campaign 1 Body";
    catalog_creative_notification_ad.payload.title =
        "Test Notification Ad Campaign 1 Title";
    catalog_creative_notification_ad.payload.target_url =
        GURL("https://brave.com/1/notification_ad");
    catalog_creative_notification_ads.push_back(
        catalog_creative_notification_ad);

    // Creative New Tab Page Ads
    CatalogCreativeNewTabPageAdList catalog_creative_new_tab_page_ads;

    CatalogCreativeNewTabPageAdInfo catalog_creative_new_tab_page_ad;
    catalog_creative_new_tab_page_ad.creative_instance_id =
        "7ff400b9-7f8a-46a8-89f1-cb386612edcf";
    CatalogTypeInfo catalog_type_new_tab_page_ad;
    catalog_type_new_tab_page_ad.code = "new_tab_page_all_v1";
    catalog_type_new_tab_page_ad.name = "new_tab_page";
    catalog_type_new_tab_page_ad.platform = "all";
    catalog_type_new_tab_page_ad.version = 1;
    catalog_creative_new_tab_page_ad.type = catalog_type_new_tab_page_ad;
    catalog_creative_new_tab_page_ad.payload.company_name = "New Tab Page 1";
    catalog_creative_new_tab_page_ad.payload.image_url =
        GURL("https://brave.com/1/test.jpg");
    catalog_creative_new_tab_page_ad.payload.alt =
        "Test New Tab Page Ad Campaign 1";
    catalog_creative_new_tab_page_ad.payload.target_url =
        GURL("https://brave.com/1/new_tab_page_ad");
    CatalogNewTabPageAdWallpaperInfo wallpaper_1;
    wallpaper_1.image_url = GURL("https://brave.com/1/test2.jpg");
    CatalogNewTabPageAdWallpaperFocalPointInfo focal_point_1;
    focal_point_1.x = 1200;
    focal_point_1.y = 1400;
    wallpaper_1.focal_point = focal_point_1;
    catalog_creative_new_tab_page_ad.payload.wallpapers.push_back(wallpaper_1);
    CatalogNewTabPageAdWallpaperInfo wallpaper_2;
    wallpaper_2.image_url = GURL("https://brave.com/1/test3.jpg");
    CatalogNewTabPageAdWallpaperFocalPointInfo focal_point_2;
    focal_point_2.x = 1200;
    focal_point_2.y = 1400;
    wallpaper_2.focal_point = focal_point_2;
    catalog_creative_new_tab_page_ad.payload.wallpapers.push_back(wallpaper_2);

    catalog_creative_new_tab_page_ads.push_back(
        catalog_creative_new_tab_page_ad);

    // Creative Promoted Content Ads
    CatalogCreativePromotedContentAdList catalog_creative_promoted_content_ads;

    CatalogCreativePromotedContentAdInfo catalog_creative_promoted_content_ad;
    catalog_creative_promoted_content_ad.creative_instance_id =
        "60001aa5-9368-45d2-81fc-e69887d278c5";
    CatalogTypeInfo catalog_type_promoted_content_ad_type;
    catalog_type_promoted_content_ad_type.code = "promoted_content_all_v1";
    catalog_type_promoted_content_ad_type.name = "promoted_content";
    catalog_type_promoted_content_ad_type.platform = "all";
    catalog_type_promoted_content_ad_type.version = 1;
    catalog_creative_promoted_content_ad.type =
        catalog_type_promoted_content_ad_type;
    catalog_creative_promoted_content_ad.payload.title = "Promoted Content 1";
    catalog_creative_promoted_content_ad.payload.description =
        "Test Promoted Content Ad Campaign 1";
    catalog_creative_promoted_content_ad.payload.target_url =
        GURL("https://brave.com/1/promoted_content_ad");
    catalog_creative_promoted_content_ads.push_back(
        catalog_creative_promoted_content_ad);

    // Creative Inline Content Ads
    CatalogCreativeInlineContentAdList catalog_creative_inline_content_ads;

    CatalogCreativeInlineContentAdInfo catalog_creative_inline_content_ad;
    catalog_creative_inline_content_ad.creative_instance_id =
        "30db5f7b-dba3-48a3-b299-c9bd9c67da65";
    CatalogTypeInfo catalog_type_inline_content_ad_type;
    catalog_type_inline_content_ad_type.code = "inline_content_all_v1";
    catalog_type_inline_content_ad_type.name = "inline_content";
    catalog_type_inline_content_ad_type.platform = "all";
    catalog_type_inline_content_ad_type.version = 1;
    catalog_creative_inline_content_ad.type =
        catalog_type_inline_content_ad_type;
    catalog_creative_inline_content_ad.payload.title = "Inline Content 1";
    catalog_creative_inline_content_ad.payload.description =
        "Test Inline Content Ad Campaign 1";
    catalog_creative_inline_content_ad.payload.image_url =
        GURL("https://www.brave.com/1/image.png");
    catalog_creative_inline_content_ad.payload.dimensions = "200x100";
    catalog_creative_inline_content_ad.payload.cta_text =
        "Call to Action Text 1";
    catalog_creative_inline_content_ad.payload.target_url =
        GURL("https://brave.com/1/inline_content_ad");
    catalog_creative_inline_content_ads.push_back(
        catalog_creative_inline_content_ad);

    // Conversions
    ConversionList conversions;

    ConversionInfo conversion;
    conversion.creative_set_id = "340c927f-696e-4060-9933-3eafc56c3f31";
    conversion.type = "postview";
    conversion.url_pattern = "https://www.brave.com/1/*";
    conversion.observation_window = 30;
    conversion.expire_at = base::Time::FromDoubleT(4105036799);
    conversions.push_back(conversion);

    // Creative Sets
    CatalogCreativeSetList catalog_creative_sets;

    CatalogCreativeSetInfo catalog_creative_set;
    catalog_creative_set.creative_set_id =
        "340c927f-696e-4060-9933-3eafc56c3f31";
    catalog_creative_set.per_day = 5;
    catalog_creative_set.per_week = 6;
    catalog_creative_set.per_month = 7;
    catalog_creative_set.split_test_group = "GroupB";
    catalog_creative_set.total_max = 100;
    catalog_creative_set.value = 0.05;
    catalog_creative_set.segments = catalog_segments;
    catalog_creative_set.oses = catalog_oses;
    catalog_creative_set.creative_notification_ads =
        catalog_creative_notification_ads;
    catalog_creative_set.creative_inline_content_ads =
        catalog_creative_inline_content_ads;
    catalog_creative_set.creative_new_tab_page_ads =
        catalog_creative_new_tab_page_ads;
    catalog_creative_set.creative_promoted_content_ads =
        catalog_creative_promoted_content_ads;
    catalog_creative_set.conversions = conversions;
    catalog_creative_sets.push_back(catalog_creative_set);

    // Dayparts
    CatalogDaypartList catalog_dayparts;

    CatalogDaypartInfo catalog_daypart_1;
    catalog_daypart_1.dow = "012";
    catalog_daypart_1.start_minute = 0;
    catalog_daypart_1.end_minute = 1439;
    catalog_dayparts.push_back(catalog_daypart_1);

    CatalogDaypartInfo catalog_daypart_2;
    catalog_daypart_2.dow = "345";
    catalog_daypart_2.start_minute = 1000;
    catalog_daypart_2.end_minute = 1200;
    catalog_dayparts.push_back(catalog_daypart_2);

    // Geo Targets
    CatalogGeoTargetList catalog_geo_targets;

    CatalogGeoTargetInfo catalog_geo_target;
    catalog_geo_target.code = "US";
    catalog_geo_target.name = "United States";
    catalog_geo_targets.push_back(catalog_geo_target);

    // Campaign
    CatalogCampaignInfo catalog_campaign;

    catalog_campaign.campaign_id = "27a624a1-9c80-494a-bf1b-af327b563f85";
    catalog_campaign.priority = 1;
    catalog_campaign.ptr = 1.0;
    catalog_campaign.start_at = DistantPastAsISO8601();
    catalog_campaign.end_at = DistantFutureAsISO8601();
    catalog_campaign.daily_cap = 10;
    catalog_campaign.advertiser_id = "a437c7f3-9a48-4fe8-b37b-99321bea93fe";
    catalog_campaign.creative_sets = catalog_creative_sets;
    catalog_campaign.dayparts = catalog_dayparts;
    catalog_campaign.geo_targets = catalog_geo_targets;

    return catalog_campaign;
  }

  CatalogCampaignInfo BuildCatalogCampaign2() {
    // Segments
    CatalogSegmentList catalog_segments;

    CatalogSegmentInfo catalog_segment;
    catalog_segment.code = "Svp7l-zGN";
    catalog_segment.name = "untargeted";
    catalog_segments.push_back(catalog_segment);

    // OSes
    CatalogOsList catalog_oses;

    CatalogOsInfo catalog_os_1;
    catalog_os_1.code = "mbwfZU-4W";
    catalog_os_1.name = "android";
    catalog_oses.push_back(catalog_os_1);

    CatalogOsInfo catalog_os_2;
    catalog_os_2.code = "k80syyzDa";
    catalog_os_2.name = "ios";
    catalog_oses.push_back(catalog_os_2);

    // Creative Notification Ads
    CatalogCreativeNotificationAdList catalog_creative_notification_ads;

    CatalogCreativeNotificationAdInfo catalog_creative_notification_ad;
    catalog_creative_notification_ad.creative_instance_id =
        "17206fbd-0282-4759-ad28-d5e040ee1ff7";
    CatalogTypeInfo catalog_notification_ad_type;
    catalog_notification_ad_type.code = "notification_all_v1";
    catalog_notification_ad_type.name = "notification";
    catalog_notification_ad_type.platform = "all";
    catalog_notification_ad_type.version = 1;
    catalog_creative_notification_ad.type = catalog_notification_ad_type;
    catalog_creative_notification_ad.payload.body =
        "Test Notification Ad Campaign 2 Body";
    catalog_creative_notification_ad.payload.title =
        "Test Notification Ad Campaign 2 Title";
    catalog_creative_notification_ad.payload.target_url =
        GURL("https://brave.com/2/notification_ad");
    catalog_creative_notification_ads.push_back(
        catalog_creative_notification_ad);

    // Creative New Tab Page Ads
    CatalogCreativeNewTabPageAdList catalog_creative_new_tab_page_ads;

    CatalogCreativeNewTabPageAdInfo catalog_creative_new_tab_page_ad;
    catalog_creative_new_tab_page_ad.creative_instance_id =
        "3dfe54d0-80b7-48d7-9bcc-3c77a912f583";
    CatalogTypeInfo catalog_type_new_tab_page_ad;
    catalog_type_new_tab_page_ad.code = "new_tab_page_all_v1";
    catalog_type_new_tab_page_ad.name = "new_tab_page";
    catalog_type_new_tab_page_ad.platform = "all";
    catalog_type_new_tab_page_ad.version = 1;
    catalog_creative_new_tab_page_ad.type = catalog_type_new_tab_page_ad;
    catalog_creative_new_tab_page_ad.payload.company_name = "New Tab Page 2";
    catalog_creative_new_tab_page_ad.payload.image_url =
        GURL("https://brave.com/2/test.jpg");
    catalog_creative_new_tab_page_ad.payload.alt =
        "Test New Tab Page Ad Campaign 2";
    catalog_creative_new_tab_page_ad.payload.target_url =
        GURL("https://brave.com/2/new_tab_page_ad");
    CatalogNewTabPageAdWallpaperInfo wallpaper_1;
    wallpaper_1.image_url = GURL("https://brave.com/2/test2.jpg");
    CatalogNewTabPageAdWallpaperFocalPointInfo focal_point_1;
    focal_point_1.x = 1000;
    focal_point_1.y = 1200;
    wallpaper_1.focal_point = focal_point_1;
    catalog_creative_new_tab_page_ad.payload.wallpapers.push_back(wallpaper_1);
    CatalogNewTabPageAdWallpaperInfo wallpaper_2;
    wallpaper_2.image_url = GURL("https://brave.com/2/test3.jpg");
    CatalogNewTabPageAdWallpaperFocalPointInfo focal_point_2;
    focal_point_2.x = 500;
    focal_point_2.y = 600;
    wallpaper_2.focal_point = focal_point_2;
    catalog_creative_new_tab_page_ad.payload.wallpapers.push_back(wallpaper_2);

    catalog_creative_new_tab_page_ads.push_back(
        catalog_creative_new_tab_page_ad);

    // Creative Promoted Content Ads
    CatalogCreativePromotedContentAdList catalog_creative_promoted_content_ads;

    CatalogCreativePromotedContentAdInfo catalog_creative_promoted_content_ad;
    catalog_creative_promoted_content_ad.creative_instance_id =
        "9f2f49ab-77d7-4e99-9428-472dc8e04f90";
    CatalogTypeInfo catalog_type_promoted_content_ad_type;
    catalog_type_promoted_content_ad_type.code = "promoted_content_all_v1";
    catalog_type_promoted_content_ad_type.name = "promoted_content";
    catalog_type_promoted_content_ad_type.platform = "all";
    catalog_type_promoted_content_ad_type.version = 1;
    catalog_creative_promoted_content_ad.type =
        catalog_type_promoted_content_ad_type;
    catalog_creative_promoted_content_ad.payload.title = "Promoted Content 2";
    catalog_creative_promoted_content_ad.payload.description =
        "Test Promoted Content Ad Campaign 2";
    catalog_creative_promoted_content_ad.payload.target_url =
        GURL("https://brave.com/2/promoted_content_ad");
    catalog_creative_promoted_content_ads.push_back(
        catalog_creative_promoted_content_ad);

    // Creative Inline Content Ads
    CatalogCreativeInlineContentAdList catalog_creative_inline_content_ads;

    CatalogCreativeInlineContentAdInfo catalog_creative_inline_content_ad;
    catalog_creative_inline_content_ad.creative_instance_id =
        "de54add5-ba76-469d-891f-b4d9f8e09b3d";
    CatalogTypeInfo catalog_type_inline_content_ad_type;
    catalog_type_inline_content_ad_type.code = "inline_content_all_v1";
    catalog_type_inline_content_ad_type.name = "inline_content";
    catalog_type_inline_content_ad_type.platform = "all";
    catalog_type_inline_content_ad_type.version = 1;
    catalog_creative_inline_content_ad.type =
        catalog_type_inline_content_ad_type;
    catalog_creative_inline_content_ad.payload.title = "Inline Content 2";
    catalog_creative_inline_content_ad.payload.description =
        "Test Inline Content Ad Campaign 2";
    catalog_creative_inline_content_ad.payload.image_url =
        GURL("https://www.brave.com/2/image.png");
    catalog_creative_inline_content_ad.payload.dimensions = "100x200";
    catalog_creative_inline_content_ad.payload.cta_text =
        "Call to Action Text 2";
    catalog_creative_inline_content_ad.payload.target_url =
        GURL("https://brave.com/2/inline_content_ad");
    catalog_creative_inline_content_ads.push_back(
        catalog_creative_inline_content_ad);

    // Conversions
    ConversionList conversions;

    ConversionInfo conversion;
    conversion.creative_set_id = "741cd2ba-3100-45f2-be1e-acedd24e0067";
    conversion.type = "postclick";
    conversion.url_pattern = "https://www.brave.com/2/*";
    conversion.observation_window = 7;
    conversion.advertiser_public_key = "";
    conversion.expire_at = base::Time::FromDoubleT(4103049599);
    conversions.push_back(conversion);

    // Creative Sets
    CatalogCreativeSetList catalog_creative_sets;

    CatalogCreativeSetInfo catalog_creative_set;
    catalog_creative_set.creative_set_id =
        "741cd2ba-3100-45f2-be1e-acedd24e0067";
    catalog_creative_set.per_day = 10;
    catalog_creative_set.per_week = 11;
    catalog_creative_set.per_month = 12;
    catalog_creative_set.total_max = 1000;
    catalog_creative_set.value = 0.1;
    catalog_creative_set.segments = catalog_segments;
    catalog_creative_set.oses = catalog_oses;
    catalog_creative_set.creative_notification_ads =
        catalog_creative_notification_ads;
    catalog_creative_set.creative_new_tab_page_ads =
        catalog_creative_new_tab_page_ads;
    catalog_creative_set.creative_promoted_content_ads =
        catalog_creative_promoted_content_ads;
    catalog_creative_set.creative_inline_content_ads =
        catalog_creative_inline_content_ads;
    catalog_creative_set.conversions = conversions;
    catalog_creative_sets.push_back(catalog_creative_set);

    // Dayparts
    CatalogDaypartList catalog_dayparts;

    CatalogDaypartInfo catalog_daypart;
    catalog_daypart.dow = "0123456";
    catalog_daypart.start_minute = 0;
    catalog_daypart.end_minute = 1439;
    catalog_dayparts.push_back(catalog_daypart);

    // Geo Targets
    CatalogGeoTargetList catalog_geo_targets;

    CatalogGeoTargetInfo catalog_geo_target;
    catalog_geo_target.code = "US";
    catalog_geo_target.name = "United States";
    catalog_geo_targets.push_back(catalog_geo_target);

    // Campaign
    CatalogCampaignInfo catalog_campaign;
    catalog_campaign.campaign_id = "856fc4bc-a21b-4582-bab7-a20d412359aa";
    catalog_campaign.priority = 2;
    catalog_campaign.ptr = 0.5;
    catalog_campaign.start_at = DistantPastAsISO8601();
    catalog_campaign.end_at = DistantFutureAsISO8601();
    catalog_campaign.daily_cap = 25;
    catalog_campaign.advertiser_id = "7523854c-5f28-4153-9da8-d9da6804ed58";
    catalog_campaign.creative_sets = catalog_creative_sets;
    catalog_campaign.dayparts = catalog_dayparts;
    catalog_campaign.geo_targets = catalog_geo_targets;

    return catalog_campaign;
  }
};

TEST_F(BatAdsCatalogTest, ParseCatalogWithSingleCampaign) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathAndParseTagsToString(kCatalogWithSingleCampaign);
  ASSERT_TRUE(json);

  // Act
  const absl::optional<CatalogInfo> catalog = JSONReader::ReadCatalog(*json);
  ASSERT_TRUE(catalog);

  // Assert
  CatalogInfo expected_catalog;
  expected_catalog.id = "29e5c8bc0ba319069980bb390d8e8f9b58c05a20";
  expected_catalog.version = 9;
  expected_catalog.ping = base::Milliseconds(7200000);
  expected_catalog.campaigns.push_back(BuildCatalogCampaign1());

  EXPECT_EQ(expected_catalog, *catalog);
}

TEST_F(BatAdsCatalogTest, ParseCatalogWithMultipleCampaigns) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathAndParseTagsToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(json);

  // Act
  const absl::optional<CatalogInfo> catalog = JSONReader::ReadCatalog(*json);
  ASSERT_TRUE(catalog);

  // Assert
  CatalogInfo expected_catalog;
  expected_catalog.id = "29e5c8bc0ba319069980bb390d8e8f9b58c05a20";
  expected_catalog.version = 9;
  expected_catalog.ping = base::Milliseconds(7200000);
  expected_catalog.campaigns.push_back(BuildCatalogCampaign1());
  expected_catalog.campaigns.push_back(BuildCatalogCampaign2());

  EXPECT_EQ(expected_catalog, *catalog);
}

TEST_F(BatAdsCatalogTest, ParseEmptyCatalog) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathAndParseTagsToString(kEmptyCatalog);
  ASSERT_TRUE(json);

  // Act
  const absl::optional<CatalogInfo> catalog = JSONReader::ReadCatalog(*json);
  ASSERT_TRUE(catalog);

  // Assert
  CatalogInfo expected_catalog;

  EXPECT_EQ(expected_catalog, *catalog);
}

TEST_F(BatAdsCatalogTest, InvalidCatalog) {
  // Arrange

  // Act
  const absl::optional<CatalogInfo> catalog =
      JSONReader::ReadCatalog(kInvalidCatalog);

  // Assert
  EXPECT_FALSE(catalog);
}

}  // namespace ads
