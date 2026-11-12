/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Field;
using ::testing::IsEmpty;

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest, ReadOrderPatterns) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "regex": {
          "source": "elementText",
          "patterns": {
            "de": "(?i)^Bestellnummer:\\s*(\\w+)$",
            "en": "(?i)^Order Number:\\s*(\\w+)$"
          }
        },
        "selectors": {
          "otto.de": {
            "thankYou": [
              {
                "selector": "[data-order-id]",
                "source": "attribute",
                "attribute": "data-order-id"
              }
            ]
          }
        }
      },
      "value": {
        "regex": {
          "currency": {
            "EUR": "(?i)EUR|€"
          },
          "keyword": {
            "de": "(?i)^Gesamt$"
          },
          "numberOfAncestorLevelsToSearch": 6
        },
        "selectors": {
          "mediamarkt.de": {
            "checkout": [
              {
                "selector": "[data-test=\"checkout-total\"]",
                "source": "text"
              }
            ]
          }
        }
      }
    })JSON");

  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      json::reader::ReadOrderPatterns(dict);

  // Assert
  ComponentOrderIdExtractionPatternsInfo expected_order_id_extraction_patterns;
  expected_order_id_extraction_patterns.source = "elementText";
  expected_order_id_extraction_patterns.regexes["de"] =
      "(?i)^Bestellnummer:\\s*(\\w+)$";
  expected_order_id_extraction_patterns.regexes["en"] =
      "(?i)^Order Number:\\s*(\\w+)$";
  OrderExtractionSelectorInfo id_order_extraction_selector;
  id_order_extraction_selector.selector = "[data-order-id]";
  id_order_extraction_selector.attribute_name = "data-order-id";
  expected_order_id_extraction_patterns
      .selectors["otto.de"][OrderExtractionPageType::kThankYou] = {
      id_order_extraction_selector};
  EXPECT_EQ(expected_order_id_extraction_patterns,
            order_extraction_patterns.id);

  ComponentOrderValueExtractionPatternsInfo
      expected_order_value_extraction_patterns;
  expected_order_value_extraction_patterns.currency_regexes["EUR"] =
      "(?i)EUR|€";
  expected_order_value_extraction_patterns.keyword_regexes["de"] =
      "(?i)^Gesamt$";
  expected_order_value_extraction_patterns.number_of_ancestor_levels_to_search =
      6;
  OrderExtractionSelectorInfo value_order_extraction_selector;
  value_order_extraction_selector.selector = R"([data-test="checkout-total"])";
  expected_order_value_extraction_patterns
      .selectors["mediamarkt.de"][OrderExtractionPageType::kCheckout] = {
      value_order_extraction_selector};
  EXPECT_EQ(expected_order_value_extraction_patterns,
            order_extraction_patterns.value);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     ReadEmptyOrderPatternsIfMissingIdAndValue) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON({})JSON");

  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      json::reader::ReadOrderPatterns(dict);

  // Assert
  EXPECT_EQ(ComponentOrderIdExtractionPatternsInfo(),
            order_extraction_patterns.id);
  EXPECT_EQ(ComponentOrderValueExtractionPatternsInfo(),
            order_extraction_patterns.value);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     DoNotReadIdRegexesIfNotStrings) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "regex": {
          "source": "elementText",
          "patterns": {
            "de": 42
          }
        }
      }
    })JSON");

  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      json::reader::ReadOrderPatterns(dict);

  // Assert
  EXPECT_THAT(order_extraction_patterns.id.regexes, IsEmpty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     DoNotReadIdSelectorIfMissingSelectorKey) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "selectors": {
          "otto.de": {
            "thankYou": [
              {
                "source": "attribute",
                "attribute": "data-order-id"
              }
            ]
          }
        }
      }
    })JSON");

  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      json::reader::ReadOrderPatterns(dict);

  // Assert
  EXPECT_THAT(order_extraction_patterns.id.selectors, IsEmpty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     DoNotSetAttributeNameIfSourceIsNotAttribute) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "selectors": {
          "zalando.de": {
            "thankYou": [
              {
                "selector": "[data-testid=\"orderNumber\"]",
                "source": "text",
                "attribute": "data-order-id"
              }
            ]
          }
        }
      }
    })JSON");

  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      json::reader::ReadOrderPatterns(dict);

  // Assert
  EXPECT_THAT(order_extraction_patterns.id.selectors.at("zalando.de")
                  .at(OrderExtractionPageType::kThankYou),
              ElementsAre(Field(&OrderExtractionSelectorInfo::attribute_name,
                                Eq(std::nullopt))));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     DoNotReadSelectorsByDomainIfPageTypesIsNotADict) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "selectors": {
          "otto.de": "not a dict"
        }
      }
    })JSON");

  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      json::reader::ReadOrderPatterns(dict);

  // Assert
  EXPECT_THAT(order_extraction_patterns.id.selectors, IsEmpty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     DoNotReadSelectorListEntryIfNotADict) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "selectors": {
          "otto.de": {
            "thankYou": ["not a dict"]
          }
        }
      }
    })JSON");

  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      json::reader::ReadOrderPatterns(dict);

  // Assert, a domain/page type whose list contains only unparseable
  // entries is dropped rather than kept as an empty list.
  EXPECT_THAT(order_extraction_patterns.id.selectors, IsEmpty());
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     DefaultNumberOfAncestorLevelsToSearchIfMissing) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "value": {
        "regex": {
          "currency": {
            "EUR": "(?i)EUR|€"
          }
        }
      }
    })JSON");

  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      json::reader::ReadOrderPatterns(dict);

  // Assert
  EXPECT_EQ(
      0u, order_extraction_patterns.value.number_of_ancestor_levels_to_search);
}

}  // namespace brave_ads
