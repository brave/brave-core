/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_flattener.h"

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_page_type.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_value_extraction_patterns_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderExtractionPatternsFlattenerTest,
     FlattenOrderIdPatternsOrdersDetectedLanguageFirst) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo order_id_extraction_patterns;
  order_id_extraction_patterns.regexes = {
      {"en", "english regex"}, {"de", "german regex"}, {"fr", "french regex"}};

  const std::vector<std::string> domain_candidates;

  // Act
  const OrderIdExtractionPatternsInfo flattened_order_id_extraction_patterns =
      FlattenOrderIdExtractionPatterns(order_id_extraction_patterns,
                                       domain_candidates,
                                       /*detected_language=*/"de");

  // Assert
  EXPECT_EQ("german regex",
            flattened_order_id_extraction_patterns.regexes.front());
  EXPECT_EQ(3U, flattened_order_id_extraction_patterns.regexes.size());
}

TEST(BraveAdsOrderExtractionPatternsFlattenerTest,
     FlattenOrderIdPatternsTriesAllLanguagesIfDetectedLanguageIsUnknown) {
  // Arrange
  ComponentOrderIdExtractionPatternsInfo order_id_extraction_patterns;
  order_id_extraction_patterns.regexes = {{"en", "english regex"},
                                          {"de", "german regex"}};

  const std::vector<std::string> domain_candidates;

  // Act
  const OrderIdExtractionPatternsInfo flattened_order_id_extraction_patterns =
      FlattenOrderIdExtractionPatterns(order_id_extraction_patterns,
                                       domain_candidates,
                                       /*detected_language=*/"");

  // Assert
  EXPECT_EQ(2U, flattened_order_id_extraction_patterns.regexes.size());
}

TEST(BraveAdsOrderExtractionPatternsFlattenerTest,
     FlattenOrderIdPatternsSelectsMostSpecificDomainSelectors) {
  // Arrange
  OrderExtractionSelectorInfo checkout_order_extraction_selector;
  checkout_order_extraction_selector.selector = "[data-shopping-cart-id]";
  checkout_order_extraction_selector.attribute_name = "data-shopping-cart-id";

  OrderExtractionSelectorInfo thank_you_order_extraction_selector;
  thank_you_order_extraction_selector.selector = "[data-order-id]";
  thank_you_order_extraction_selector.attribute_name = "data-order-id";

  ComponentOrderIdExtractionPatternsInfo order_id_extraction_patterns;
  order_id_extraction_patterns
      .selectors["otto.de"][OrderExtractionPageType::kCheckout] = {
      checkout_order_extraction_selector};
  order_id_extraction_patterns
      .selectors["otto.de"][OrderExtractionPageType::kThankYou] = {
      thank_you_order_extraction_selector};

  // Trying the most specific host first, matching real resolution order.
  const std::vector<std::string> domain_candidates = {"checkout.otto.de",
                                                      "otto.de"};

  // Act
  const OrderIdExtractionPatternsInfo flattened_order_id_extraction_patterns =
      FlattenOrderIdExtractionPatterns(order_id_extraction_patterns,
                                       domain_candidates,
                                       /*detected_language=*/"");

  // Assert, checkout selectors first since a tab may already be past
  // checkout by the time extraction runs.
  ASSERT_EQ(2U, flattened_order_id_extraction_patterns.selectors.size());
  EXPECT_EQ(checkout_order_extraction_selector,
            flattened_order_id_extraction_patterns.selectors.front());
  EXPECT_EQ(thank_you_order_extraction_selector,
            flattened_order_id_extraction_patterns.selectors.back());
}

TEST(BraveAdsOrderExtractionPatternsFlattenerTest,
     FlattenOrderIdPatternsHasNoSelectorsIfNoDomainCandidateMatches) {
  // Arrange
  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = "[data-order-id]";

  ComponentOrderIdExtractionPatternsInfo order_id_extraction_patterns;
  order_id_extraction_patterns
      .selectors["otto.de"][OrderExtractionPageType::kThankYou] = {
      order_extraction_selector};

  const std::vector<std::string> domain_candidates = {"zalando.de"};

  // Act
  const OrderIdExtractionPatternsInfo flattened_order_id_extraction_patterns =
      FlattenOrderIdExtractionPatterns(order_id_extraction_patterns,
                                       domain_candidates,
                                       /*detected_language=*/"");

  // Assert
  EXPECT_TRUE(flattened_order_id_extraction_patterns.selectors.empty());
}

TEST(BraveAdsOrderExtractionPatternsFlattenerTest,
     FlattenOrderValuePatternsResolvesCurrencyKeywordAndAncestorDepth) {
  // Arrange
  ComponentOrderValueExtractionPatternsInfo order_value_extraction_patterns;
  order_value_extraction_patterns.currency_regexes = {{"EUR", "eur regex"},
                                                      {"USD", "usd regex"}};
  order_value_extraction_patterns.keyword_regexes = {
      {"en", "english keyword regex"}, {"de", "german keyword regex"}};
  order_value_extraction_patterns.number_of_ancestor_levels_to_search = 6;

  const std::vector<std::string> domain_candidates;

  // Act
  const OrderValueExtractionPatternsInfo
      flattened_order_value_extraction_patterns =
          FlattenOrderValueExtractionPatterns(order_value_extraction_patterns,
                                              domain_candidates,
                                              /*detected_language=*/"de");

  // Assert
  EXPECT_EQ(2U,
            flattened_order_value_extraction_patterns.currency_regexes.size());
  EXPECT_EQ("german keyword regex",
            flattened_order_value_extraction_patterns.keyword_regexes.front());
  EXPECT_EQ(6U, flattened_order_value_extraction_patterns
                    .number_of_ancestor_levels_to_search);
}

TEST(BraveAdsOrderExtractionPatternsFlattenerTest,
     FlattenOrderValuePatternsSelectsConfiguredDomainSelectors) {
  // Arrange
  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = "[data-test=\"checkout-total\"]";

  ComponentOrderValueExtractionPatternsInfo order_value_extraction_patterns;
  order_value_extraction_patterns
      .selectors["mediamarkt.de"][OrderExtractionPageType::kCheckout] = {
      order_extraction_selector};

  const std::vector<std::string> domain_candidates = {"mediamarkt.de"};

  // Act
  const OrderValueExtractionPatternsInfo
      flattened_order_value_extraction_patterns =
          FlattenOrderValueExtractionPatterns(order_value_extraction_patterns,
                                              domain_candidates,
                                              /*detected_language=*/"");

  // Assert
  ASSERT_EQ(1U, flattened_order_value_extraction_patterns.selectors.size());
  EXPECT_EQ(order_extraction_selector,
            flattened_order_value_extraction_patterns.selectors.front());
}

TEST(BraveAdsOrderExtractionPatternsFlattenerTest,
     FlattenOrderValuePatternsHasNoSelectorsIfNoneConfiguredForDomain) {
  // Arrange
  const ComponentOrderValueExtractionPatternsInfo
      order_value_extraction_patterns;

  const std::vector<std::string> domain_candidates = {"brave.com"};

  // Act
  const OrderValueExtractionPatternsInfo
      flattened_order_value_extraction_patterns =
          FlattenOrderValueExtractionPatterns(order_value_extraction_patterns,
                                              domain_candidates,
                                              /*detected_language=*/"en");

  // Assert
  EXPECT_TRUE(flattened_order_value_extraction_patterns.selectors.empty());
}

}  // namespace brave_ads
