/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_converting_util.h"
#include "brave/components/brave_ads/core/browser/search_result_ad/test_web_page_util.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr const char* kSearchResultAdRequiredAttributes[] = {
    "data-placement-id", "data-creative-instance-id", "data-creative-set-id",
    "data-campaign-id",  "data-advertiser-id",        "data-headline-text",
    "data-description",  "data-landing-page",         "data-rewards-value"};

constexpr const char* kSearchResultAdConversionAttributes[] = {
    "data-conversion-type-value", "data-conversion-url-pattern-value",
    "data-conversion-advertiser-public-key-value",
    "data-conversion-observation-window-value"};

void CheckRequiredAttributes(
    const ads::mojom::SearchResultAdInfoPtr& search_result_ad) {
  EXPECT_EQ(search_result_ad->placement_id, kTestWebPagePlacementId);
  EXPECT_EQ(search_result_ad->creative_instance_id, "value0");
  EXPECT_EQ(search_result_ad->creative_set_id, "value1");
  EXPECT_EQ(search_result_ad->campaign_id, "value2");
  EXPECT_EQ(search_result_ad->advertiser_id, "value3");
  EXPECT_EQ(search_result_ad->headline_text, "value4");
  EXPECT_EQ(search_result_ad->description, "value5");
  EXPECT_EQ(search_result_ad->target_url, GURL("https://brave.com"));
  EXPECT_EQ(search_result_ad->value, 0.5);
}

void CheckConversionAttributes(
    const ads::mojom::SearchResultAdInfoPtr& search_result_ad) {
  EXPECT_EQ(search_result_ad->conversion->type, "value6");
  EXPECT_EQ(search_result_ad->conversion->url_pattern, "value7");
  EXPECT_EQ(search_result_ad->conversion->advertiser_public_key, "value8");
  EXPECT_EQ(
      static_cast<size_t>(search_result_ad->conversion->observation_window),
      1U);
}

}  // namespace

TEST(SearchResultAdConvertingTest, ValidWebPage) {
  std::vector<::schema_org::mojom::EntityPtr> entities =
      CreateTestWebPageEntities();
  const auto search_result_ads =
      ConvertWebPageEntitiesToSearchResultAds(entities);
  EXPECT_EQ(search_result_ads.size(), 1U);
  const ads::mojom::SearchResultAdInfoPtr& search_result_ad =
      search_result_ads.at(kTestWebPagePlacementId);
  ASSERT_TRUE(search_result_ad.get());
  ASSERT_TRUE(search_result_ad->conversion.get());

  CheckRequiredAttributes(search_result_ad);
  CheckConversionAttributes(search_result_ad);
}

TEST(SearchResultAdConvertingTest, NotValidWebPage) {
  {
    std::vector<::schema_org::mojom::EntityPtr> entities;
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities();
    entities[0]->type = "Not-Product";
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities();
    entities[0]->properties.clear();
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities();
    auto& property = entities[0]->properties[0];
    property->name = "not-creatives";
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities();
    auto& property = entities[0]->properties[0];
    property->values = schema_org::mojom::Values::NewEntityValues({});
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities();
    auto& property = entities[0]->properties[0];
    property->values = schema_org::mojom::Values::NewStringValues({"creative"});
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }
}

TEST(SearchResultAdConvertingTest, AdEntityExtraProperty) {
  std::vector<::schema_org::mojom::EntityPtr> entities =
      CreateTestWebPageEntities();
  auto& property = entities[0]->properties[0];
  auto& ad_entity = property->values->get_entity_values()[0];

  schema_org::mojom::PropertyPtr extra_property =
      schema_org::mojom::Property::New();
  extra_property->name = "extra-name";
  extra_property->values =
      schema_org::mojom::Values::NewStringValues({"extra-value"});
  ad_entity->properties.push_back(std::move(extra_property));

  const auto search_result_ads =
      ConvertWebPageEntitiesToSearchResultAds(entities);
  ASSERT_EQ(search_result_ads.size(), 1U);
  const ads::mojom::SearchResultAdInfoPtr& search_result_ad =
      search_result_ads.at(kTestWebPagePlacementId);
  ASSERT_TRUE(search_result_ad.get());
  ASSERT_TRUE(search_result_ad->conversion.get());

  CheckRequiredAttributes(search_result_ad);
  CheckConversionAttributes(search_result_ad);
}

TEST(SearchResultAdConvertingTest, AdEntityPropertySkipped) {
  for (const char* attribute : kSearchResultAdRequiredAttributes) {
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities({attribute});
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  for (const char* attribute : kSearchResultAdConversionAttributes) {
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities({attribute});
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_EQ(search_result_ads.size(), 1U);
    const ads::mojom::SearchResultAdInfoPtr& search_result_ad =
        search_result_ads.at(kTestWebPagePlacementId);
    ASSERT_TRUE(search_result_ad.get());

    CheckRequiredAttributes(search_result_ad);
    EXPECT_FALSE(search_result_ad->conversion);
  }
}

TEST(SearchResultAdConvertingTest, NotValidAdEntityWrongPropertyType) {
  {
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities();
    auto& property = entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];
    ad_entity->type = "Not-SearchResultAd";
    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    // Change type of "data-landing-page".
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities({"data-landing-page"});
    auto& property = entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    // Add an URL with http scheme.
    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "data-landing-page";
    extra_property->values =
        schema_org::mojom::Values::NewStringValues({"http://brave.com"});
    ad_entity->properties.push_back(std::move(extra_property));

    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    // Change type of "data-rewards-value".
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities({"data-rewards-value"});
    auto& property = entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "data-rewards-value";
    extra_property->values =
        schema_org::mojom::Values::NewStringValues({"0-5"});
    ad_entity->properties.push_back(std::move(extra_property));

    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    // Change type of "data-conversion-observation-window-value".
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities({"data-conversion-observation-window-value"});
    auto& property = entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "data-conversion-observation-window-value";
    extra_property->values = schema_org::mojom::Values::NewStringValues({"1"});
    ad_entity->properties.push_back(std::move(extra_property));

    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }

  {
    // Change type of "data-creative-instance-id".
    std::vector<::schema_org::mojom::EntityPtr> entities =
        CreateTestWebPageEntities({"data-creative-instance-id"});
    auto& property = entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "data-creative-instance-id";
    extra_property->values = schema_org::mojom::Values::NewLongValues({101});
    ad_entity->properties.push_back(std::move(extra_property));

    const auto search_result_ads =
        ConvertWebPageEntitiesToSearchResultAds(entities);
    EXPECT_TRUE(search_result_ads.empty());
  }
}

}  // namespace brave_ads
