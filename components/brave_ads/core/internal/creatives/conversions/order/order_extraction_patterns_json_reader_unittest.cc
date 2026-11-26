/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/test/order_extraction_patterns_resource_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     ReadConversionConfigMatchesResource) {
  // Act
  const OrderExtractionPatternsInfo order_extraction_patterns =
      test::ReadConversionConfigFromResource();

  // Assert
  EXPECT_EQ(test::BuildExpectedConversionConfig(), order_extraction_patterns);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     ReadEmptyOrderPatternsIfMissingIdAndValue) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON({})JSON");

  // Act
  std::optional<OrderExtractionPatternsInfo> order_extraction_patterns =
      json::reader::MaybeReadOrderPatterns(dict);

  // Assert
  ASSERT_TRUE(order_extraction_patterns);
  EXPECT_EQ(ComponentOrderIdExtractionPatternsInfo(),
            order_extraction_patterns->id);
  EXPECT_EQ(ComponentOrderValueExtractionPatternsInfo(),
            order_extraction_patterns->value);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfIdRegexEntryIsNotAString) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "regex": {
          "patterns": {
            "de": 42
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfIdRegexMapIsEmpty) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "regex": {
          "patterns": {}
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfIdRegexEntryIsEmptyString) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "regex": {
          "patterns": {
            "en": ""
          }
        }
      },
      "value": {}
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfValueRegexMapHasZeroCompilablePatterns) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "value": {
        "regex": {
          "currency": {
            "EUR": "(unterminated"
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     SkipNonCompilingCurrencyRegexButKeepOthers) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "value": {
        "regex": {
          "currency": {
            "EUR": "(?i)EUR|€",
            "USD": "(unterminated"
          }
        }
      }
    })JSON");

  // Act
  std::optional<OrderExtractionPatternsInfo> order_extraction_patterns =
      json::reader::MaybeReadOrderPatterns(dict);

  // Assert
  ASSERT_TRUE(order_extraction_patterns);
  EXPECT_THAT(order_extraction_patterns->value.currency_regexes,
              testing::ElementsAre(testing::Pair("EUR", "(?i)EUR|€")));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSelectorEntryMissingSelectorKey) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "sites": {
        "bar.example": {
          "id": {
            "thankYou": [
              {
                "attributeName": "data-order-id"
              }
            ]
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSelectorEntryHasEmptyAttributeName) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "sites": {
        "bar.example": {
          "id": {
            "thankYou": [
              {
                "selector": "[data-order-id]",
                "attributeName": ""
              }
            ]
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     DoesNotSetAttributeNameIfMissing) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "validation": {
          "defaultPattern": "^[A-Za-z0-9]{8}$"
        }
      },
      "sites": {
        "corge.example": {
          "id": {
            "thankYou": [
              {
                "selector": "[data-testid=\"orderNumber\"]"
              }
            ]
          }
        }
      }
    })JSON");

  // Act
  std::optional<OrderExtractionPatternsInfo> order_extraction_patterns =
      json::reader::MaybeReadOrderPatterns(dict);

  // Assert
  ASSERT_TRUE(order_extraction_patterns);
  EXPECT_THAT(order_extraction_patterns->id.selectors.at("corge.example")
                  .at(OrderExtractionPageType::kThankYou),
              testing::ElementsAre(
                  testing::Field(&OrderExtractionSelectorInfo::attribute_name,
                                 testing::Eq(std::nullopt))));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSiteEntryIsNotADict) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "sites": {
        "bar.example": "not a dict"
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSelectorListEntryIsNotADict) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "sites": {
        "bar.example": {
          "id": {
            "thankYou": ["not a dict"]
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfPageTypeListValueIsNotAList) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "sites": {
        "bar.example": {
          "id": {
            "thankYou": "not a list"
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfIdSearchBoundsMissingWhenRegexPatternsConfigured) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "validation": {
          "defaultPattern": "^[A-Za-z0-9]{8}$"
        },
        "regex": {
          "patterns": {
            "en": "(?i)^Order Number:\\s*(\\w+)$"
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(
    BraveAdsOrderExtractionPatternsJsonReaderTest,
    RejectUpdateIfIdValidationDefaultPatternMissingWhenRegexPatternsConfigured) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "regex": {
          "patterns": {
            "en": "(?i)^Order Number:\\s*(\\w+)$"
          }
        },
        "search": {
          "defaultMaxDescendantDepth": 32,
          "defaultMaxCombinedTextLengthInBytes": 8192
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfIdValidationDefaultPatternIsEmpty) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "validation": {
          "defaultPattern": ""
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfIdValidationDefaultPatternDoesNotCompile) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "validation": {
          "defaultPattern": "(unterminated"
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSiteIdValidationPatternIsEmpty) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "sites": {
        "waldo.example": {
          "id": {
            "validation": {
              "pattern": ""
            }
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSelectorValidationPatternDoesNotCompile) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "id": {
        "validation": {
          "defaultPattern": "^[A-Za-z0-9]{8}$"
        }
      },
      "sites": {
        "bar.example": {
          "id": {
            "thankYou": [
              {
                "selector": "[data-order-id]",
                "validation": {
                  "pattern": "(unterminated"
                }
              }
            ]
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSearchBoundIsNegative) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "value": {
        "search": {
          "defaultMaxAncestorDepth": -1
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSearchBoundIsWrongType) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "value": {
        "search": {
          "defaultMaxAncestorDepth": "6"
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     AcceptExplicitZeroSearchBound) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "value": {
        "search": {
          "defaultMaxAncestorDepth": 0
        }
      }
    })JSON");

  // Act
  std::optional<OrderExtractionPatternsInfo> order_extraction_patterns =
      json::reader::MaybeReadOrderPatterns(dict);

  // Assert
  ASSERT_TRUE(order_extraction_patterns);
  EXPECT_EQ(0, order_extraction_patterns->value
                   .default_number_of_ancestor_levels_to_search);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     RejectUpdateIfSiteSearchOverrideIsNegative) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "sites": {
        "foo.example": {
          "value": {
            "search": {
              "maxAncestorDepth": -1
            }
          }
        }
      }
    })JSON");

  // Act & Assert
  EXPECT_FALSE(json::reader::MaybeReadOrderPatterns(dict));
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
  std::optional<OrderExtractionPatternsInfo> order_extraction_patterns =
      json::reader::MaybeReadOrderPatterns(dict);

  // Assert
  ASSERT_TRUE(order_extraction_patterns);
  EXPECT_EQ(0, order_extraction_patterns->value
                   .default_number_of_ancestor_levels_to_search);
}

TEST(BraveAdsOrderExtractionPatternsJsonReaderTest,
     ReadSiteLevelIdValidationPatternWithNoSelectors) {
  // Arrange
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
    {
      "sites": {
        "waldo.example": {
          "id": {
            "validation": {
              "pattern": "^WLD[0-9]{2}-[0-9]{12}$"
            }
          }
        }
      }
    })JSON");

  // Act
  std::optional<OrderExtractionPatternsInfo> order_extraction_patterns =
      json::reader::MaybeReadOrderPatterns(dict);

  // Assert
  ASSERT_TRUE(order_extraction_patterns);
  EXPECT_THAT(order_extraction_patterns->id.selectors, testing::IsEmpty());
  EXPECT_EQ("^WLD[0-9]{2}-[0-9]{12}$",
            order_extraction_patterns->id.validation_pattern_overrides.at(
                "waldo.example"));
}

}  // namespace brave_ads
