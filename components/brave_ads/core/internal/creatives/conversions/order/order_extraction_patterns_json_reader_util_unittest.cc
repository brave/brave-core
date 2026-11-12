/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads::json::reader {

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest, ParseCheckoutPageType) {
  // Act & Assert
  EXPECT_EQ(OrderExtractionPageType::kCheckout, ParsePageType("checkout"));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest, ParseThankYouPageType) {
  // Act & Assert
  EXPECT_EQ(OrderExtractionPageType::kThankYou, ParsePageType("thankYou"));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoNotParseUnrecognizedPageType) {
  // Act & Assert
  EXPECT_FALSE(ParsePageType("cart"));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest, ParseStringMap) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "de": "german regex",
      "en": "english regex"
    })JSON");

  // Act
  const base::flat_map<std::string, std::string> string_map =
      ParseStringMap(&dict);

  // Assert
  EXPECT_EQ("german regex", string_map.at("de"));
  EXPECT_EQ("english regex", string_map.at("en"));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoNotParseNonStringValuesInStringMap) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "de": 42
    })JSON");

  // Act & Assert
  EXPECT_TRUE(ParseStringMap(&dict).empty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoNotParseStringMapFromNullDict) {
  // Act & Assert
  EXPECT_TRUE(ParseStringMap(/*dict=*/nullptr).empty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     ParseSelectorListWithTextSource) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "[data-test=\"checkout-total\"]",
        "source": "text"
      }
    ])JSON");

  // Act
  const OrderExtractionSelectors order_extraction_selectors =
      ParseSelectorList(list);

  // Assert
  ASSERT_EQ(1u, order_extraction_selectors.size());
  EXPECT_EQ(R"([data-test="checkout-total"])",
            order_extraction_selectors[0].selector);
  EXPECT_FALSE(order_extraction_selectors[0].attribute_name);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     ParseSelectorListWithAttributeSource) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "[data-order-id]",
        "source": "attribute",
        "attribute": "data-order-id"
      }
    ])JSON");

  // Act
  const OrderExtractionSelectors order_extraction_selectors =
      ParseSelectorList(list);

  // Assert
  ASSERT_EQ(1u, order_extraction_selectors.size());
  EXPECT_EQ("[data-order-id]", order_extraction_selectors[0].selector);
  EXPECT_EQ("data-order-id", order_extraction_selectors[0].attribute_name);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoNotParseSelectorListEntryMissingSelectorKey) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "source": "attribute",
        "attribute": "data-order-id"
      }
    ])JSON");

  // Act & Assert
  EXPECT_TRUE(ParseSelectorList(list).empty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoNotSetAttributeNameIfSourceIsNotAttribute) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "[data-testid=\"orderNumber\"]",
        "source": "text",
        "attribute": "data-order-id"
      }
    ])JSON");

  // Act
  const OrderExtractionSelectors order_extraction_selectors =
      ParseSelectorList(list);

  // Assert
  ASSERT_EQ(1u, order_extraction_selectors.size());
  EXPECT_FALSE(order_extraction_selectors[0].attribute_name);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     ParseSelectorsByDomain) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "otto.de": {
        "checkout": [
          {
            "selector": "[data-shopping-cart-id]",
            "source": "attribute",
            "attribute": "data-shopping-cart-id"
          }
        ],
        "thankYou": [
          {
            "selector": "[data-order-id]",
            "source": "attribute",
            "attribute": "data-order-id"
          }
        ]
      }
    })JSON");

  // Act
  const auto selectors_by_domain = ParseSelectorsByDomain(&dict);

  // Assert
  const auto& selectors_by_page_type = selectors_by_domain.at("otto.de");
  EXPECT_EQ("[data-shopping-cart-id]",
            selectors_by_page_type.at(OrderExtractionPageType::kCheckout)[0]
                .selector);
  EXPECT_EQ("[data-order-id]",
            selectors_by_page_type.at(OrderExtractionPageType::kThankYou)[0]
                .selector);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoNotParseSelectorsByDomainWithUnrecognizedPageType) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "otto.de": {
        "cart": [
          {
            "selector": "[data-order-id]"
          }
        ]
      }
    })JSON");

  // Act & Assert
  EXPECT_TRUE(ParseSelectorsByDomain(&dict).empty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoNotParseSelectorsByDomainWhenPageTypesIsNotADict) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "otto.de": "not a dict"
    })JSON");

  // Act & Assert
  EXPECT_TRUE(ParseSelectorsByDomain(&dict).empty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoNotParseSelectorsByDomainFromNullDict) {
  // Act & Assert
  EXPECT_TRUE(ParseSelectorsByDomain(/*dict=*/nullptr).empty());
}

}  // namespace brave_ads::json::reader
