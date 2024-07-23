/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/ad_preferences_info.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kAdPreferencesAsJson[] = R"(
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
  ASSERT_TRUE(ad_preferences.FromJson(json));

  // Assert
  ASSERT_THAT(ad_preferences.filtered_advertisers, ::testing::SizeIs(1));
  EXPECT_EQ("filtered_advertiser_id",
            ad_preferences.filtered_advertisers[0].id);

  ASSERT_THAT(ad_preferences.filtered_categories, ::testing::SizeIs(1));
  EXPECT_EQ("filtered_category_name",
            ad_preferences.filtered_categories[0].name);

  ASSERT_THAT(ad_preferences.saved_ads, ::testing::SizeIs(1));
  EXPECT_EQ("creative_instance_id",
            ad_preferences.saved_ads[0].creative_instance_id);

  ASSERT_THAT(ad_preferences.flagged_ads, ::testing::SizeIs(1));
  EXPECT_EQ("creative_set_id", ad_preferences.flagged_ads[0].creative_set_id);
}

}  // namespace

class BraveAdsAdPreferencesInfoTest : public test::TestBase {};

TEST_F(BraveAdsAdPreferencesInfoTest, SerializeSampleAdPreferencesInfo) {
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

TEST_F(BraveAdsAdPreferencesInfoTest, ParseSampleAdPreferencesInfoJson) {
  // Act & Assert
  ParseJsonAndCompareWithSampleAdPreferencesInfo(kAdPreferencesAsJson);
}

TEST_F(BraveAdsAdPreferencesInfoTest, ParseEmptyJson) {
  // Arrange
  AdPreferencesInfo ad_preferences;

  // Act
  ASSERT_TRUE(ad_preferences.FromJson("{}"));

  // Assert
  EXPECT_THAT(ad_preferences, ::testing::FieldsAre(
                                  /*filtered_advertisers*/ ::testing::IsEmpty(),
                                  /*filtered_categories*/ ::testing::IsEmpty(),
                                  /*saved_ads*/ ::testing::IsEmpty(),
                                  /*flagged_ads*/ ::testing::IsEmpty()));
}

TEST_F(BraveAdsAdPreferencesInfoTest, ParsePreferencesWithNotValidMembers) {
  // Arrange
  AdPreferencesInfo ad_preferences;

  // Act & Assert
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
