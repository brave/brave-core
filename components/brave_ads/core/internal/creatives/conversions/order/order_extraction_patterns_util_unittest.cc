/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_util.h"

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsOrdersDetectedLanguageFirst) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns.regexes = {
      {"en", "english regex"}, {"de", "german regex"}, {"fr", "french regex"}};

  const std::vector<std::string> site_candidates;

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/"de");

  // Assert
  ASSERT_EQ(3U, order_id_extraction_patterns->regexes.size());
  EXPECT_EQ("german regex", order_id_extraction_patterns->regexes.front());
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsTriesAllLanguagesIfDetectedLanguageIsUnknown) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns.regexes = {{"en", "english regex"},
                                                    {"de", "german regex"}};

  const std::vector<std::string> site_candidates;

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ(2U, order_id_extraction_patterns->regexes.size());
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsSelectsMostSpecificSiteSelectors) {
  // Arrange
  OrderExtractionSelectorInfo checkout_order_extraction_selector;
  checkout_order_extraction_selector.selector = "[data-shopping-cart-id]";
  checkout_order_extraction_selector.attribute_name = "data-shopping-cart-id";

  OrderExtractionSelectorInfo thank_you_order_extraction_selector;
  thank_you_order_extraction_selector.selector = "[data-order-id]";
  thank_you_order_extraction_selector.attribute_name = "data-order-id";

  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns
      .selectors["waldo.example"][OrderExtractionPageType::kCheckout] = {
      checkout_order_extraction_selector};
  component_order_id_extraction_patterns
      .selectors["waldo.example"][OrderExtractionPageType::kThankYou] = {
      thank_you_order_extraction_selector};

  const std::vector<std::string> site_candidates = {"checkout.waldo.example",
                                                    "waldo.example"};

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  ASSERT_EQ(2U, order_id_extraction_patterns->selectors.size());
  EXPECT_EQ(checkout_order_extraction_selector.selector,
            order_id_extraction_patterns->selectors[0]->selector);
  EXPECT_EQ(checkout_order_extraction_selector.attribute_name,
            order_id_extraction_patterns->selectors[0]->attribute_name);
  EXPECT_EQ(thank_you_order_extraction_selector.selector,
            order_id_extraction_patterns->selectors[1]->selector);
  EXPECT_EQ(thank_you_order_extraction_selector.attribute_name,
            order_id_extraction_patterns->selectors[1]->attribute_name);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsResolvesDefaultMaxCombinedTextLength) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns
      .default_max_combined_text_length_to_search = 4096;

  const std::vector<std::string> site_candidates;

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ(4096U,
            order_id_extraction_patterns->max_combined_text_length_to_search);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsResolvesMaxCombinedTextLengthSiteOverride) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns
      .default_max_combined_text_length_to_search = 4096;
  component_order_id_extraction_patterns
      .max_combined_text_length_to_search_overrides["fred.example"] = 16384;

  const std::vector<std::string> site_candidates = {"fred.example"};

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ(16384U,
            order_id_extraction_patterns->max_combined_text_length_to_search);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsResolvesDefaultNumberOfDescendantLevels) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns
      .default_number_of_descendant_levels_to_search = 32;

  const std::vector<std::string> site_candidates;

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ(
      32U, order_id_extraction_patterns->number_of_descendant_levels_to_search);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsResolvesNumberOfDescendantLevelsSiteOverride) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns
      .default_number_of_descendant_levels_to_search = 32;
  component_order_id_extraction_patterns
      .number_of_descendant_levels_to_search_overrides["fred.example"] = 33;

  const std::vector<std::string> site_candidates = {"fred.example"};

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ(
      33U, order_id_extraction_patterns->number_of_descendant_levels_to_search);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsResolvesValidationPatternPrecedence) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns.default_validation_pattern =
      "^[A-Za-z0-9]{8}$";
  component_order_id_extraction_patterns
      .validation_pattern_overrides["waldo.example"] = "^[A-Fa-f0-9]{24}$";

  const std::vector<std::string> site_candidates = {"waldo.example"};

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ("^[A-Fa-f0-9]{24}$",
            order_id_extraction_patterns->validation_pattern);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsFallsBackToDefaultValidationPatternForSite) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns.default_validation_pattern =
      "^[A-Za-z0-9]{8}$";

  const std::vector<std::string> site_candidates = {"unconfigured.com"};

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ("^[A-Za-z0-9]{8}$",
            order_id_extraction_patterns->validation_pattern);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsHasNoValidationPatternIfNoDefaultOrOverride) {
  // Arrange
  const ComponentOrderIdExtractionPatternsInfo
      component_order_id_extraction_patterns;

  const std::vector<std::string> site_candidates;

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_FALSE(order_id_extraction_patterns->validation_pattern);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsResolvesOverriddenPropertyIndependently) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns
      .default_max_combined_text_length_to_search = 4096;
  component_order_id_extraction_patterns
      .default_number_of_descendant_levels_to_search = 32;
  component_order_id_extraction_patterns.default_validation_pattern =
      "^[A-Za-z0-9]{8}$";
  component_order_id_extraction_patterns
      .number_of_descendant_levels_to_search_overrides["fred.example"] = 33;

  const std::vector<std::string> site_candidates = {"fred.example"};

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ(
      33U, order_id_extraction_patterns->number_of_descendant_levels_to_search);
  EXPECT_EQ(4096U,
            order_id_extraction_patterns->max_combined_text_length_to_search);
  EXPECT_EQ("^[A-Za-z0-9]{8}$",
            order_id_extraction_patterns->validation_pattern);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsHasNoSelectorsIfNoSiteCandidateMatches) {
  // Arrange
  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = "[data-order-id]";

  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns
      .selectors["waldo.example"][OrderExtractionPageType::kThankYou] = {
      order_extraction_selector};

  const std::vector<std::string> site_candidates = {"plugh.example"};

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_THAT(order_id_extraction_patterns->selectors, ::testing::IsEmpty());
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderValuePatternsResolvesCurrencyKeywordAndAncestorDepth) {
  // Arrange
  ComponentOrderValueExtractionPatternsInfo
      component_order_value_extraction_patterns;
  component_order_value_extraction_patterns.currency_regexes = {
      {"EUR", "eur regex"}, {"USD", "usd regex"}};
  component_order_value_extraction_patterns.keyword_regexes = {
      {"en", "english keyword regex"}, {"de", "german keyword regex"}};
  component_order_value_extraction_patterns
      .default_number_of_ancestor_levels_to_search = 6;

  const std::vector<std::string> site_candidates;

  // Act
  const mojom::OrderValueExtractionPatternsInfoPtr
      order_value_extraction_patterns = ToOrderValueExtractionPatterns(
          component_order_value_extraction_patterns, site_candidates,
          /*detected_language=*/"de");

  // Assert
  EXPECT_EQ(2U, order_value_extraction_patterns->currency_regexes.size());
  ASSERT_EQ(2U, order_value_extraction_patterns->keyword_regexes.size());
  EXPECT_EQ("german keyword regex",
            order_value_extraction_patterns->keyword_regexes.front());
  EXPECT_EQ(
      6U, order_value_extraction_patterns->number_of_ancestor_levels_to_search);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderValuePatternsResolvesAncestorDepthSiteOverride) {
  // Arrange
  ComponentOrderValueExtractionPatternsInfo
      component_order_value_extraction_patterns;
  component_order_value_extraction_patterns
      .default_number_of_ancestor_levels_to_search = 6;
  component_order_value_extraction_patterns
      .number_of_ancestor_levels_to_search_overrides["fred.example"] = 7;

  const std::vector<std::string> site_candidates = {"fred.example"};

  // Act
  const mojom::OrderValueExtractionPatternsInfoPtr
      order_value_extraction_patterns = ToOrderValueExtractionPatterns(
          component_order_value_extraction_patterns, site_candidates,
          /*detected_language=*/"");

  // Assert
  EXPECT_EQ(
      7U, order_value_extraction_patterns->number_of_ancestor_levels_to_search);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderIdPatternsPrefersMostSpecificSiteSearchOverride) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo component_order_id_extraction_patterns;
  component_order_id_extraction_patterns
      .number_of_descendant_levels_to_search_overrides
          ["checkout.waldo.example"] = 40;
  component_order_id_extraction_patterns
      .number_of_descendant_levels_to_search_overrides["waldo.example"] = 32;

  const std::vector<std::string> site_candidates = {"checkout.waldo.example",
                                                    "waldo.example"};

  // Act
  const mojom::OrderIdExtractionPatternsInfoPtr order_id_extraction_patterns =
      ToOrderIdExtractionPatterns(component_order_id_extraction_patterns,
                                  site_candidates,
                                  /*detected_language=*/std::nullopt);

  // Assert
  EXPECT_EQ(
      40U, order_id_extraction_patterns->number_of_descendant_levels_to_search);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderValuePatternsSelectsConfiguredSiteSelectors) {
  // Arrange
  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = R"([data-test="checkout-total"])";

  ComponentOrderValueExtractionPatternsInfo
      component_order_value_extraction_patterns;
  component_order_value_extraction_patterns
      .selectors["fred.example"][OrderExtractionPageType::kCheckout] = {
      order_extraction_selector};

  const std::vector<std::string> site_candidates = {"fred.example"};

  // Act
  const mojom::OrderValueExtractionPatternsInfoPtr
      order_value_extraction_patterns = ToOrderValueExtractionPatterns(
          component_order_value_extraction_patterns, site_candidates,
          /*detected_language=*/"");

  // Assert
  ASSERT_EQ(1U, order_value_extraction_patterns->selectors.size());
  EXPECT_EQ(order_extraction_selector.selector,
            order_value_extraction_patterns->selectors.front()->selector);
}

TEST(BraveAdsOrderExtractionPatternsUtilTest,
     ToOrderValuePatternsHasNoSelectorsIfNoneConfiguredForSite) {
  // Arrange
  const ComponentOrderValueExtractionPatternsInfo
      component_order_value_extraction_patterns;

  const std::vector<std::string> site_candidates = {"brave.com"};

  // Act
  const mojom::OrderValueExtractionPatternsInfoPtr
      order_value_extraction_patterns = ToOrderValueExtractionPatterns(
          component_order_value_extraction_patterns, site_candidates,
          /*detected_language=*/"en");

  // Assert
  EXPECT_TRUE(order_value_extraction_patterns->selectors.empty());
}

}  // namespace brave_ads
