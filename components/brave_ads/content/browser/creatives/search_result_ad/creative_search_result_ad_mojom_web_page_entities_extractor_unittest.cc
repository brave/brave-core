/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_extractor.h"

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_constants.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_test_util.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_test_util.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_test_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "components/schema_org/common/metadata.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

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
          /*creative_set_conversion*/ ::testing::_));
}

void VerifyRequiredMojomCreativeSetConversionExpectations(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad) {
  EXPECT_EQ(mojom_creative_ad->creative_set_conversion->url_pattern,
            test::kCreativeSetConversionUrlPattern);
  EXPECT_EQ(mojom_creative_ad->creative_set_conversion->observation_window,
            test::kCreativeSetConversionObservationWindow);
}

void VerifyOptionalMojomCreativeSetConversionExpectations(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad) {
  EXPECT_EQ(mojom_creative_ad->creative_set_conversion
                ->verifiable_advertiser_public_key_base64,
            test::kCreativeSetConversionAdvertiserPublicKey);
}

}  // namespace

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest, Extract) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});

  const CreativeSearchResultAdMap creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.at(test::kCreativeAdPlacementId);
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->creative_set_conversion);

  VerifyRequiredMojomCreativeAdExpectations(mojom_creative_ad);
  VerifyRequiredMojomCreativeSetConversionExpectations(mojom_creative_ad);
  VerifyOptionalMojomCreativeSetConversionExpectations(mojom_creative_ad);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractIfEntityTypeIsUnsupported) {
  // Invalid type.
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{});

  const auto& mojom_property = mojom_web_page_entities[0]->properties[0];
  const auto& mojom_entity = mojom_property->values->get_entity_values()[0];
  mojom_entity->type = "unsupported";

  const CreativeSearchResultAdMap creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractForEmptyMojomWebPageEntities) {
  {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities;

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{});
    mojom_web_page_entities[0]->type = "unsupported";

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{});
    mojom_web_page_entities[0]->properties.clear();

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{});
    const auto& mojom_property = mojom_web_page_entities[0]->properties[0];
    mojom_property->name = "unsupported";

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{});
    const auto& mojom_property = mojom_web_page_entities[0]->properties[0];
    mojom_property->values = schema_org::mojom::Values::NewEntityValues({});

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{});
    const auto& mojom_property = mojom_web_page_entities[0]->properties[0];
    mojom_property->values =
        schema_org::mojom::Values::NewStringValues({"unsupported"});

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractForMissingPlacementId) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntities(
          /*excluded_property_names=*/{"data-placement-id"});

  const CreativeSearchResultAdMap creative_search_result_ads =
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

  const CreativeSearchResultAdMap creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.at(
          test::kEscapedCreativeAdPlacementIdWithUnreservedCharacters);
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

  const CreativeSearchResultAdMap creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));

  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.at(test::kCreativeAdPlacementId);
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->creative_set_conversion);

  VerifyRequiredMojomCreativeAdExpectations(mojom_creative_ad);
  VerifyRequiredMojomCreativeSetConversionExpectations(mojom_creative_ad);
  VerifyOptionalMojomCreativeSetConversionExpectations(mojom_creative_ad);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractIfMissingRequiredCreativeAdProperties) {
  static constexpr const char* const kRequiredCreativeAdPropertyNames[] = {
      kCreativeAdPlacementIdPropertyName,
      kCreativeAdCreativeInstanceIdPropertyName,
      kCreativeAdCreativeSetIdPropertyName,
      kCreativeAdCampaignIdPropertyName,
      kCreativeAdAdvertiserIdPropertyName,
      kCreativeAdHeadlineTextPropertyName,
      kCreativeAdDescriptionPropertyName,
      kCreativeAdLandingPagePropertyName,
      kCreativeAdRewardsValuePropertyName};

  for (const char* const property_name : kRequiredCreativeAdPropertyNames) {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{property_name});

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractInvalidCreativeAdProperties) {
  {
    // Invalid "data-landing-page".
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntitiesWithProperty(
            kCreativeAdLandingPagePropertyName, /*value=*/"http://brave.com");

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    // Invalid "data-rewards-value".
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntitiesWithProperty(
            kCreativeAdRewardsValuePropertyName, /*value=*/"0-5");

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }

  {
    // Invalid "data-conversion-observation-window-value".
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntitiesWithProperty(
            kCreativeSetConversionObservationWindowPropertyName, /*value=*/"1");

    const CreativeSearchResultAdMap creative_search_result_ads =
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
    const auto& mojom_property = mojom_web_page_entities[0]->properties[0];
    auto& mojom_entity = mojom_property->values->get_entity_values()[0];
    test::AddMojomProperty<int64_t>(&mojom_entity->properties,
                                    kCreativeAdCreativeInstanceIdPropertyName,
                                    /*value=*/101);

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    EXPECT_THAT(creative_search_result_ads, ::testing::IsEmpty());
  }
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     ExtractIfMissingOptionalCreativeSetConversionAdvertiserPublicKey) {
  const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
      test::CreativeSearchResultAdMojomWebPageEntitiesWithProperty(
          kCreativeSetConversionAdvertiserPublicKeyPropertyName, /*value=*/"");

  const CreativeSearchResultAdMap creative_search_result_ads =
      ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
          mojom_web_page_entities);
  ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
  const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
      creative_search_result_ads.at(test::kCreativeAdPlacementId);
  ASSERT_TRUE(mojom_creative_ad);
  ASSERT_TRUE(mojom_creative_ad->creative_set_conversion);

  VerifyRequiredMojomCreativeAdExpectations(mojom_creative_ad);
  VerifyRequiredMojomCreativeSetConversionExpectations(mojom_creative_ad);
  EXPECT_FALSE(mojom_creative_ad->creative_set_conversion
                   ->verifiable_advertiser_public_key_base64);
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     DoNotExtractIfMissingRequiredCreativeSetConversionProperties) {
  static constexpr const char* const
      kRequiredCreativeSetConversionPropertyNames[] = {
          kCreativeSetConversionUrlPatternPropertyName,
          kCreativeSetConversionObservationWindowPropertyName};

  for (const char* const property_name :
       kRequiredCreativeSetConversionPropertyNames) {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{property_name});

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
        creative_search_result_ads.at(test::kCreativeAdPlacementId);
    ASSERT_TRUE(mojom_creative_ad);

    VerifyRequiredMojomCreativeAdExpectations(mojom_creative_ad);
    EXPECT_FALSE(mojom_creative_ad->creative_set_conversion);
  }
}

TEST(BraveAdsCreativeSearchResultAdMojomWebPageEntitiesExtractorTest,
     ExtractIfMissingOptionalCreativeSetConversionProperties) {
  static constexpr const char* const
      kOptionalCreativeSetConversionPropertyNames[] = {
          kCreativeSetConversionAdvertiserPublicKeyPropertyName};

  for (const char* const property_name :
       kOptionalCreativeSetConversionPropertyNames) {
    const std::vector<schema_org::mojom::EntityPtr> mojom_web_page_entities =
        test::CreativeSearchResultAdMojomWebPageEntities(
            /*excluded_property_names=*/{property_name});

    const CreativeSearchResultAdMap creative_search_result_ads =
        ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
            mojom_web_page_entities);
    ASSERT_THAT(creative_search_result_ads, ::testing::SizeIs(1));
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad =
        creative_search_result_ads.at(test::kCreativeAdPlacementId);
    ASSERT_TRUE(mojom_creative_ad);

    VerifyRequiredMojomCreativeAdExpectations(mojom_creative_ad);
    VerifyRequiredMojomCreativeSetConversionExpectations(mojom_creative_ad);
    EXPECT_FALSE(mojom_creative_ad->creative_set_conversion
                     ->verifiable_advertiser_public_key_base64);
  }
}

}  // namespace brave_ads
