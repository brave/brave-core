/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_mojom_util.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_selector_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::IsEmpty;

TEST(BraveAdsOrderExtractionPatternsMojomUtilTest,
     DoNotConvertToMojomOrderExtractionPatternsIfNeitherIdNorValue) {
  // Act & Assert
  EXPECT_FALSE(ToMojomOrderExtractionPatterns(/*id=*/std::nullopt,
                                              /*value=*/std::nullopt));
}

TEST(BraveAdsOrderExtractionPatternsMojomUtilTest,
     ConvertIdOnlyToMojomOrderExtractionPatterns) {
  // Arrange
  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = "[data-order-id]";
  order_extraction_selector.attribute_name = "data-order-id";

  OrderIdExtractionPatternsInfo order_id_extraction_patterns;
  order_id_extraction_patterns.regexes = {"(?i)^Bestellnummer:\\s*(\\w+)$"};
  order_id_extraction_patterns.selectors = {order_extraction_selector};

  // Act
  const mojom::OrderExtractionPatternsInfoPtr mojom_patterns =
      ToMojomOrderExtractionPatterns(std::move(order_id_extraction_patterns),
                                     /*value=*/std::nullopt);

  // Assert
  ASSERT_TRUE(mojom_patterns);
  ASSERT_TRUE(mojom_patterns->id);
  EXPECT_FALSE(mojom_patterns->value);
  EXPECT_EQ(std::vector<std::string>{"(?i)^Bestellnummer:\\s*(\\w+)$"},
            mojom_patterns->id->regexes);
  ASSERT_EQ(1u, mojom_patterns->id->selectors.size());
  EXPECT_EQ("[data-order-id]", mojom_patterns->id->selectors[0]->selector);
  EXPECT_EQ("data-order-id", mojom_patterns->id->selectors[0]->attribute_name);
}

TEST(BraveAdsOrderExtractionPatternsMojomUtilTest,
     ConvertValueOnlyToMojomOrderExtractionPatterns) {
  // Arrange
  OrderValueExtractionPatternsInfo order_value_extraction_patterns;
  order_value_extraction_patterns.currency_regexes = {{"EUR", "(?i)EUR|€"}};
  order_value_extraction_patterns.keyword_regexes = {"(?i)^Gesamt$"};
  order_value_extraction_patterns.number_of_ancestor_levels_to_search = 6;

  // Act
  const mojom::OrderExtractionPatternsInfoPtr mojom_patterns =
      ToMojomOrderExtractionPatterns(
          /*id=*/std::nullopt, std::move(order_value_extraction_patterns));

  // Assert
  ASSERT_TRUE(mojom_patterns);
  EXPECT_FALSE(mojom_patterns->id);
  ASSERT_TRUE(mojom_patterns->value);
  EXPECT_EQ((base::flat_map<std::string, std::string>{{"EUR", "(?i)EUR|€"}}),
            mojom_patterns->value->currency_regexes);
  EXPECT_EQ(std::vector<std::string>{"(?i)^Gesamt$"},
            mojom_patterns->value->keyword_regexes);
  EXPECT_EQ(6u, mojom_patterns->value->number_of_ancestor_levels_to_search);
  EXPECT_THAT(mojom_patterns->value->selectors, IsEmpty());
}

TEST(BraveAdsOrderExtractionPatternsMojomUtilTest,
     DoNotSetMojomAttributeNameIfUnset) {
  // Arrange
  OrderExtractionSelectorInfo order_extraction_selector;
  order_extraction_selector.selector = "[data-test=\"checkout-total\"]";

  OrderIdExtractionPatternsInfo order_id_extraction_patterns;
  order_id_extraction_patterns.selectors = {order_extraction_selector};

  // Act
  const mojom::OrderExtractionPatternsInfoPtr mojom_patterns =
      ToMojomOrderExtractionPatterns(std::move(order_id_extraction_patterns),
                                     /*value=*/std::nullopt);

  // Assert
  ASSERT_TRUE(mojom_patterns);
  ASSERT_TRUE(mojom_patterns->id);
  ASSERT_EQ(1u, mojom_patterns->id->selectors.size());
  EXPECT_FALSE(mojom_patterns->id->selectors[0]->attribute_name);
}

}  // namespace brave_ads
