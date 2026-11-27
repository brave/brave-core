/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads::json::reader {

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest, ParseCheckoutPageType) {
  // Act & Assert
  EXPECT_EQ(OrderExtractionPageType::kCheckout, MaybeParsePageType("checkout"));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest, ParseThankYouPageType) {
  // Act & Assert
  EXPECT_EQ(OrderExtractionPageType::kThankYou, MaybeParsePageType("thankYou"));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoesNotParseUnrecognizedPageType) {
  // Act & Assert
  EXPECT_FALSE(MaybeParsePageType("cart"));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     ParseSelectorListWithTextContentSelector) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "[data-test=\"checkout-total\"]"
      }
    ])JSON");

  // Act
  std::optional<OrderExtractionSelectors> order_extraction_selectors =
      MaybeParseSelectorList(list);

  // Assert
  ASSERT_TRUE(order_extraction_selectors);
  ASSERT_EQ(1U, order_extraction_selectors->size());
  EXPECT_EQ(R"([data-test="checkout-total"])",
            (*order_extraction_selectors)[0].selector);
  EXPECT_FALSE((*order_extraction_selectors)[0].attribute_name);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     ParseSelectorListWithAttributeNameSelector) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "[data-order-id]",
        "attributeName": "data-order-id"
      }
    ])JSON");

  // Act
  std::optional<OrderExtractionSelectors> order_extraction_selectors =
      MaybeParseSelectorList(list);

  // Assert
  ASSERT_TRUE(order_extraction_selectors);
  ASSERT_EQ(1U, order_extraction_selectors->size());
  EXPECT_EQ("[data-order-id]", (*order_extraction_selectors)[0].selector);
  EXPECT_EQ("data-order-id", (*order_extraction_selectors)[0].attribute_name);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     ParseSelectorListWithPatternAndValidationPattern) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "script:not([src])",
        "pattern": "(?i)xvpn:sub:([0-9a-f-]+)",
        "validation": {
          "pattern": "^[A-Fa-f0-9-]{36}$"
        }
      }
    ])JSON");

  // Act
  std::optional<OrderExtractionSelectors> order_extraction_selectors =
      MaybeParseSelectorList(list);

  // Assert
  ASSERT_TRUE(order_extraction_selectors);
  ASSERT_EQ(1U, order_extraction_selectors->size());
  EXPECT_EQ("(?i)xvpn:sub:([0-9a-f-]+)",
            (*order_extraction_selectors)[0].pattern);
  EXPECT_EQ("^[A-Fa-f0-9-]{36}$",
            (*order_extraction_selectors)[0].validation_pattern);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     TreatEmptyValidationObjectAsAbsent) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "[data-order-id]",
        "validation": {}
      }
    ])JSON");

  // Act
  std::optional<OrderExtractionSelectors> order_extraction_selectors =
      MaybeParseSelectorList(list);

  // Assert
  ASSERT_TRUE(order_extraction_selectors);
  ASSERT_EQ(1U, order_extraction_selectors->size());
  EXPECT_FALSE((*order_extraction_selectors)[0].validation_pattern);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     RejectSelectorListEntryMissingSelectorKey) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "attributeName": "data-order-id"
      }
    ])JSON");

  // Act & Assert
  EXPECT_FALSE(MaybeParseSelectorList(list));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     RejectSelectorListEntryWithEmptySelector) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": ""
      }
    ])JSON");

  // Act & Assert
  EXPECT_FALSE(MaybeParseSelectorList(list));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     RejectSelectorListEntryWithEmptyAttributeName) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "[data-order-id]",
        "attributeName": ""
      }
    ])JSON");

  // Act & Assert
  EXPECT_FALSE(MaybeParseSelectorList(list));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     RejectSelectorListEntryThatIsNotADict) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      "not a dict"
    ])JSON");

  // Act & Assert
  EXPECT_FALSE(MaybeParseSelectorList(list));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     DoesNotSetAttributeNameIfMissing) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(R"JSON(
    [
      {
        "selector": "[data-testid=\"orderNumber\"]"
      }
    ])JSON");

  // Act
  std::optional<OrderExtractionSelectors> order_extraction_selectors =
      MaybeParseSelectorList(list);

  // Assert
  ASSERT_TRUE(order_extraction_selectors);
  ASSERT_EQ(1U, order_extraction_selectors->size());
  EXPECT_FALSE((*order_extraction_selectors)[0].attribute_name);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     ParseSelectorsByPageType) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "checkout": [
        {
          "selector": "[data-shopping-cart-id]",
          "attributeName": "data-shopping-cart-id"
        }
      ],
      "thankYou": [
        {
          "selector": "[data-order-id]",
          "attributeName": "data-order-id"
        }
      ]
    })JSON");

  // Act
  std::optional<
      base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
      selectors_by_page_type = MaybeParseSelectorsByPageType(dict);

  // Assert
  ASSERT_TRUE(selectors_by_page_type);
  EXPECT_EQ("[data-shopping-cart-id]",
            selectors_by_page_type->at(OrderExtractionPageType::kCheckout)[0]
                .selector);
  EXPECT_EQ("[data-order-id]",
            selectors_by_page_type->at(OrderExtractionPageType::kThankYou)[0]
                .selector);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     IgnoreUnrecognizedPageTypeKey) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "cart": [
        {
          "selector": "[data-order-id]"
        }
      ]
    })JSON");

  // Act
  std::optional<
      base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
      selectors_by_page_type = MaybeParseSelectorsByPageType(dict);

  // Assert
  ASSERT_TRUE(selectors_by_page_type);
  EXPECT_TRUE(selectors_by_page_type->empty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderUtilTest,
     RejectSelectorsByPageTypeWhenRecognizedKeyValueIsNotAList) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "checkout": "not a list"
    })JSON");

  // Act & Assert
  EXPECT_FALSE(MaybeParseSelectorsByPageType(dict));
}

}  // namespace brave_ads::json::reader
