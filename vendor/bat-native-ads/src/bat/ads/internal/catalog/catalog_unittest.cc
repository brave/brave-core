/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const char kEmptyCatalog[] = "empty_catalog.json";

const char kCatalogWithSingleCampaign[] = "catalog_with_single_campaign.json";

const char kCatalogWithMultipleCampaigns[] =
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
    catalog_segment.name = "Technology & Computing";
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

    // Creative Ad Notifications
    CatalogCreativeAdNotificationList catalog_creative_ad_notifications;

    CatalogCreativeAdNotificationInfo catalog_creative_ad_notification;
    catalog_creative_ad_notification.creative_instance_id =
        "87c775ca-919b-4a87-8547-94cf0c3161a2";
    CatalogTypeInfo catalog_ad_notification_type;
    catalog_ad_notification_type.code = "notification_all_v1";
    catalog_ad_notification_type.name = "notification";
    catalog_ad_notification_type.platform = "all";
    catalog_ad_notification_type.version = 1;
    catalog_creative_ad_notification.type = catalog_ad_notification_type;
    catalog_creative_ad_notification.payload.body =
        "Test Ad Notification Campaign 1 Body";
    catalog_creative_ad_notification.payload.title =
        "Test Ad Notification Campaign 1 Title";
    catalog_creative_ad_notification.payload.target_url =
        "https://brave.com/1/ad_notification";
    catalog_creative_ad_notifications.push_back(
        catalog_creative_ad_notification);

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
    catalog_creative_new_tab_page_ad.payload.alt =
        "Test New Tab Page Ad Campaign 1";
    catalog_creative_new_tab_page_ad.payload.target_url =
        "https://brave.com/1/new_tab_page_ad";
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
        "https://brave.com/1/promoted_content_ad";
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
    catalog_creative_promoted_content_ad.type =
        catalog_type_inline_content_ad_type;
    catalog_creative_inline_content_ad.payload.title = "Inline Content 1";
    catalog_creative_inline_content_ad.payload.description =
        "Test Inline Content Ad Campaign 1";
    catalog_creative_inline_content_ad.payload.target_url =
        "https://brave.com/1/inline_content_ad";
    catalog_creative_inline_content_ads.push_back(
        catalog_creative_inline_content_ad);

    // Conversions
    ConversionList conversions;

    ConversionInfo conversion;
    conversion.creative_set_id = "340c927f-696e-4060-9933-3eafc56c3f31";
    conversion.type = "postview";
    conversion.url_pattern = "https://www.brave.com/1/*";
    conversion.observation_window = 30;
    conversion.expiry_timestamp = 4105036799;
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
    catalog_creative_set.segments = catalog_segments;
    catalog_creative_set.oses = catalog_oses;
    catalog_creative_set.creative_ad_notifications =
        catalog_creative_ad_notifications;
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

    // Creative Ad Notifications
    CatalogCreativeAdNotificationList catalog_creative_ad_notifications;

    CatalogCreativeAdNotificationInfo catalog_creative_ad_notification;
    catalog_creative_ad_notification.creative_instance_id =
        "17206fbd-0282-4759-ad28-d5e040ee1ff7";
    CatalogTypeInfo catalog_ad_notification_type;
    catalog_ad_notification_type.code = "notification_all_v1";
    catalog_ad_notification_type.name = "notification";
    catalog_ad_notification_type.platform = "all";
    catalog_ad_notification_type.version = 1;
    catalog_creative_ad_notification.type = catalog_ad_notification_type;
    catalog_creative_ad_notification.payload.body =
        "Test Ad Notification Campaign 2 Body";
    catalog_creative_ad_notification.payload.title =
        "Test Ad Notification Campaign 2 Title";
    catalog_creative_ad_notification.payload.target_url =
        "https://brave.com/2/ad_notification";
    catalog_creative_ad_notifications.push_back(
        catalog_creative_ad_notification);

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
    catalog_creative_new_tab_page_ad.payload.alt =
        "Test New Tab Page Ad Campaign 2";
    catalog_creative_new_tab_page_ad.payload.target_url =
        "https://brave.com/2/new_tab_page_ad";
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
        "https://brave.com/2/promoted_content_ad";
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
    catalog_creative_promoted_content_ad.type =
        catalog_type_inline_content_ad_type;
    catalog_creative_inline_content_ad.payload.title = "Inline Content 2";
    catalog_creative_inline_content_ad.payload.description =
        "Test Inline Content Ad Campaign 2";
    catalog_creative_inline_content_ad.payload.target_url =
        "https://brave.com/2/inline_content_ad";
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
    conversion.expiry_timestamp = 4103049599;
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
    catalog_creative_set.segments = catalog_segments;
    catalog_creative_set.oses = catalog_oses;
    catalog_creative_set.creative_ad_notifications =
        catalog_creative_ad_notifications;
    catalog_creative_set.creative_new_tab_page_ads =
        catalog_creative_new_tab_page_ads;
    catalog_creative_set.creative_promoted_content_ads =
        catalog_creative_promoted_content_ads;
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

TEST_F(BatAdsCatalogTest, ParseCatalog) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  // Act
  Catalog catalog;
  const bool success = catalog.FromJson(json);

  // Assert
  EXPECT_TRUE(success);
}

TEST_F(BatAdsCatalogTest, ParseEmptyCatalog) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kEmptyCatalog);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  // Act
  Catalog catalog;
  const bool success = catalog.FromJson(json);

  // Assert
  EXPECT_TRUE(success);
}

TEST_F(BatAdsCatalogTest, InvalidCatalog) {
  // Arrange

  // Act
  Catalog catalog;
  const bool success = catalog.FromJson("invalid_json");

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(BatAdsCatalogTest, HasChanged) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const bool has_changed =
      catalog.HasChanged("4665197588efd8cc17a55b3c7740a4fecefec2f3");

  // Assert
  EXPECT_TRUE(has_changed);
}

TEST_F(BatAdsCatalogTest, HasNotChanged) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const bool has_changed =
      catalog.HasChanged("29e5c8bc0ba319069980bb390d8e8f9b58c05a20");

  // Assert
  EXPECT_FALSE(has_changed);
}

TEST_F(BatAdsCatalogTest, GetId) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const std::string id = catalog.GetId();

  // Assert
  EXPECT_EQ("29e5c8bc0ba319069980bb390d8e8f9b58c05a20", id);
}

TEST_F(BatAdsCatalogTest, GetVersion) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const int version = catalog.GetVersion();

  // Assert
  EXPECT_EQ(8, version);
}

TEST_F(BatAdsCatalogTest, GetPing) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const int64_t ping = catalog.GetPing();

  // Assert
  EXPECT_EQ(7200, ping);
}

TEST_F(BatAdsCatalogTest, GetIssuers) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const CatalogIssuersInfo catalog_issuers = catalog.GetIssuers();

  // Assert
  CatalogIssuersInfo expected_catalog_issuers;

  expected_catalog_issuers.public_key =
      "qi1Vl8YrPEZliN5wmBgLTuGkbk8K505QwlXLTZjUd34=";

  CatalogIssuerInfo catalog_issuer_1;
  catalog_issuer_1.name = "0.01BAT";
  catalog_issuer_1.public_key = "VihGXGoiQ5Fjxe4SrskIVMcmERa1LoAgvhFxxfLmNEI=";
  expected_catalog_issuers.issuers.push_back(catalog_issuer_1);

  CatalogIssuerInfo catalog_issuer_2;
  catalog_issuer_2.name = "0.05BAT";
  catalog_issuer_2.public_key = "mmXlFlskcF+LjQmJTPQUmoDMV8Co2r+0eNqSyzCywmk=";
  expected_catalog_issuers.issuers.push_back(catalog_issuer_2);

  EXPECT_EQ(expected_catalog_issuers, catalog_issuers);
}

TEST_F(BatAdsCatalogTest, GetCampaign) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithSingleCampaign);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const CatalogCampaignList catalog_campaigns = catalog.GetCampaigns();

  // Assert
  CatalogCampaignList expected_catalog_campaigns;
  CatalogCampaignInfo catalog_campaign = BuildCatalogCampaign1();
  expected_catalog_campaigns.push_back(catalog_campaign);

  EXPECT_EQ(expected_catalog_campaigns, catalog_campaigns);
}

TEST_F(BatAdsCatalogTest, GetCampaigns) {
  // Arrange
  const absl::optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalogWithMultipleCampaigns);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  const CatalogCampaignList catalog_campaigns = catalog.GetCampaigns();

  // Assert
  CatalogCampaignList expected_catalog_campaigns;
  CatalogCampaignInfo catalog_campaign_1 = BuildCatalogCampaign1();
  expected_catalog_campaigns.push_back(catalog_campaign_1);
  CatalogCampaignInfo catalog_campaign_2 = BuildCatalogCampaign2();
  expected_catalog_campaigns.push_back(catalog_campaign_2);

  EXPECT_EQ(expected_catalog_campaigns, catalog_campaigns);
}

}  // namespace ads
