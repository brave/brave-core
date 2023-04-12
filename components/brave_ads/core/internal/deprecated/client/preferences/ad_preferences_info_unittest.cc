/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/ad_preferences_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

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

void ParseJsonAndCompareWithSampleAdPreferencesInfo(const std::string& json) {
  // Arrange
  AdPreferencesInfo ad_preferences;

  // Act
  EXPECT_TRUE(ad_preferences.FromJson(json));

  // Assert
  ASSERT_EQ(1U, ad_preferences.filtered_advertisers.size());
  EXPECT_EQ("filtered_advertiser_id",
            ad_preferences.filtered_advertisers[0].id);
  ASSERT_EQ(1U, ad_preferences.filtered_categories.size());
  EXPECT_EQ("filtered_category_name",
            ad_preferences.filtered_categories[0].name);
  ASSERT_EQ(1U, ad_preferences.saved_ads.size());
  EXPECT_EQ("creative_instance_id",
            ad_preferences.saved_ads[0].creative_instance_id);
  ASSERT_EQ(1U, ad_preferences.flagged_ads.size());
  EXPECT_EQ("creative_set_id", ad_preferences.flagged_ads[0].creative_set_id);
}

}  // namespace

class BatAdsAdPreferencesInfoTest : public UnitTestBase {};

TEST_F(BatAdsAdPreferencesInfoTest, SerializeSampleAdPreferencesInfo) {
  // Arrange
  AdPreferencesInfo ad_preferences;

  FilteredAdvertiserInfo filtered_advertiser;
  filtered_advertiser.id = "filtered_advertiser_id";
  ad_preferences.filtered_advertisers.push_back(filtered_advertiser);

  FilteredCategoryInfo filtered_category;
  filtered_category.name = "filtered_category_name";
  ad_preferences.filtered_categories.push_back(filtered_category);

  SavedAdInfo saved_ad;
  saved_ad.creative_instance_id = "creative_instance_id";
  ad_preferences.saved_ads.push_back(saved_ad);

  FlaggedAdInfo flagged_ad;
  flagged_ad.creative_set_id = "creative_set_id";
  ad_preferences.flagged_ads.push_back(flagged_ad);

  // Act
  const std::string json = ad_preferences.ToJson();

  // Assert
  ParseJsonAndCompareWithSampleAdPreferencesInfo(json);
}

TEST_F(BatAdsAdPreferencesInfoTest, ParseSampleAdPreferencesInfoJson) {
  const AdPreferencesInfo ad_preferences;
  ParseJsonAndCompareWithSampleAdPreferencesInfo(kSampleAdPreferencesInfoJson);
}

TEST_F(BatAdsAdPreferencesInfoTest, ParseEmptyJson) {
  // Arrange
  AdPreferencesInfo ad_preferences;

  // Act
  EXPECT_TRUE(ad_preferences.FromJson("{}"));

  // Assert
  EXPECT_EQ(0U, ad_preferences.filtered_advertisers.size());
  EXPECT_EQ(0U, ad_preferences.filtered_categories.size());
  EXPECT_EQ(0U, ad_preferences.saved_ads.size());
  EXPECT_EQ(0U, ad_preferences.flagged_ads.size());
}

TEST_F(BatAdsAdPreferencesInfoTest, ParsePreferencesWithNotValidMembers) {
  // Arrange
  AdPreferencesInfo ad_preferences;

  // Act & Assert
  // filtered_advertisers
  EXPECT_FALSE(ad_preferences.FromJson(R"({
    {
      "filtered_advertisers": [
        {
          "not_id": "value"
        }
      ]
  })"));
  EXPECT_FALSE(ad_preferences.FromJson(R"({
    {
      "filtered_advertisers": [
        {
          "id": 123
        }
      ]
  })"));

  // filtered_categories
  EXPECT_FALSE(ad_preferences.FromJson(R"({
    {
      "filtered_categories": [
        {
          "not_name": "value"
        }
      ]
  })"));
  EXPECT_FALSE(ad_preferences.FromJson(R"({
    {
      "filtered_categories": [
        {
          "not_name": 123
        }
      ]
  })"));

  // saved_ads
  EXPECT_FALSE(ad_preferences.FromJson(R"({
    {
      "saved_ads": [
        {
          "not_creative_instance_id": "value"
        }
      ]
  })"));
  EXPECT_FALSE(ad_preferences.FromJson(R"({
    {
      "saved_ads": [
        {
          "creative_instance_id": 123
        }
      ]
  })"));

  // flagged_ads
  EXPECT_FALSE(ad_preferences.FromJson(R"({
    {
      "flagged_ads": [
        {
          "not_creative_set_id": "value"
        }
      ]
  })"));
  EXPECT_FALSE(ad_preferences.FromJson(R"({
    {
      "flagged_ads": [
        {
          "creative_set_id": 123
        }
      ]
  })"));
}

}  // namespace brave_ads
