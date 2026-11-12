/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/order_patterns_mojom_util.h"

#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::IsEmpty;

TEST(BraveAdsOrderPatternsMojomUtilTest, ConvertBothIdAndValuePatterns) {
  // Arrange
  mojom::OrderExtractionPatternsInfoPtr mojom_order_patterns =
      mojom::OrderExtractionPatternsInfo::New();

  mojom::OrderIdExtractionPatternsInfoPtr mojom_id =
      mojom::OrderIdExtractionPatternsInfo::New();
  mojom_id->regexes = {"(?i)^Bestellnummer:\\s*(\\w+)$"};
  mojom::OrderExtractionSelectorInfoPtr mojom_id_selector =
      mojom::OrderExtractionSelectorInfo::New();
  mojom_id_selector->selector = "[data-order-id]";
  mojom_id_selector->attribute_name = "data-order-id";
  mojom_id->selectors.push_back(std::move(mojom_id_selector));
  mojom_order_patterns->id = std::move(mojom_id);

  mojom::OrderValueExtractionPatternsInfoPtr mojom_value =
      mojom::OrderValueExtractionPatternsInfo::New();
  mojom_value->currency_regexes = {{"EUR", "(?i)EUR|€"}};
  mojom_value->keyword_regexes = {"(?i)^Gesamt$"};
  mojom_value->number_of_ancestor_levels_to_search = 6;
  mojom::OrderExtractionSelectorInfoPtr mojom_value_selector =
      mojom::OrderExtractionSelectorInfo::New();
  mojom_value_selector->selector = R"([data-test="checkout-total"])";
  mojom_value->selectors.push_back(std::move(mojom_value_selector));
  mojom_order_patterns->value = std::move(mojom_value);

  // Act
  const mojom::OrderPatternsInfoPtr order_patterns =
      ToMojomOrderPatterns(*mojom_order_patterns);

  // Assert
  EXPECT_EQ(std::vector<std::string>{"(?i)^Bestellnummer:\\s*(\\w+)$"},
            order_patterns->id_regexes);
  ASSERT_EQ(1u, order_patterns->id_selectors.size());
  EXPECT_EQ("[data-order-id]", order_patterns->id_selectors[0]->selector);
  EXPECT_EQ("data-order-id", order_patterns->id_selectors[0]->attribute_name);

  EXPECT_EQ((base::flat_map<std::string, std::string>{{"EUR", "(?i)EUR|€"}}),
            order_patterns->value_currency_regexes);
  EXPECT_EQ(std::vector<std::string>{"(?i)^Gesamt$"},
            order_patterns->value_keyword_regexes);
  EXPECT_EQ(6u, order_patterns->number_of_ancestor_levels_to_search);
  ASSERT_EQ(1u, order_patterns->value_selectors.size());
  EXPECT_EQ(R"([data-test="checkout-total"])",
            order_patterns->value_selectors[0]->selector);
  EXPECT_FALSE(order_patterns->value_selectors[0]->attribute_name);
}

TEST(BraveAdsOrderPatternsMojomUtilTest,
     ConvertEmptyOrderPatternsIfNeitherIdNorValueSet) {
  // Arrange
  mojom::OrderExtractionPatternsInfoPtr mojom_order_patterns =
      mojom::OrderExtractionPatternsInfo::New();

  // Act
  const mojom::OrderPatternsInfoPtr order_patterns =
      ToMojomOrderPatterns(*mojom_order_patterns);

  // Assert
  EXPECT_THAT(order_patterns->id_regexes, IsEmpty());
  EXPECT_THAT(order_patterns->id_selectors, IsEmpty());
  EXPECT_THAT(order_patterns->value_currency_regexes, IsEmpty());
  EXPECT_THAT(order_patterns->value_keyword_regexes, IsEmpty());
  EXPECT_EQ(0u, order_patterns->number_of_ancestor_levels_to_search);
  EXPECT_THAT(order_patterns->value_selectors, IsEmpty());
}

}  // namespace brave_ads
