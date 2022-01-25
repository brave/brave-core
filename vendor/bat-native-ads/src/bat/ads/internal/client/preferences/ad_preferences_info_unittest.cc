/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/preferences/ad_preferences_info.h"
#include "bat/ads/internal/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAdsAdPreferencesInfoTest.*

namespace ads {

namespace {

constexpr char kSampleAdPreferencesInfoJson[] = R"(
{
  "filtered_advertisers": [
    {
      "id": "filtered_advertiser_id"
    }
  ],
  "filtered_categories": [
    {
      "name": "filtered_category_name"
    }
  ],
  "saved_ads": [
    {
      "creative_instance_id": "creative_instance_id"
    }
  ],
  "flagged_ads": [
    {
      "creative_set_id": "creative_set_id"
    }
  ]
})";

}  // namespace

class BatAdsAdPreferencesInfoTest : public UnitTestBase {
 public:
  BatAdsAdPreferencesInfoTest() {}
  ~BatAdsAdPreferencesInfoTest() override {}

  void ParseJsonAndCompareWithSampleAdPreferencesInfo(const std::string& json) {
    // Arrange
    AdPreferencesInfo ad_preferences_info;

    // Act
    EXPECT_TRUE(ad_preferences_info.FromJson(json));

    // Assert
    ASSERT_EQ(1u, ad_preferences_info.filtered_advertisers.size());
    EXPECT_EQ("filtered_advertiser_id",
              ad_preferences_info.filtered_advertisers[0].id);
    ASSERT_EQ(1u, ad_preferences_info.filtered_categories.size());
    EXPECT_EQ("filtered_category_name",
              ad_preferences_info.filtered_categories[0].name);
    ASSERT_EQ(1u, ad_preferences_info.saved_ads.size());
    EXPECT_EQ("creative_instance_id",
              ad_preferences_info.saved_ads[0].creative_instance_id);
    ASSERT_EQ(1u, ad_preferences_info.flagged_ads.size());
    EXPECT_EQ("creative_set_id",
              ad_preferences_info.flagged_ads[0].creative_set_id);
  }
};

TEST_F(BatAdsAdPreferencesInfoTest, SerializeSampleAdPreferencesInfo) {
  // Arrange
  AdPreferencesInfo ad_preferences_info;

  FilteredAdvertiserInfo filtered_advertiser;
  filtered_advertiser.id = "filtered_advertiser_id";
  ad_preferences_info.filtered_advertisers.push_back(filtered_advertiser);

  FilteredCategoryInfo filtered_category;
  filtered_category.name = "filtered_category_name";
  ad_preferences_info.filtered_categories.push_back(filtered_category);

  SavedAdInfo saved_ad;
  saved_ad.creative_instance_id = "creative_instance_id";
  ad_preferences_info.saved_ads.push_back(saved_ad);

  FlaggedAdInfo flagged_ad;
  flagged_ad.creative_set_id = "creative_set_id";
  ad_preferences_info.flagged_ads.push_back(flagged_ad);

  // Act
  const std::string json = ad_preferences_info.ToJson();

  // Assert
  ParseJsonAndCompareWithSampleAdPreferencesInfo(json);
}

TEST_F(BatAdsAdPreferencesInfoTest, ParseSampleAdPreferencesInfoJson) {
  AdPreferencesInfo ad_preferences_info;
  ParseJsonAndCompareWithSampleAdPreferencesInfo(kSampleAdPreferencesInfoJson);
}

TEST_F(BatAdsAdPreferencesInfoTest, ParseEmptyJson) {
  // Arrange
  AdPreferencesInfo ad_preferences_info;

  // Act
  EXPECT_TRUE(ad_preferences_info.FromJson("{}"));

  // Assert
  EXPECT_EQ(0u, ad_preferences_info.filtered_advertisers.size());
  EXPECT_EQ(0u, ad_preferences_info.filtered_categories.size());
  EXPECT_EQ(0u, ad_preferences_info.saved_ads.size());
  EXPECT_EQ(0u, ad_preferences_info.flagged_ads.size());
}

TEST_F(BatAdsAdPreferencesInfoTest, ParsePreferencesWithNotValidMembers) {
  // Arrange
  AdPreferencesInfo ad_preferences_info;

  // Act & Assert
  // filtered_advertisers
  EXPECT_FALSE(ad_preferences_info.FromJson(R"({
    {
      "filtered_advertisers": [
        {
          "not_id": "value"
        }
      ]
  })"));
  EXPECT_FALSE(ad_preferences_info.FromJson(R"({
    {
      "filtered_advertisers": [
        {
          "id": 123
        }
      ]
  })"));

  // filtered_categories
  EXPECT_FALSE(ad_preferences_info.FromJson(R"({
    {
      "filtered_categories": [
        {
          "not_name": "value"
        }
      ]
  })"));
  EXPECT_FALSE(ad_preferences_info.FromJson(R"({
    {
      "filtered_categories": [
        {
          "not_name": 123
        }
      ]
  })"));

  // saved_ads
  EXPECT_FALSE(ad_preferences_info.FromJson(R"({
    {
      "saved_ads": [
        {
          "not_creative_instance_id": "value"
        }
      ]
  })"));
  EXPECT_FALSE(ad_preferences_info.FromJson(R"({
    {
      "saved_ads": [
        {
          "creative_instance_id": 123
        }
      ]
  })"));

  // flagged_ads
  EXPECT_FALSE(ad_preferences_info.FromJson(R"({
    {
      "flagged_ads": [
        {
          "not_creative_set_id": "value"
        }
      ]
  })"));
  EXPECT_FALSE(ad_preferences_info.FromJson(R"({
    {
      "flagged_ads": [
        {
          "creative_set_id": 123
        }
      ]
  })"));
}

}  // namespace ads
