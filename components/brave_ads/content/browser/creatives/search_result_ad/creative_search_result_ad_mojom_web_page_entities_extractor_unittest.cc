/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_extractor.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_constants.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_test_constants.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/test/creative_search_result_ad_mojom_test_util.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/test/creative_search_result_ad_mojom_web_page_entities_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "components/schema_org/common/metadata.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void VerifyRequiredMojomCreativeAdExpectations(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad) {
  EXPECT_THAT(
      *mojom_creative_ad,
      ::testing::FieldsAre(
          mojom::AdType::kSearchResultAd, test::kCreativeAdPlacementId,
          test::kCreativeAdCreativeInstanceId, test::kCreativeAdCreativeSetId,
          test::kCreativeAdCampaignId, test::kCreativeAdAdvertiserId,
          GURL(test::kCreativeAdLandingPage), test::kCreativeAdHeadlineText,
          test::kCreativeAdDescription,
          ::testing::DoubleEq(test::kCreativeAdRewardsValue),
          /*campaign_conversion=*/::testing::_));
}

// Builds the default fixture, then nests an `order` sub-object under
// `data-conversion` with the given extraction flags and checkout pattern,
// matching the shape `ExtractCampaignConversionMojomProperty` expects.
std::vector<schema_org::mojom::EntityPtr> MojomWebPageEntitiesWithOrder(
    bool should_extract_id,
    bool should_extract_value,
    std::optional<std::string_view> checkout_url_pattern) {
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});

  auto& mojom_ad_entity = mojom_web_page_entities.front()
                              ->properties.front()
                              ->values->get_entity_values()
                              .front();
  auto* const mojom_data_property = test::FindMojomPropertyByName(
      mojom_ad_entity, kCampaignConversionDataPropertyName);
  CHECK(mojom_data_property);
  auto& mojom_data_entity =
      (*mojom_data_property)->values->get_entity_values().front();

  schema_org::mojom::EntityPtr mojom_order_entity =
      schema_org::mojom::Entity::New();

  schema_org::mojom::PropertyPtr mojom_extract_id_property =
      schema_org::mojom::Property::New();
  mojom_extract_id_property->name =
      kCampaignConversionDataOrderExtractIdPropertyName;
  mojom_extract_id_property->values =
      schema_org::mojom::Values::NewBoolValues({should_extract_id});
  mojom_order_entity->properties.push_back(
      std::move(mojom_extract_id_property));

  schema_org::mojom::PropertyPtr mojom_extract_value_property =
      schema_org::mojom::Property::New();
  mojom_extract_value_property->name =
      kCampaignConversionDataOrderExtractValuePropertyName;
  mojom_extract_value_property->values =
      schema_org::mojom::Values::NewBoolValues({should_extract_value});
  mojom_order_entity->properties.push_back(
      std::move(mojom_extract_value_property));

  if (checkout_url_pattern) {
    test::AddMojomProperty<std::string>(
        &mojom_order_entity->properties,
        kCampaignConversionDataOrderCheckoutUrlPatternPropertyName,
        std::string(*checkout_url_pattern));
  }

  std::vector<schema_org::mojom::EntityPtr> mojom_order_entities;
  mojom_order_entities.push_back(std::move(mojom_order_entity));

  schema_org::mojom::PropertyPtr mojom_order_property =
      schema_org::mojom::Property::New();
  mojom_order_property->name = kCampaignConversionDataOrderPropertyName;
  mojom_order_property->values = schema_org::mojom::Values::NewEntityValues(
      std::move(mojom_order_entities));
  mojom_data_entity->properties.push_back(std::move(mojom_order_property));

  return mojom_web_page_entities;
}

void VerifyRequiredMojomCampaignConversionExpectations(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad) {
  EXPECT_THAT(
      mojom_creative_ad->campaign_conversion->thank_you_url_pattern,
      ::testing::Optional(std::string(test::kCampaignConversionUrlPattern)));
  EXPECT_EQ(mojom_creative_ad->campaign_conversion->observation_window,
            test::kCampaignConversionObservationWindow);
}

}  // namespace

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest, Extract) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.front();
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->campaign_conversion);

  VerifyRequiredMojomCreativeAdExpectations(mojom_creative_ad);
  VerifyRequiredMojomCampaignConversionExpectations(mojom_creative_ad);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractIfEntityTypeIsUnsupported) {
  // Invalid type.
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});

  const auto& mojom_entity = mojom_web_page_entities.front()
                                 ->properties.front()
                                 ->values->get_entity_values()
                                 .front();
  mojom_entity->type = "unsupported";

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractForEmptyMojomWebPageEntities) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities;

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractForUnsupportedWebPageEntityType) {
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});
  mojom_web_page_entities.front()->type = "unsupported";

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractForEmptyWebPageEntityProperties) {
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});
  mojom_web_page_entities.front()->properties.clear();

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractForUnsupportedMojomPropertyName) {
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});
  auto& mojom_property = mojom_web_page_entities.front()->properties.front();
  mojom_property->name = "unsupported";

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractForEmptyMojomPropertyEntityValues) {
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});
  auto& mojom_property = mojom_web_page_entities.front()->properties.front();
  mojom_property->values = schema_org::mojom::Values::NewEntityValues({});

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractForUnsupportedMojomPropertyValueType) {
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});
  auto& mojom_property = mojom_web_page_entities.front()->properties.front();
  mojom_property->values =
      schema_org::mojom::Values::NewStringValues({"unsupported"});

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractForMissingPlacementId) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{"data-placement-id"});

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     ExtractWhenDataPlacementIdIncludeUnreservedCharacters) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntitiesWithProperty(
          kCreativeAdPlacementIdPropertyName,
          test::kCreativeAdPlacementIdWithUnreservedCharacters);

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.front();
  ASSERT_TRUE(mojom_creative_ad);
  EXPECT_EQ(test::kEscapedCreativeAdPlacementIdWithUnreservedCharacters,
            mojom_creative_ad->placement_id);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     ExtractWhenIncludesUnknownProperties) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntitiesWithProperty(
          /*name=*/"foo",
          /*value=*/"bar");

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));

  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.front();
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->campaign_conversion);

  VerifyRequiredMojomCreativeAdExpectations(mojom_creative_ad);
  VerifyRequiredMojomCampaignConversionExpectations(mojom_creative_ad);
}

void VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
    std::string_view property_name) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{property_name});

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingPlacementId) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdPlacementIdPropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingCreativeInstanceId) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdCreativeInstanceIdPropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingCreativeSetId) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdCreativeSetIdPropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingCampaignId) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdCampaignIdPropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingAdvertiserId) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdAdvertiserIdPropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingHeadlineText) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdHeadlineTextPropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingDescription) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdDescriptionPropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingLandingPage) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdLandingPagePropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingRewardsValue) {
  VerifyDoesNotExtractIfMissingRequiredCreativeAdProperty(
      kCreativeAdRewardsValuePropertyName);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractInvalidCreativeAdProperties) {
  {
    // Invalid "data-landing-page".
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntitiesWithProperty(
            kCreativeAdLandingPagePropertyName, /*value=*/"http://brave.com");

    const CreativeSearchResultAdList creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    // Invalid "data-rewards-value".
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntitiesWithProperty(
            kCreativeAdRewardsValuePropertyName, /*value=*/"0-5");

    const CreativeSearchResultAdList creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    // Invalid "observationWindow".
    std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{
                kCampaignConversionDataObservationWindowPropertyName});

    auto& mojom_ad_entity = mojom_web_page_entities.front()
                                ->properties.front()
                                ->values->get_entity_values()
                                .front();
    auto* const mojom_data_property = test::FindMojomPropertyByName(
        mojom_ad_entity, kCampaignConversionDataPropertyName);
    ASSERT_TRUE(mojom_data_property);
    auto& mojom_data_entity =
        (*mojom_data_property)->values->get_entity_values().front();
    test::AddMojomProperty<std::string>(
        &mojom_data_entity->properties,
        kCampaignConversionDataObservationWindowPropertyName, /*value=*/"1");

    const CreativeSearchResultAdList creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    // Invalid "data-creative-instance-id".
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{
                kCreativeAdCreativeInstanceIdPropertyName});
    const auto& mojom_property =
        mojom_web_page_entities.front()->properties.front();
    auto& mojom_entity = mojom_property->values->get_entity_values().front();
    test::AddMojomProperty<int64_t>(&mojom_entity->properties,
                                    kCreativeAdCreativeInstanceIdPropertyName,
                                    /*value=*/101);

    const CreativeSearchResultAdList creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotAttachCampaignConversionIfUrlPatternIsEmpty) {
  // Arrange
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{
              kCampaignConversionDataUrlPatternPropertyName});

  auto& mojom_ad_entity = mojom_web_page_entities.front()
                              ->properties.front()
                              ->values->get_entity_values()
                              .front();
  auto* const mojom_data_property = test::FindMojomPropertyByName(
      mojom_ad_entity, kCampaignConversionDataPropertyName);
  ASSERT_TRUE(mojom_data_property);
  auto& mojom_data_entity =
      (*mojom_data_property)->values->get_entity_values().front();
  test::AddMojomProperty<std::string>(
      &mojom_data_entity->properties,
      kCampaignConversionDataUrlPatternPropertyName, /*value=*/"");

  // Act
  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);

  // Assert
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.front();
  ASSERT_TRUE(mojom_creative_ad);
  EXPECT_FALSE(mojom_creative_ad->campaign_conversion);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractIfMissingRequiredCampaignConversionProperties) {
  for (std::string_view property_name :
       {kCampaignConversionDataUrlPatternPropertyName,
        kCampaignConversionDataObservationWindowPropertyName}) {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{property_name});

    const CreativeSearchResultAdList creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
        creative_search_result_ads.front();
    ASSERT_TRUE(mojom_creative_ad);

    VerifyRequiredMojomCreativeAdExpectations(mojom_creative_ad);
    EXPECT_FALSE(mojom_creative_ad->campaign_conversion);
  }
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     ExtractsOrderExtractionFlagsIndependently) {
  static constexpr struct {
    std::string_view name;
    bool should_extract_id;
    bool should_extract_value;
  } kTestCases[] = {
      {"NeitherFlag", /*should_extract_id=*/false,
       /*should_extract_value=*/false},
      {"IdOnly", /*should_extract_id=*/true, /*should_extract_value=*/false},
      {"ValueOnly", /*should_extract_id=*/false,
       /*should_extract_value=*/true},
      {"BothFlags", /*should_extract_id=*/true,
       /*should_extract_value=*/true},
  };

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(test_case.name);

    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        MojomWebPageEntitiesWithOrder(test_case.should_extract_id,
                                      test_case.should_extract_value,
                                      /*checkout_url_pattern=*/std::nullopt);

    const CreativeSearchResultAdList creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
        creative_search_result_ads.front();
    ASSERT_TRUE(mojom_creative_ad);
    ASSERT_TRUE(mojom_creative_ad->campaign_conversion);

    EXPECT_EQ(test_case.should_extract_id,
              mojom_creative_ad->campaign_conversion->should_extract_id);
    EXPECT_EQ(test_case.should_extract_value,
              mojom_creative_ad->campaign_conversion->should_extract_value);
  }
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     ExtractsCheckoutUrlPattern) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      MojomWebPageEntitiesWithOrder(
          /*should_extract_id=*/true, /*should_extract_value=*/false,
          test::kCampaignConversionCheckoutUrlPattern);

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.front();
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->campaign_conversion);

  EXPECT_THAT(mojom_creative_ad->campaign_conversion->checkout_url_pattern,
              ::testing::Optional(
                  std::string(test::kCampaignConversionCheckoutUrlPattern)));
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotAttachCheckoutUrlPatternIfEmpty) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      MojomWebPageEntitiesWithOrder(/*should_extract_id=*/true,
                                    /*should_extract_value=*/false,
                                    /*checkout_url_pattern=*/"");

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.front();
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->campaign_conversion);

  EXPECT_FALSE(mojom_creative_ad->campaign_conversion->checkout_url_pattern);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoesNotExtractOrderFlagsIfOrderPropertyIsMalformed) {
  // The `order` property holds a string instead of a nested entity.
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});

  auto& mojom_ad_entity = mojom_web_page_entities.front()
                              ->properties.front()
                              ->values->get_entity_values()
                              .front();
  auto* const mojom_data_property = test::FindMojomPropertyByName(
      mojom_ad_entity, kCampaignConversionDataPropertyName);
  ASSERT_TRUE(mojom_data_property);
  auto& mojom_data_entity =
      (*mojom_data_property)->values->get_entity_values().front();
  test::AddMojomProperty<std::string>(&mojom_data_entity->properties,
                                      kCampaignConversionDataOrderPropertyName,
                                      /*value=*/"malformed");

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.front();
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->campaign_conversion);

  EXPECT_FALSE(mojom_creative_ad->campaign_conversion->should_extract_id);
  EXPECT_FALSE(mojom_creative_ad->campaign_conversion->should_extract_value);
  EXPECT_FALSE(mojom_creative_ad->campaign_conversion->checkout_url_pattern);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DefaultsOrderFlagIfPropertyTypeIsMalformed) {
  // `extractId` holds a string instead of a bool.
  std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      MojomWebPageEntitiesWithOrder(/*should_extract_id=*/true,
                                    /*should_extract_value=*/false,
                                    /*checkout_url_pattern=*/std::nullopt);

  auto& mojom_ad_entity = mojom_web_page_entities.front()
                              ->properties.front()
                              ->values->get_entity_values()
                              .front();
  auto* const mojom_data_property = test::FindMojomPropertyByName(
      mojom_ad_entity, kCampaignConversionDataPropertyName);
  ASSERT_TRUE(mojom_data_property);
  auto& mojom_data_entity =
      (*mojom_data_property)->values->get_entity_values().front();
  auto* const mojom_order_property = test::FindMojomPropertyByName(
      mojom_data_entity, kCampaignConversionDataOrderPropertyName);
  ASSERT_TRUE(mojom_order_property);
  auto& mojom_order_entity =
      (*mojom_order_property)->values->get_entity_values().front();
  auto& mojom_properties = mojom_order_entity->properties;
  std::erase_if(mojom_properties, [](const auto& mojom_property) {
    return mojom_property->name ==
           kCampaignConversionDataOrderExtractIdPropertyName;
  });
  test::AddMojomProperty<std::string>(
      &mojom_order_entity->properties,
      kCampaignConversionDataOrderExtractIdPropertyName, /*value=*/"true");

  const CreativeSearchResultAdList creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.front();
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->campaign_conversion);

  EXPECT_FALSE(mojom_creative_ad->campaign_conversion->should_extract_id);
}

}  // namespace brave_ads
