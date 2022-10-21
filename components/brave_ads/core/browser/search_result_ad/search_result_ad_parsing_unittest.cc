/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_parsing.h"
#include "brave/components/brave_ads/core/browser/search_result_ad/test_web_page_util.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom.h"

namespace brave_ads {

TEST(SearchResultAdParsingTest, ValidWebPage) {
  blink::mojom::WebPagePtr web_page = CreateTestWebPage();
  const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
  EXPECT_EQ(ads.size(), 1u);
  const ads::mojom::SearchResultAdInfoPtr& search_result_ad =
      ads.at(kCreativeInstanceId);
  ASSERT_TRUE(search_result_ad.get());

  EXPECT_EQ(search_result_ad->target_url, GURL(kTargetUrl));
  EXPECT_EQ(search_result_ad->value, 0.5);
  EXPECT_EQ(
      static_cast<size_t>(search_result_ad->conversion->observation_window),
      1U);

  EXPECT_EQ(search_result_ad->creative_instance_id, kCreativeInstanceId);
  EXPECT_EQ(search_result_ad->placement_id, "value0");
  EXPECT_EQ(search_result_ad->creative_set_id, "value1");
  EXPECT_EQ(search_result_ad->campaign_id, "value2");
  EXPECT_EQ(search_result_ad->advertiser_id, "value3");
  EXPECT_EQ(search_result_ad->headline_text, "value4");
  EXPECT_EQ(search_result_ad->description, "value5");
  EXPECT_EQ(search_result_ad->conversion->type, "value6");
  EXPECT_EQ(search_result_ad->conversion->url_pattern, "value7");
  EXPECT_EQ(search_result_ad->conversion->advertiser_public_key, "value8");
}

TEST(SearchResultAdParsingTest, NotValidWebPage) {
  {
    blink::mojom::WebPagePtr web_page = blink::mojom::WebPage::New();
    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    blink::mojom::WebPagePtr web_page = CreateTestWebPage();
    web_page->entities[0]->type = "Not-Product";
    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    blink::mojom::WebPagePtr web_page = CreateTestWebPage();
    web_page->entities[0]->properties.clear();
    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    blink::mojom::WebPagePtr web_page = CreateTestWebPage();
    auto& property = web_page->entities[0]->properties[0];
    property->name = "not-creatives";
    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    blink::mojom::WebPagePtr web_page = CreateTestWebPage();
    auto& property = web_page->entities[0]->properties[0];
    property->values = schema_org::mojom::Values::NewEntityValues({});
    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    blink::mojom::WebPagePtr web_page = CreateTestWebPage();
    auto& property = web_page->entities[0]->properties[0];
    property->values = schema_org::mojom::Values::NewStringValues({"creative"});
    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }
}

TEST(SearchResultAdParsingTest, NotValidAdEntityExtraProperty) {
  {
    blink::mojom::WebPagePtr web_page = CreateTestWebPage();
    auto& property = web_page->entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];
    ad_entity->type = "Not-SearchResultAd";
    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    blink::mojom::WebPagePtr web_page = CreateTestWebPage();
    auto& property = web_page->entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "extra-name";
    extra_property->values =
        schema_org::mojom::Values::NewStringValues({"extra-value"});
    ad_entity->properties.push_back(std::move(extra_property));

    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }
}

TEST(SearchResultAdParsingTest, NotValidAdEntityPropertySkipped) {
  constexpr int kSearchResultAdAttributesCount = 12;
  for (int index = 0; index < kSearchResultAdAttributesCount; ++index) {
    blink::mojom::WebPagePtr web_page = CreateTestWebPage(index);
    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }
}

TEST(SearchResultAdParsingTest, NotValidAdEntityWrongPropertyType) {
  {
    // Skip "data-landing-page".
    blink::mojom::WebPagePtr web_page = CreateTestWebPage(0);
    auto& property = web_page->entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "data-landing-page";
    extra_property->values =
        schema_org::mojom::Values::NewStringValues({kTargetUrl});
    ad_entity->properties.push_back(std::move(extra_property));

    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    // Skip "data-rewards-value".
    blink::mojom::WebPagePtr web_page = CreateTestWebPage(1);
    auto& property = web_page->entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "data-rewards-value";
    extra_property->values =
        schema_org::mojom::Values::NewStringValues({"0-5"});
    ad_entity->properties.push_back(std::move(extra_property));

    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    // Skip "data-conversion-observation-window-value".
    blink::mojom::WebPagePtr web_page = CreateTestWebPage(2);
    auto& property = web_page->entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "data-conversion-observation-window-value";
    extra_property->values = schema_org::mojom::Values::NewStringValues({"1"});
    ad_entity->properties.push_back(std::move(extra_property));

    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }

  {
    // Skip "data-creative-instance-id".
    blink::mojom::WebPagePtr web_page = CreateTestWebPage(3);
    auto& property = web_page->entities[0]->properties[0];
    auto& ad_entity = property->values->get_entity_values()[0];

    schema_org::mojom::PropertyPtr extra_property =
        schema_org::mojom::Property::New();
    extra_property->name = "data-creative-instance-id";
    extra_property->values = schema_org::mojom::Values::NewLongValues({101});
    ad_entity->properties.push_back(std::move(extra_property));

    const SearchResultAdMap ads = ParseWebPageEntities(std::move(web_page));
    EXPECT_TRUE(ads.empty());
  }
}

}  // namespace brave_ads
